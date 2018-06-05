/*
 * Copyright (c) 2017, RISE SICS
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BOARD_H_
#define BOARD_H_

#define PLATFORM_HAS_LEDS     1
#define PLATFORM_HAS_BUTTON   1
#define PLATFORM_HAS_RADIO    1
#define PLATFORM_HAS_SENSORS  0

#define LEDS_CONF_LEGACY_API  1
#define LEDS_CONF_RED    1
#define LEDS_CONF_GREEN  2

#define GPIO_HAL_CONF_PIN_COUNT 64
#define GPIO_EFR32_CONFIG {}

/*
 * Override button symbols from dev/button-sensor.h, for the examples that
 * include it
 */
#define button_sensor button_left_sensor
#define button_sensor2 button_right_sensor

/*---------------------------------------------------------------------------*/
#define BUTTON_SENSOR "Button"
/*---------------------------------------------------------------------------*/
#define BUTTON_SENSOR_VALUE_STATE    0
#define BUTTON_SENSOR_VALUE_DURATION 1

#define BUTTON_SENSOR_VALUE_RELEASED 0
#define BUTTON_SENSOR_VALUE_PRESSED  1

/*---------------------------------------------------------------------------*/
extern const struct sensors_sensor button_left_sensor;
extern const struct sensors_sensor button_right_sensor;

/*---------------------------------------------------------------------------*/

#define BSP_SERIAL_APP_CTS_PIN                (2)
#define BSP_SERIAL_APP_CTS_PORT               (gpioPortA)
#define BSP_SERIAL_APP_CTS_LOC                (30)

#define BSP_SERIAL_APP_RX_PIN                 (1)
#define BSP_SERIAL_APP_RX_PORT                (gpioPortA)
#define BSP_SERIAL_APP_RX_LOC                 (USART_ROUTELOC0_RXLOC_LOC0)

#define BSP_SERIAL_APP_TX_PIN                 (0)
#define BSP_SERIAL_APP_TX_PORT                (gpioPortA)
#define BSP_SERIAL_APP_TX_LOC                 (USART_ROUTELOC0_TXLOC_LOC0)

#define BSP_SERIAL_APP_RTS_PIN                (3)
#define BSP_SERIAL_APP_RTS_PORT               (gpioPortA)
#define BSP_SERIAL_APP_RTS_LOC                (30)


#define BOARD_STRING      "TB-Sense-2"

#define BOARD_BUTTON_PORT         gpioPortD       /**< Pushbutton port                  */
#define BOARD_BUTTON_SHIFT        14              /**< Pushbutton shift value           */
#define BOARD_BUTTON_LEFT         0x01            /**< Left pushbutton value            */
#define BOARD_BUTTON_RIGHT        0x02            /**< Right pushbutton value           */
#define BOARD_BUTTON_MASK         0x03            /**< Pushbutton mask                  */
#define BOARD_BUTTON_LEFT_PORT    gpioPortD       /**< Left pushbutton port             */
#define BOARD_BUTTON_LEFT_PIN     14              /**< Left pushbutton pin              */
#define BOARD_BUTTON_RIGHT_PORT   gpioPortD       /**< Right pushbutton port            */
#define BOARD_BUTTON_RIGHT_PIN    15              /**< Right pushbutton pin             */
#define BOARD_BUTTON_INT_FLAG     0x04            /**< Pushbutton interrupt flag value  */
#define BOARD_BUTTON_INT_ENABLE   true            /**< Pushbutton interrupt enable      */


#define BOARD_ENV_ENABLE_PORT       gpioPortF
#define BOARD_ENV_ENABLE_PIN        9

#define BOARD_IMU_ENABLE_PORT       gpioPortF     /**< IMU enable port                  */
#define BOARD_IMU_ENABLE_PIN        8             /**< IMU enable pin                   */
#define BOARD_IMU_INT_PORT          gpioPortF     /**< IMU interrupt port               */
#define BOARD_IMU_INT_PIN           12            /**< IMU interrupt pin                */
#define BOARD_IMU_SPI_PORT          gpioPortC     /**< IMU SPI port                     */
#define BOARD_IMU_SPI_MOSI_PIN      0             /**< IMU SPI master out slave in pin  */
#define BOARD_IMU_SPI_MISO_PIN      1             /**< IMU SPI master in slave out pin  */
#define BOARD_IMU_SPI_SCLK_PIN      2             /**< IMU SPI serial clock pin         */
#define BOARD_IMU_SPI_CS_PIN        3             /**< IMU SPI chip select pin          */

