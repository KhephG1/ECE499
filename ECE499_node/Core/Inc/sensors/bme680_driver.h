#ifndef BME680_DRIVER_H
#define BME680_DRIVER_H
#include "bsec_interface.h"
#include <bsec_datatypes.h>
int8_t bme680_init(struct bme68x_dev *bme, uint8_t intf);
int8_t bme680_step(struct bme68x_dev* dev, bsec_output_t* outputs, uint8_t* outpuuts);
void dwt_delay_init(void);
#endif