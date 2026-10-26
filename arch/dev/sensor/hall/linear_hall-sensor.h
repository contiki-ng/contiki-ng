#ifndef HALL_SENSOR_H_
#define HALL_SENSOR_H_

#include "sensors.h"

extern const struct sensors_sensor hall_sensor;

#define HALL_CONFIGURE_GPIO_PORT   (0)

#define HALL_CONFIGURE_GPIO_PIN    (15)

#define HALL_STATUS_NOT_FOUND (1)

#define HALL_STATUS_OKAY (0)

#define HALL_VALUE    (1)

#endif
