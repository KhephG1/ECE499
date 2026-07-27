#include "scd40_driver.h"
#include "scd40.h"
#include "scd40_interface.h"
#include "uart_logs.h"
#include <stdint.h>
#define SCD40_BUF_SIZE (100)

uint16_t scd40_CO2_buf[SCD40_BUF_SIZE] = {};
float scd40_hum_buf[SCD40_BUF_SIZE] = {};
float scd40_temp_buf[SCD40_BUF_SIZE] = {};
static uint8_t num_readings = 0;

uint8_t init_scd40(scd4x_handle_t *gs_handle, scd4x_t type, float cal_pressure, float cal_temp) {
    uint8_t result;
    DRIVER_SCD4X_LINK_INIT(gs_handle, scd4x_handle_t);
    DRIVER_SCD4X_LINK_IIC_INIT(gs_handle, scd4x_interface_iic_init);
    DRIVER_SCD4X_LINK_IIC_DEINIT(gs_handle, scd4x_interface_iic_deinit);
    DRIVER_SCD4X_LINK_IIC_WRITE_COMMAND(gs_handle, scd4x_interface_iic_write_cmd);
    DRIVER_SCD4X_LINK_IIC_READ_COMMAND(gs_handle, scd4x_interface_iic_read_cmd);
    DRIVER_SCD4X_LINK_DELAY_MS(gs_handle, scd4x_interface_delay_ms);
    DRIVER_SCD4X_LINK_DEBUG_PRINT(gs_handle, scd4x_interface_debug_print);
    result = scd4x_set_type(gs_handle, type);
    if (result != 0)
    {
        scd4x_interface_debug_print("scd4x: set type failed.\r\n");
    
        return 1;
    }
    
    /* scd4x init */
    result = scd4x_init(gs_handle);
    if (result != 0)
    {
        scd4x_interface_debug_print("scd4x: init failed.\r\n");
        
        return 1;
    }
    
    /* start */
    scd4x_perform_self_test(gs_handle, &result);
    if(result){
        scd4x_interface_debug_print("scd4x: self test failed.\r\n");
    }
    //calibrate for temperature and pressure
    uint16_t pressure_reg = 0;
    uint16_t temp_reg = 0;
    scd4x_ambient_pressure_convert_to_register(gs_handle, cal_pressure,&pressure_reg);
    scd4x_set_ambient_pressure(gs_handle,pressure_reg);
    scd4x_temperature_offset_convert_to_register(gs_handle, cal_temp, &temp_reg);
    scd4x_set_temperature_offset(gs_handle, temp_reg);
    //enable automatic self calibration (targets ambient CO2 of 400ppm by default)
    scd4x_set_automatic_self_calibration(gs_handle, SCD4X_BOOL_TRUE);
    //start SCD40 with 30 second sampling period
    result = scd4x_start_low_power_periodic_measurement(gs_handle);
    //start SCD40 with 5 second sampling period - TODO: This is for testing only and should be reverted on pcb
    //result = scd4x_start_periodic_measurement(gs_handle);
    if (result != 0)
    {
        scd4x_interface_debug_print("scd4x: start periodic measurement failed.\n");
        (void)scd4x_deinit(gs_handle);
        
        return 1;
    }
    
    return 0;

}

uint8_t scd4x_basic_read(scd4x_handle_t *gs_handle)
{
    uint8_t res;
    uint16_t co2_raw;
    uint16_t temperature_raw;
    uint16_t humidity_raw;
    
    /* read data */
    res = scd4x_read(gs_handle, &co2_raw, &scd40_CO2_buf[num_readings],
                     &temperature_raw, &scd40_temp_buf[num_readings],
                     &humidity_raw, &scd40_hum_buf[num_readings]);
    if (res != 0)
    {
        return res;
    }
    char co2[20] = {};
    char temp[20] = {};
    char hum[20] = {};
    intToStr( (int)scd40_CO2_buf[num_readings], co2,3 );
    my_ftoa(scd40_temp_buf[num_readings],temp,3);
    my_ftoa(scd40_hum_buf[num_readings],hum,3);
    gs_handle->debug_print("SCD40: %s , %s , %s \r\n",co2,temp,hum);
    num_readings++;
    if(num_readings >= SCD40_BUF_SIZE){
        num_readings = 0;
    }
    return 0;
}

uint16_t* get_scd40_CO2_readings(){
    return scd40_CO2_buf;
}
float* get_scd40_temp_readings(){
    return scd40_temp_buf;
}
float* get_scd40_hum_readings(){
    return scd40_hum_buf;
}
uint8_t get_scd40_latest_index(){
    return (num_readings == 0) ? (SCD40_BUF_SIZE - 1) : (num_readings - 1);
}