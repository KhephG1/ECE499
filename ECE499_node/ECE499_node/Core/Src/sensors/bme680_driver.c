
#define BME68X_USE_FPU
#include <bsec_datatypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "i2c.h"
#include "main.h"
#include "uart_logs.h"
#include "bme680.h"
#include "bme680_defs.h"
#include "bsec_iaq.h"
#include "bsec_interface.h"

#define DEFAULT_TIMEOUT (1000)

/******************************************************************************/
/*!                 Macro definitions                                         */
/*! BME68X shuttle board ID */
#define BME68X_SHUTTLE_ID  0x93
#define BME68X_PROFILE_LEN (10)
#define BSEC_BLOB_SIZE (492)
#define MS_TO_NS (1000000)
#define MS_TO_US (1000)
/******************************************************************************/
/*!                Static variable definition                                 */
static uint8_t dev_addr;
static uint8_t bme_current_op_mode = BME68X_SLEEP_MODE;
static uint16_t heater_dur_prof[100];
static uint16_t heater_temp_prof[100];
static struct bme68x_conf conf = {
    .os_hum = BME68X_OS_1X,
    .os_temp = BME68X_OS_2X,
    .os_pres = BME68X_OS_16X,
    .filter = BME68X_FILTER_SIZE_127,
    .odr = BME68X_ODR20_POS
};
static struct bme68x_heatr_conf heatr_conf = {
    .heatr_dur = BME68X_HEATR_DUR1,
    .heatr_temp = BME68X_LOW_TEMP,
    .profile_len = BME68X_PROFILE_LEN,
    .heatr_temp_prof = heater_temp_prof,
    .heatr_dur_prof = heater_dur_prof
};
static struct bme68x_data bme_data = {};

/******************************************************************************/
/*!                User interface functions                                   */

/*!
 */
BME68X_INTF_RET_TYPE bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
   
    uint8_t device_addr = *(uint8_t*)intf_ptr;

    (void)intf_ptr;
    uint8_t status = HAL_I2C_Master_Transmit(&hi2c4, device_addr<<1, &reg_addr, 1, DEFAULT_TIMEOUT);
    if(status != HAL_OK){
        return status;
    }
    return HAL_I2C_Master_Receive(&hi2c4, device_addr<<1, reg_data, len, DEFAULT_TIMEOUT);
}

BME68X_INTF_RET_TYPE bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t device_addr = *(uint8_t*)intf_ptr;

    (void)intf_ptr;
    uint8_t buf[len + 1] = {};
    memcpy(buf, &reg_addr,1);
    memcpy(&buf[1], reg_data, len);
    return HAL_I2C_Master_Transmit(&hi2c4, device_addr<<1, buf, (uint16_t)len + 1, DEFAULT_TIMEOUT);
}

void dwt_delay_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline void dwt_delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < cycles)
    {
        /* busy wait */
    }
}

void bme68x_delay_us(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr;
    dwt_delay_us(period);
}
void bme68x_check_rslt(const char api_name[], int8_t rslt)
{
    switch (rslt)
    {
        case BME68X_OK:

            /* Do nothing */
            break;
        case BME68X_E_NULL_PTR:
            log_debug("API name [%s]  Error [%d] : Null pointer\r\n", api_name, rslt);
            break;
        case BME68X_E_COM_FAIL:
            log_debug("API name [%s]  Error [%d] : Communication failure\r\n", api_name, rslt);
            break;
        case BME68X_E_INVALID_LENGTH:
            log_debug("API name [%s]  Error [%d] : Incorrect length parameter\r\n", api_name, rslt);
            break;
        case BME68X_E_DEV_NOT_FOUND:
            log_debug("API name [%s]  Error [%d] : Device not found\r\n", api_name, rslt);
            break;
        case BME68X_E_SELF_TEST:
            log_debug("API name [%s]  Error [%d] : Self test error\r\n", api_name, rslt);
            break;
        case BME68X_W_NO_NEW_DATA:
            log_debug("API name [%s]  Warning [%d] : No new data found\r\n", api_name, rslt);
            break;
        default:
            log_debug("API name [%s]  Error [%d] : Unknown error code\r\n", api_name, rslt);
            break;
    }
}

int8_t bme68x_interface_init(struct bme68x_dev *bme, uint8_t intf)
{
    int8_t rslt = BME68X_OK;

    if (bme != NULL)
    {
        if (intf == BME68X_I2C_INTF)
        {
            printf("I2C Interface\n");
            dev_addr = BME68X_I2C_ADDR_LOW;
            bme->read = bme68x_i2c_read;
            bme->write = bme68x_i2c_write;
            bme->intf = BME68X_I2C_INTF;
        }

        bme->delay_us = bme68x_delay_us;
        bme->intf_ptr = &dev_addr;
        bme->amb_temp = 25; /* The ambient temperature in deg C is used for defining the heater temperature */
        
        rslt = bme68x_init(bme);
        rslt = bme68x_get_conf(&conf, bme);
        rslt = bme68x_get_heatr_conf(&heatr_conf, bme);
        rslt = bme68x_set_conf(&conf, bme);
        if(rslt != BME68X_OK){
            return rslt;
        }
        rslt = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, bme);
        if(rslt != BME68X_OK){
            return rslt;
        }
        bme68x_set_op_mode(BME68X_FORCED_MODE, bme);
        if(rslt != BME68X_OK){
            return rslt;
        }
        return BME68X_OK;
    }
    return BME68X_E_NULL_PTR;
}

