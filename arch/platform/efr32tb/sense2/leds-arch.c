#include <em_device.h>
#include <em_cmu.h>
#include <em_gpio.h>
#include <dev/leds.h>
/*---------------------------------------------------------------------------*/
void
leds_arch_init(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  /* Initialize LEDs (RED and Green) */
  GPIO_PinModeSet(BOARD_LED_RED_PORT, BOARD_LED_RED_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(BOARD_LED_GREEN_PORT, BOARD_LED_GREEN_PIN, gpioModePushPull, 0);
}
/*---------------------------------------------------------------------------*/
unsigned char
leds_arch_get(void)
{
  unsigned char state = 0;
  state |= GPIO_PinOutGet(BOARD_LED_RED_PORT, BOARD_LED_RED_PIN) ? LEDS_RED : 0;
  state |= GPIO_PinOutGet(BOARD_LED_GREEN_PORT, BOARD_LED_GREEN_PIN) ? LEDS_GREEN : 0;
  return state;
}
/*---------------------------------------------------------------------------*/
void
leds_arch_set(unsigned char leds)
{
  (leds & LEDS_RED ? GPIO_PinOutSet : GPIO_PinOutClear)(BOARD_LED_RED_PORT, BOARD_LED_RED_PIN);
  (leds & LEDS_GREEN ? GPIO_PinOutSet : GPIO_PinOutClear)(BOARD_LED_GREEN_PORT, BOARD_LED_GREEN_PIN);
}
/*---------------------------------------------------------------------------*/
