/*
 *  ======== ti_drivers_config.h ========
 *  Configured TI-Drivers module declarations
 *
 *  The macros defines herein are intended for use by applications which
 *  directly include this header. These macros should NOT be hard coded or
 *  copied into library source code.
 *
 *  Symbols declared as const are intended for use with libraries.
 *  Library source code must extern the correct symbol--which is resolved
 *  when the application is linked.
 *
 *  DO NOT EDIT - This file is generated for the LP_EM_CC1354P10_1
 *  by the SysConfig tool.
 */
#ifndef ti_drivers_config_h
#define ti_drivers_config_h

#define CONFIG_SYSCONFIG_PREVIEW

#define CONFIG_LP_EM_CC1354P10_1
#ifndef DeviceFamily_CC13X4
#define DeviceFamily_CC13X4
#endif

#include <ti/devices/DeviceFamily.h>

#include <stdint.h>

/* support C++ sources */
#ifdef __cplusplus
extern "C" {
#endif


/*
 *  ======== CCFG ========
 */


/*
 *  ======== GPIO ========
 */
extern const uint_least8_t CONFIG_GPIO_LED_0_CONST;
#define CONFIG_GPIO_LED_0 6

extern const uint_least8_t CONFIG_GPIO_LED_1_CONST;
#define CONFIG_GPIO_LED_1 7

extern const uint_least8_t CONFIG_GPIO_BTN_1_CONST;
#define CONFIG_GPIO_BTN_1 15

extern const uint_least8_t CONFIG_GPIO_BTN_2_CONST;
#define CONFIG_GPIO_BTN_2 14
 
/* Owned by /ti/drivers/RF as  */
extern const uint_least8_t CONFIG_RF_24GHZ_CONST;
#define CONFIG_RF_24GHZ 34

/* Owned by /ti/drivers/RF as  */
extern const uint_least8_t CONFIG_RF_HIGH_PA_CONST;
#define CONFIG_RF_HIGH_PA 3

/* Owned by /ti/drivers/RF as  */
extern const uint_least8_t CONFIG_RF_SUB1GHZ_CONST;
#define CONFIG_RF_SUB1GHZ 35

/* Owned by CONFIG_UART2_0 as  */
extern const uint_least8_t CONFIG_PIN_UART_TX_CONST;
#define CONFIG_PIN_UART_TX 13

/* Owned by CONFIG_UART2_0 as  */
extern const uint_least8_t CONFIG_PIN_UART_RX_CONST;
#define CONFIG_PIN_UART_RX 12

/* Owned by CONFIG_SPI_0 as  */
extern const uint_least8_t CONFIG_GPIO_SPI_0_SCLK_CONST;
#define CONFIG_GPIO_SPI_0_SCLK 18

/* Owned by CONFIG_SPI_0 as  */
extern const uint_least8_t CONFIG_GPIO_SPI_0_POCI_CONST;
#define CONFIG_GPIO_SPI_0_POCI 17

/* Owned by CONFIG_SPI_0 as  */
extern const uint_least8_t CONFIG_GPIO_SPI_0_PICO_CONST;
#define CONFIG_GPIO_SPI_0_PICO 16

/* The range of pins available on this device */
extern const uint_least8_t GPIO_pinLowerBound;
extern const uint_least8_t GPIO_pinUpperBound;

/* LEDs are active high */
#define CONFIG_GPIO_LED_ON  (1)
#define CONFIG_GPIO_LED_OFF (0)

#define CONFIG_LED_ON  (CONFIG_GPIO_LED_ON)
#define CONFIG_LED_OFF (CONFIG_GPIO_LED_OFF)


/*
 *  ======== RF ========
 */
#define Board_DIO_35_RFSW 0x00000023


/*
 *  ======== UART2 ========
 */

/*
 *  TX: DIO13
 *  RX: DIO12
 *  XDS110 UART
 */
extern const uint_least8_t                  CONFIG_UART2_0_CONST;
#define CONFIG_UART2_0                      0
#define CONFIG_TI_DRIVERS_UART2_COUNT       1

/*
 *  ======== SPI ========
 */

/*
 *  PICO: DIO16
 *  POCI: DIO17
 *  SCLK: DIO18
 */
extern const uint_least8_t              CONFIG_SPI_0_CONST;
#define CONFIG_SPI_0                    0
#define CONFIG_TI_DRIVERS_SPI_COUNT     1

/*
 *  ======== TRNG ========
 */

extern const uint_least8_t              CONFIG_TRNG_0_CONST;
#define CONFIG_TRNG_0                   0
#define CONFIG_TI_DRIVERS_TRNG_COUNT    1


/*
 *  ======== Watchdog ========
 */

extern const uint_least8_t                  CONFIG_WATCHDOG_0_CONST;
#define CONFIG_WATCHDOG_0                   0
#define CONFIG_TI_DRIVERS_WATCHDOG_COUNT    1

/*
 *  ======== Board_init ========
 *  Perform all required TI-Drivers initialization
 *
 *  This function should be called once at a point before any use of
 *  TI-Drivers.
 */
extern void Board_init(void);

/*
 *  ======== Board_initGeneral ========
 *  (deprecated)
 *
 *  Board_initGeneral() is defined purely for backward compatibility.
 *
 *  All new code should use Board_init() to do any required TI-Drivers
 *  initialization _and_ use <Driver>_init() for only where specific drivers
 *  are explicitly referenced by the application.  <Driver>_init() functions
 *  are idempotent.
 */
#define Board_initGeneral Board_init

#ifdef __cplusplus
}
#endif

#endif /* include guard */
