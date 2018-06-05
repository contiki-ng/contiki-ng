#ifndef BMP_DRIVER_H
#define BMP_DRIVER_H

#define BMP_OK                             0x0000
#define BMP_ERROR_DRIVER_NOT_INITIALIZED   0x0001
#define BMP_ERROR_I2C_TRANSACTION_FAILED   0x0002
#define BMP_ERROR_DEVICE_ID_MISMATCH       0x0003
#define BMP_DEVICE_ID_BMP280               0x58

typedef struct {
  uint8_t oversampling;     /**< Oversampling value                         */
  uint8_t powerMode;        /**< SLEEP, FORCED or NORMAL power mode setting */
  uint8_t standbyTime;      /**< Standby time setting                       */
} bmp_config_t;

uint32_t bmp_init(uint8_t *devid);
uint32_t bmp_config(bmp_config_t *cfg);
uint32_t bmp_get_temperature_pressure(int32_t *temp, uint32_t *pressure);

#endif /* BMP_DRIVER_H */
