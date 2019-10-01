#ifndef I2C_ARCH_H_
#define I2C_ARCH_H_

#include "em_i2c.h"

/**
 * see EFR32MG1 datasheet for allowed I2C locations
 * see efr32mg1p_i2c.h for possible values
 */

typedef struct {
  uint32_t sda_loc;
  uint32_t scl_loc;
  I2C_TypeDef *I2Cx;
} i2c_bus_config_t;

#endif /* I2C_ARCH_H_ */
