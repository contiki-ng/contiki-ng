#include "contiki.h"

#include <stdio.h>

#include "linear_hall-sensor.h"
/*---------------------------------------------------------------------------*/
PROCESS(hall_process, "HALL process");
AUTOSTART_PROCESSES(&hall_process);
/*---------------------------------------------------------------------------*/
#define HALL_GPIO_PORT (1)
#define HALL_GPIO_PIN (15)

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hall_process, ev, data)
{
  static struct etimer timer;
  int hall_value;

  PROCESS_BEGIN();

  hall_sensor.configure(HALL_CONFIGURE_GPIO_PORT, HALL_GPIO_PORT);
  //sets the GPIO port to be HAL_GPIO_PORT
  hall_sensor.configure(HALL_CONFIGURE_GPIO_PIN, HALL_GPIO_PIN);
  //sets the GPIO pin to be HAL_GPIO_PIN
  hall_sensor.configure(SENSORS_HW_INIT, 0);
  //initializes the sensor, sets the last read time to 0

  printf("Sensor Warming up");
  /* Wait 2 seconds for the MQ sensor to be ready */
  etimer_set(&timer, CLOCK_SECOND * 5);
  
  /* Wait for the periodic timer to expire */
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

  /* Activate the sensor after warm-up */
  SENSORS_ACTIVATE(hall_sensor);

  /* Setup a periodic timer that expires after 1 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 5);
  while(1) {
    /*
     * Request a fresh read
     */
  
    switch(hall_sensor.status(0)) {
        case HALL_STATUS_OKAY:
            hall_value = hall_sensor.value(0);
            printf("Hall_switch = %d\n", hall_value);
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
