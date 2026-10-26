#include "contiki.h"
#include "linear_hall-sensor.h"
#include <string.h>
#include "dev/gpio-hal.h"

/*---------------------------------------------------------------------------*/
/**
 * @brief GPIO High
 *
 */
#define HALL_SIGNAL_HIGH           (1)

/**
 * @brief GPIO Low
 *
 */
#define HALL_SIGNAL_LOW            (0)

/*---------------------------------------------------------------------------*/
/**
 * @brief SR04 struct
 *
 */
typedef struct {
  /**
   * @brief GPIO Port
   *
   */
  gpio_hal_port_t port;
  /**
   * @brief GPIO Pin
   *
   */
  gpio_hal_pin_t pin;
  
  clock_time_t last_read;
  /**
   * @brief Data 
   *
   */
  int hall_value;

  uint8_t status;
} hall_t;

static hall_t hall_s;

/*---------------------------------------------------------------------------*/

// static int 
// sr04_signal_duration(uint8_t active, uint32_t max_duration) 
// {
//   rtimer_clock_t elapsed_ticks;
//   rtimer_clock_t max_wait_ticks = US_TO_RTIMERTICKS(max_duration);
  
//   /* Wait for signal to change to active*/
//   RTIMER_BUSYWAIT_UNTIL(gpio_hal_arch_read_pin(sr04.echo_port, sr04.echo_pin) == active, max_wait_ticks);

//   rtimer_clock_t start_ticks = RTIMER_NOW();

//   /* Wait for signal to change to non-active */
//   RTIMER_BUSYWAIT_UNTIL(gpio_hal_arch_read_pin(sr04.echo_port, sr04.echo_pin) != active, max_wait_ticks);

//   elapsed_ticks = RTIMER_NOW() - start_ticks;

//   if(elapsed_ticks >= max_wait_ticks) {
//     return -1;
//   }

//   return RTIMERTICKS_TO_US(elapsed_ticks);
// }
/*---------------------------------------------------------------------------*/
static uint8_t
hall_read(void)
{
  int data; 

  gpio_hal_arch_pin_set_input(hall_s.port, hall_s.pin);
  data = gpio_hal_arch_read_pin(hall_s.port, hall_s.pin);

  hall_s.hall_value = data;
  // hall_s.hall_value = value;

  /* Verify  checksum */
 return HALL_STATUS_OKAY;
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  (void)type;
  return hall_s.hall_value;
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  (void)type;

  return hall_s.status;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  switch(type) {
  case HALL_CONFIGURE_GPIO_PORT:
    hall_s.port = c;
    break;
  case HALL_CONFIGURE_GPIO_PIN:
    hall_s.pin = c;
    break;
  case SENSORS_HW_INIT:
    hall_s.last_read = 0;
  case SENSORS_ACTIVE:
    if(c == 1) {
      clock_time_t now;

      now = clock_seconds();
      // if(now - sr04.last_read < SR04_SAMPLING_RATE) {
      //   return 0;
      // }
      hall_s.last_read = now;
      hall_s.status = hall_read();
    }
  case SENSORS_READY:
    break;
  default:
    return 0;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(hall_sensor, "hall", value, configure, status);
/*----------------------------------------------------------------------------*/
/** @} */
