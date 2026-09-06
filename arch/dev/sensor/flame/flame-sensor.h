#ifndef FLAME_SENSOR_H_
#define FLAME_SENSOR_H_

#include "sensors.h"

extern const struct sensors_sensor flame_sensor;

#define FLAME_CONFIGURE_GPIO_PORT   (0)

#define FLAME_CONFIGURE_GPIO_PIN    (1)

#define FLAME_STATUS_NOT_FOUND (1)

#define FLAME_STATUS_OKAY (0)

#define FLAME_VALUE    (1)

#endif
