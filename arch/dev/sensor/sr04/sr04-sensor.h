#ifndef SR04_SENSOR_H_
#define SR04_SENSOR_H_

#include "sensors.h"

extern const struct sensors_sensor sr04_sensor;

#define SR04_CONFIGURE_GPIO_TRIG_PORT   (0)

#define SR04_CONFIGURE_GPIO_TRIG_PIN    (1)

#define SR04_CONFIGURE_GPIO_ECHO_PORT   (2)

#define SR04_CONFIGURE_GPIO_ECHO_PIN    (3)

#define SR04_STATUS_NOT_FOUND (1)

#define SR04_STATUS_OKAY (0)

#define SR04_VALUE_DISTANCE    (0)

#define SR04_VALUE_DURATION    (1)

#endif
