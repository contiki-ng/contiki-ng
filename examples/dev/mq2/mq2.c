#include "contiki.h"

#include <stdio.h>

#include "mq2-sensor.h"
/*---------------------------------------------------------------------------*/
PROCESS(mq2_process, "MQ2 process");
AUTOSTART_PROCESSES(&mq2_process);
/*---------------------------------------------------------------------------*/
#define MQ2_GPIO_PORT (1)
#define MQ2_GPIO_PIN (15)

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mq2_process, ev, data)
{
  static struct etimer timer;
  int mq2_value;

  PROCESS_BEGIN();

  mq2_sensor.configure(MQ2_CONFIGURE_GPIO_PORT, MQ2_GPIO_PORT);
  mq2_sensor.configure(MQ2_CONFIGURE_GPIO_PIN, MQ2_GPIO_PIN);
 
  mq2_sensor.configure(SENSORS_HW_INIT, 0);

  printf("Gas Sensor Warming up");
  /* Wait 2 seconds for the MQ sensor to be ready */
  etimer_set(&timer, CLOCK_SECOND * 20);
  
  /* Wait for the periodic timer to expire */
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

  /* Setup a periodic timer that expires after 1 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 1);
  while(1) {
    /*
     * Request a fresh read
     */
    SENSORS_ACTIVATE(mq2_sensor);

    switch(mq2_sensor.status(0)) {
        case MQ2_STATUS_OKAY:
            mq2_value = mq2_sensor.value(0);
            printf("Alcohol_detection = %d\n", !mq2_value);
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
