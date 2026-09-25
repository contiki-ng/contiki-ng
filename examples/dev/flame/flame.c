#include "contiki.h"

#include <stdio.h>

#include "flame-sensor.h"
/*---------------------------------------------------------------------------*/
PROCESS(flame_process, "FLAME process");
AUTOSTART_PROCESSES(&flame_process);
/*---------------------------------------------------------------------------*/
#define FLAME_GPIO_PORT (1)
#define FLAME_GPIO_PIN (15)

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(flame_process, ev, data)
{
  static struct etimer timer;
  int flame_value;

  PROCESS_BEGIN();

  flame_sensor.configure(FLAME_CONFIGURE_GPIO_PORT, FLAME_GPIO_PORT);
  flame_sensor.configure(FLAME_CONFIGURE_GPIO_PIN, FLAME_GPIO_PIN);
 
  flame_sensor.configure(SENSORS_HW_INIT, 0);

  /* Wait 2 seconds for the SR04 sensor to be ready */
  etimer_set(&timer, CLOCK_SECOND * 2);

  /* Wait for the periodic timer to expire */
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

  /* Setup a periodic timer that expires after 1 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 1);
  while(1) {
    /*
     * Request a fresh read
     */
    SENSORS_ACTIVATE(flame_sensor);

    switch(flame_sensor.status(0)) {
        case FLAME_STATUS_OKAY:
            flame_value = flame_sensor.value(0);
            printf("Flame_value = %d\n", flame_value);
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
