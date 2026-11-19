#include "contiki.h"
#include "sr04-sensor.h"
#include <string.h>
#include "dev/gpio-hal.h"

/*---------------------------------------------------------------------------*/
/**
 * @brief GPIO High
 *
 */
#define SR04_SIGNAL_HIGH           (1)

/**
 * @brief GPIO Low
 *
 */
#define SR04_SIGNAL_LOW            (0)

/**
 * @brief Duration of Trigger pulse according to datasheet
 *
 */
#define SR04_TRIGGER_PULSE_DURATION (10)

#define SR04_SAMPLING_RATE (1)

#define SR04_US_GUARD     RTIMERTICKS_TO_US(1)

#define SR04_READ_MAXIMUM_DURATION (32000)

/*---------------------------------------------------------------------------*/
/**
 * @brief SR04 struct
 *
 */
typedef struct {
  /**
   * @brief GPIO Port for Trig
   *
   */
  gpio_hal_port_t trig_port;
  /**
   * @brief GPIO Pin for Trig
   *
   */
  gpio_hal_pin_t trig_pin;
  /**
   * @brief GPIO Port for Echo
   *
   */
  gpio_hal_port_t echo_port;
  /**
   * @brief GPIO Pin for Echo
   *
   */
  gpio_hal_pin_t echo_pin;
  /**
   * @brief Time of last read
   *
   */
  clock_time_t last_read;
  /**
   * @brief Data 
   *
   */
  int duration;
  int distance;
  uint8_t status;
} sr_t;

static sr_t sr04;

/*---------------------------------------------------------------------------*/

static int 
sr04_signal_duration(uint8_t active, uint32_t max_duration) 
{
  rtimer_clock_t elapsed_ticks;
  rtimer_clock_t max_wait_ticks = US_TO_RTIMERTICKS(max_duration);
  
  /* Wait for signal to change to active*/
  RTIMER_BUSYWAIT_UNTIL(gpio_hal_arch_read_pin(sr04.echo_port, sr04.echo_pin) == active, max_wait_ticks);

  rtimer_clock_t start_ticks = RTIMER_NOW();

  /* Wait for signal to change to non-active */
  RTIMER_BUSYWAIT_UNTIL(gpio_hal_arch_read_pin(sr04.echo_port, sr04.echo_pin) != active, max_wait_ticks);

  elapsed_ticks = RTIMER_NOW() - start_ticks;

  if(elapsed_ticks >= max_wait_ticks) {
    return -1;
  }

  return RTIMERTICKS_TO_US(elapsed_ticks);
}
/*---------------------------------------------------------------------------*/
static uint8_t
sr04_read(void)
{
  int data; 
  /**
   * In order for the SR-04 to work MCU will send a HIGH pulse to TRIG pin for 10us
   */
  gpio_hal_arch_pin_set_input(sr04.echo_port, sr04.echo_pin);
  gpio_hal_arch_pin_set_output(sr04.trig_port, sr04.trig_pin);
  gpio_hal_arch_set_pin(sr04.trig_port, sr04.trig_pin);
  RTIMER_BUSYWAIT(US_TO_RTIMERTICKS(10));
  gpio_hal_arch_clear_pin(sr04.trig_port, sr04.trig_pin);

  /* Read the input from the echo port and pin */
  


  data = sr04_signal_duration(SR04_SIGNAL_HIGH, SR04_READ_MAXIMUM_DURATION);

    if(data == -1) {
        return SR04_STATUS_NOT_FOUND;
    }


  sr04.duration = data;
  sr04.distance = data / 58;

 return SR04_STATUS_OKAY;
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch(type) {
    case SR04_VALUE_DURATION:
      return sr04.duration;
    case SR04_VALUE_DISTANCE:
      return sr04.distance;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  (void)type;

  return sr04.status;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  switch(type) {
  case SR04_CONFIGURE_GPIO_TRIG_PORT:
    sr04.trig_port = c;
    break;
  case SR04_CONFIGURE_GPIO_TRIG_PIN:
    sr04.trig_pin = c;
    break;
  case SR04_CONFIGURE_GPIO_ECHO_PORT:
    sr04.echo_port = c;
    break;
  case SR04_CONFIGURE_GPIO_ECHO_PIN:
    sr04.echo_pin = c;
    break;
  case SENSORS_HW_INIT:
    sr04.last_read = 0;
  case SENSORS_ACTIVE:
    if(c == 1) {
      clock_time_t now;

      now = clock_seconds();
      if(now - sr04.last_read < SR04_SAMPLING_RATE) {
        return 0;
      }
      sr04.last_read = now;
      sr04.status = sr04_read();
    }
  case SENSORS_READY:
    break;
  default:
    return 0;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(sr04_sensor, "sr04", value, configure, status);
/*----------------------------------------------------------------------------*/
/** @} */