#define SI1133_I2C_BUS i2c1_bus
#define SI7021_I2C_BUS i2c1_bus
#define BMP_I2C_BUS    i2c1_bus

/* Bit fields for PIC_REG_LED_CTRL */
#define BOARD_PIC_REG_LED_CTRL_PWR_EN       0x01   /**< LED control register, Power enable bit        */
#define BOARD_PIC_REG_LED_CTRL_LED0         0x10   /**< LED control register, LED0 control bit        */
#define BOARD_PIC_REG_LED_CTRL_LED1         0x20   /**< LED control register, LED1 control bit        */
#define BOARD_PIC_REG_LED_CTRL_LED2         0x40   /**< LED control register, LED2 control bit        */
#define BOARD_PIC_REG_LED_CTRL_LED3         0x80   /**< LED control register, LED3 control bit        */
#define BOARD_PIC_REG_LED_CTRL_LED_MASK     0xf0   /**< LED control register, LED control mask        */
#define BOARD_PIC_REG_LED_CTRL_LED_SHIFT    4      /**< LED control register, LED control shift value */

#define BOARD_RGBLED_TIMER        (TIMER1)        /**< RGB LED PWM control timer        */
#define BOARD_RGBLED_CMU_CLK      cmuClock_TIMER1 /**< RGB LED PWM control clock source */
#define BOARD_RGBLED_RED_PORT     gpioPortD       /**< RGB LED Red port                 */
#define BOARD_RGBLED_RED_PIN      11              /**< RGB LED Red pin                  */
#define BOARD_RGBLED_RED_CCLOC    19              /**< RGB LED Red CC location          */
#define BOARD_RGBLED_GREEN_PORT   gpioPortD       /**< RGB LED Green port               */
#define BOARD_RGBLED_GREEN_PIN    12              /**< RGB LED Green pin                */
#define BOARD_RGBLED_GREEN_CCLOC  19              /**< RGB LED Green CC location        */
#define BOARD_RGBLED_BLUE_PORT    gpioPortD       /**< RGB LED Blue port                */
#define BOARD_RGBLED_BLUE_PIN     13              /**< RGB LED Blue pin                 */
#define BOARD_RGBLED_BLUE_CCLOC   19              /**< RGB LED Blue CC location         */

#define BOARD_LED_PORT            gpioPortD       /**< LED port                         */
#define BOARD_LED_RED_PORT        gpioPortD       /**< Red LED port                     */
#define BOARD_LED_RED_PIN         8               /**< Red LED pin                      */
#define BOARD_LED_GREEN_PORT      gpioPortD       /**< Green LED port                   */
#define BOARD_LED_GREEN_PIN       9               /**< Green LED pin                    */
#define BOARD_RGBLED_PWR_EN_PORT  gpioPortJ       /**< RGB LED Power Enable port        */
#define BOARD_RGBLED_PWR_EN_PIN   14              /**< RGB LED Power Enable pin         */
#define BOARD_RGBLED_COM_PORT     gpioPortI       /**< RGB LED COM port                 */
#define BOARD_RGBLED_COM0_PORT    gpioPortI       /**< RGB LED COM0 port                */
#define BOARD_RGBLED_COM0_PIN     0               /**< RGB LED COM0 pin                 */
#define BOARD_RGBLED_COM1_PORT    gpioPortI       /**< RGB LED COM1 port                */
#define BOARD_RGBLED_COM1_PIN     1               /**< RGB LED COM1 pin                 */
#define BOARD_RGBLED_COM2_PORT    gpioPortI       /**< RGB LED COM2 port                */
#define BOARD_RGBLED_COM2_PIN     2               /**< RGB LED COM2 pin                 */
#define BOARD_RGBLED_COM3_PORT    gpioPortI       /**< RGB LED COM3 port                */
#define BOARD_RGBLED_COM3_PIN     3               /**< RGB LED COM3 pin                 */

/* External interrupts */
#define EXTI_BUTTON1              15
#define EXTI_BUTTON0              14
#define EXTI_CCS811_INT           13
#define EXTI_IMU_INT              12
#define EXTI_UV_ALS_INT           11
#define EXTI_HALL_OUT1            10


uint32_t board_imu_enable(int enable);

#endif /* BOARD_H_ */
