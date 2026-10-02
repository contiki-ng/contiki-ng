#include "contiki.h"

#include <stdio.h>

#include "sr04-sensor.h"
/*---------------------------------------------------------------------------*/
PROCESS(sr04_process, "SR-04 process");
AUTOSTART_PROCESSES(&sr04_process);
/*---------------------------------------------------------------------------*/
#define SR04_TRIG_GPIO_PORT (1)
#define SR04_TRIG_GPIO_PIN (15)
#define SR04_ECHO_GPIO_PORT (0)
#define SR04_ECHO_GPIO_PIN (31)
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sr04_process, ev, data)
{
  static struct etimer timer;
  int duration;
  int distance;

  PROCESS_BEGIN();

  sr04_sensor.configure(SR04_CONFIGURE_GPIO_TRIG_PORT, SR04_TRIG_GPIO_PORT);
  sr04_sensor.configure(SR04_CONFIGURE_GPIO_TRIG_PIN, SR04_TRIG_GPIO_PIN);
  sr04_sensor.configure(SR04_CONFIGURE_GPIO_ECHO_PORT, SR04_ECHO_GPIO_PORT);
  sr04_sensor.configure(SR04_CONFIGURE_GPIO_ECHO_PIN, SR04_ECHO_GPIO_PIN);
 
  sr04_sensor.configure(SENSORS_HW_INIT, 0);

  /* Wait 5 seconds for the SR04 sensor to be ready */
  etimer_set(&timer, CLOCK_SECOND * 5);

  /* Wait for the periodic timer to expire */
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

  /* Setup a periodic timer that expires after 2 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 2);
  while(1) {
    /*
     * Request a fresh read
     */
    SENSORS_ACTIVATE(sr04_sensor);

    switch(sr04_sensor.status(0)) {
        case SR04_STATUS_OKAY:
            duration = sr04_sensor.value(SR04_VALUE_DURATION);
            distance = sr04_sensor.value(SR04_VALUE_DISTANCE);
            printf("Duration = %d\tDistance = %d cm\n", duration, distance);
            break;
        case SR04_STATUS_NOT_FOUND:
            printf("No object Detected\n");
            break;
        default:
            break;
    }

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
