#ifndef VSNDRIVERS_CONF_H_
#define VSNDRIVERS_CONF_H_


#include <stdint.h>


/* The USART to which the standard C printf(), putchar(), ... will print to.
 * Note that defined USART needs to be configured by the user for this to work.
 * If the STDOUT buffering is enabled data will be written to USART if
 * the buffer is full or a carriage return (\n) is printed. STDOUT buffering
 * can be turned off with setvbuf(stdout, NULL, _IONBF, 0).
 */
#define STDOUT_USART USART1

/* The USART from which the standard C scanf(), getchar(), ... will read from.
 * Note that defined USART needs to be configured by the user for this to work.
 * If the STDIN buffering is enabled data will be returned if the buffer is full.
 * STDIN buffering can be turned off with setvbuf(stdin, NULL, _IONBF, 0).
 */
#define STDIN_USART USART1
/* Enables STDIN echo where everything received to the STDIN_USART is echoed */
//#define STDIN_USART_ECHO

/* !YOU SHOULD NOT CHANGE THIS! Indicator led functions and power management rely on this settings */
/* Set SysTick interrupt period = 1 second / SYS_TICK_DIV */
#define SYS_TICK_DIV	1000    /* SysTick interrupt set to millisecond */
#define SD_DMA_MODE
/* ------------------------------ Driver configuration ------------------------------ */
/* ------------------------------ FRAM driver configuration ------------------------- */
/* Uncomment ONE of the following three lines to select the FRAM driver mode,
 * if none is defined the driver defaults to DMA mode */
//#define NVRAM_DRIVER_MODE_POLLING
#define NVRAM_DRIVER_MODE_INTERRUPT
//#define NVRAM_DRIVER_MODE_DMA

/* ------------------------------ I2C driver configuration ------------------------- */
/* Uncomment the the corresponding line to enable I2C driver. The driver can operate
 * only in DMA mode, because I2C1 shares the DMA channels with USART2 and I2C1 shares
 * the DMA channels with USART1, the USART driver has to be configured in interuppt mode
 * when using I2C */
//#define I2C1_DMA_MODE
//#define I2C2_DMA_MODE

/* ------------------------------ SPI driver configuration --------------------------- */
/* Uncomment ONE of the following three lines to select the mode
 * in which the SPI driver should be working, if none is defined
 * the driver defaults to DMA mode */
//#define SPI1_NEW_DRIVER_MODE_INTERRUPT	1
//#define SPI2_NEW_DRIVER_MODE_INTERRUPT	1
#define SPI1_NEW_DRIVER_MODE_POLLING
#define SPI2_NEW_DRIVER_MODE_POLLING
//#define SPI1_NEW_DRIVER_MODE_DMA
//#define SPI2_NEW_DRIVER_MODE_DMA
//#define SPI1_DRIVER_MODE_DMA
//#define SPI1_DRIVER_MODE_INTERRUPT
//#define SPI2_DRIVER_MODE_INTERRUPT
//#define SPI1_DRIVER_MODE_POLLING
//#define SPI2_DRIVER_MODE_POLLING
//#define SPI3_DRIVER_MODE_POLLING
//#define SPI3_NEW_DRIVER_MODE_POLLING


/* ------------------------------ USART driver configuration ------------------------ */
/* Uncomment ONE of the following lines for each USART to select
 * the mode in which the USART driver should be working, if none is defined
 * the driver defaults to interrupt mode
 */
#define USART1_DMA_MODE
//#define USART1_INTERRUPT_MODE
//#define USART2_DMA_MODE
#define USART2_INTERRUPT_MODE
//#define USART3_DMA_MODE
#define USART3_INTERRUPT_MODE
//#define UART4_DMA_MODE
#define UART4_INTERRUPT_MODE

/* Set the desired USART buffer sizes, if nothing is defined here
 * buffer size defaults to 128 bytes */
/* RX buffer size for USARTs, USARTx_RX_BUFFER_LEN - 1 chars can be stored */
#define USART1_RX_BUFFER_LEN  1280
#define USART2_RX_BUFFER_LEN  128
#define USART3_RX_BUFFER_LEN  128
#define UART4_RX_BUFFER_LEN   128

/* TX buffer size for USARTs, USARTx_TX_BUFFER_LEN - 1 chars can be stored */
#define USART1_TX_BUFFER_LEN  1280
#define USART2_TX_BUFFER_LEN  128
#define USART3_TX_BUFFER_LEN  128
#define UART4_TX_BUFFER_LEN   128

//#define I2C1_DMA_MODE
/* ------------------------------ ZigBit driver configuration ----------------------- */
/* Set the desired message buffer size, min size is 2,
 * the buffer has to be declared by the user application check vsnZigbit_init()
 * FIXME move this value to vsnZigbit_init() function
 */
//#define ZIGBIT_MSG_BUFFER_SIZE  5

/* Function prototypes */
void vsnDriversConf_nvic(void);
//uint16_t prescaler_calc(uint32_t spiSpeed);




/* ------------------- CC 1101 / 2500 driver configuration ----------------- */

/**
 * Physical location where the radio chip is connected.
 * Only 1 value has to be set to 1 from these:
 */

//#define VSNCCRADIO_868 0





#endif //VSNDRIVERS_CONF_H_
