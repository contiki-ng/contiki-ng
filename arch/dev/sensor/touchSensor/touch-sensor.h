#ifndef TOUCH_SENSOR_H_
#define TOUCH_SENSOR_H_

#include "sensors.h"

extern const struct sensors_sensor touch_sensor;

#define TOUCH_CONFIGURE_GPIO_PORT   (0)

#define TOUCH_CONFIGURE_GPIO_PIN    (1)

#define TOUCH_STATUS_NOT_FOUND (1)

#define TOUCH_STATUS_OKAY (0)

#define TOUCH_VALUE    (1)

#endif
