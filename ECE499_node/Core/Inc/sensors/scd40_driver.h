#ifndef SCD40_DRIVER_H
#define SCD40_DRIVER_H

//initialize the SCD40 with a given handle, type, and calibration pressure
uint8_t init_scd40(scd4x_handle_t *gs_handle, scd4x_t type, float cal_pressure, float cal_temp);
//read from the SCD40. Should not be called more than once every 30 seconds. Returns 0 on success, 1 on failure
uint8_t scd4x_basic_read(scd4x_handle_t *gs_handle);

float* get_scd40_hum_readings();

float* get_scd40_temp_readings();

int* get_scd40_CO2_readings();

#endif