int8_t bme680_init(struct bme68x_dev *bme, uint8_t intf){
    int8_t status = {};
    uint8_t work_buffer_state[BSEC_MAX_WORKBUFFER_SIZE];
    uint8_t serialized_state[BSEC_MAX_STATE_BLOB_SIZE];
    uint32_t n_serialized_state = BSEC_MAX_STATE_BLOB_SIZE;
    bsec_sensor_configuration_t requested_virtual_sensors[6];
    bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
    uint8_t n_requested_virtual_sensors = 6;
    uint8_t  n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;
    //initialize the bme68x sensor:
    status = bme68x_interface_init(bme,intf );
    if(status != BME68X_OK){
        return status;
    }
    status = bsec_init();
    if(status != BSEC_OK){
        return status;
    }
    status = bsec_set_configuration(bsec_config_iaq, sizeof(bsec_config_iaq), work_buffer_state, BSEC_MAX_WORKBUFFER_SIZE);
    if(status != BSEC_OK){
        return status;
    }
    status = bsec_get_state(0,serialized_state,BSEC_MAX_STATE_BLOB_SIZE,work_buffer_state,BSEC_MAX_WORKBUFFER_SIZE, &n_serialized_state);
    if(status != BSEC_OK){
        return status;
    }
    status = bsec_set_state(serialized_state,BSEC_MAX_STATE_BLOB_SIZE,work_buffer_state,BSEC_MAX_WORKBUFFER_SIZE);
    if(status != BSEC_OK){
        return status;
    }
    for(int i = 0; i < n_requested_virtual_sensors; i++){
        requested_virtual_sensors[i].sample_rate = BSEC_SAMPLE_RATE_LP;
    }
    requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_CO2_EQUIVALENT;
    requested_virtual_sensors[1].sensor_id = BSEC_OUTPUT_BREATH_VOC_EQUIVALENT;
    requested_virtual_sensors[2].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY;
    requested_virtual_sensors[3].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE;
    requested_virtual_sensors[4].sensor_id = BSEC_OUTPUT_RAW_PRESSURE;
    requested_virtual_sensors[5].sensor_id = BSEC_OUTPUT_STABILIZATION_STATUS;
    status = bsec_update_subscription(requested_virtual_sensors, n_requested_virtual_sensors,required_sensor_settings, &n_required_sensor_settings);
    if(status != BSEC_OK){
        return status;
    }
    return BSEC_OK;
}


int8_t bme680_step(struct bme68x_dev* dev, bsec_output_t* outputs, uint8_t* n_outputs){
    bsec_bme_settings_t sensor_settings;
    uint8_t ndata = 1;
    int64_t time_stamp = (int64_t)HAL_GetTick() * MS_TO_NS;
    bme68x_get_op_mode(&bme_current_op_mode, dev);
    bsec_sensor_control(time_stamp, &sensor_settings);
    if(sensor_settings.op_mode != bme_current_op_mode){
        conf.os_hum = sensor_settings.humidity_oversampling;
        conf.os_pres = sensor_settings.pressure_oversampling;
        conf.os_temp = sensor_settings.temperature_oversampling;
        bme68x_set_conf(&conf, dev);
        bme68x_set_op_mode(sensor_settings.op_mode, dev);
        bme_current_op_mode = sensor_settings.op_mode;
    }
    if(sensor_settings.trigger_measurement == 1){
       int8_t rslt = bme68x_get_data(bme_current_op_mode,&bme_data,&ndata, dev);
       if(rslt != BME68X_OK){
        return rslt;
       }
    }
    if(sensor_settings.process_data != 0){
        int64_t timestamp = HAL_GetTick() * MS_TO_NS;
        bsec_input_t inputs[4] = {
            [0] = {
                .sensor_id = BSEC_INPUT_TEMPERATURE,
                .signal = bme_data.temperature,
                .signal_dimensions = 1,
                .time_stamp = timestamp
            },
            [1] = {
                .sensor_id = BSEC_INPUT_HUMIDITY,
                .signal = bme_data.humidity,
                .signal_dimensions = 1,
                .time_stamp = timestamp
            },
            [2] = {
                .sensor_id = BSEC_INPUT_GASRESISTOR,
                .signal = bme_data.gas_resistance,
                .signal_dimensions = 1,
                .time_stamp = timestamp
            },
            [3] = {
                .sensor_id = BSEC_INPUT_PRESSURE,
                .signal = bme_data.pressure,
                .signal_dimensions = 1,
                .time_stamp = timestamp
            }
        };
        int8_t status = bsec_do_steps(inputs, 4, outputs, n_outputs);
        char eco2[20] = {};
        char bvoc[20] = {};
        char temp[20] = {};
        char hum[20] = {};
        char pres[20] = {};
        intToStr(outputs[0].signal, eco2, 0);
        my_ftoa(outputs[1].signal, bvoc, 3);
        my_ftoa(outputs[2].signal, pres, 3);
        my_ftoa(outputs[4].signal, temp, 3);
        my_ftoa(outputs[5].signal, hum, 3);
        log_debug("BME680: %s %s %s %s %s %d\r\n",eco2,bvoc,pres,temp,hum, outputs[0].accuracy );
        if(status != BSEC_OK){
            return status;
        }
    }
    return BME68X_OK;
}