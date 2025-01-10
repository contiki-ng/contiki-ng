#include "contiki.h"

#include <stdio.h>

#include "touch-sensor.h"
/*---------------------------------------------------------------------------*/
PROCESS(touch_process, "TOUCH process");
AUTOSTART_PROCESSES(&touch_process);
/*---------------------------------------------------------------------------*/
#define TOUCH_GPIO_PORT (1)
#define TOUCH_GPIO_PIN (15)

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(touch_process, ev, data)
{
  static struct etimer timer;
  int touch_value;

  PROCESS_BEGIN();

  touch_sensor.configure(TOUCH_CONFIGURE_GPIO_PORT, TOUCH_GPIO_PORT);
  touch_sensor.configure(TOUCH_CONFIGURE_GPIO_PIN, TOUCH_GPIO_PIN);
 
  touch_sensor.configure(SENSORS_HW_INIT, 0);

  printf("Gas Sensor Warming up");
  /* Wait 2 seconds for the MQ sensor to be ready */
  etimer_set(&timer, CLOCK_SECOND * 2);
  
  /* Wait for the periodic timer to expire */
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

  /* Setup a periodic timer that expires after 1 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 1);
  while(1) {
    /*
     * Request a fresh read
     */
    SENSORS_ACTIVATE(touch_sensor);

    switch(touch_sensor.status(0)) {
        case TOUCH_STATUS_OKAY:
            touch_value = touch_sensor.value(0);
            printf("Touch_switch = %d\n", touch_value);
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
