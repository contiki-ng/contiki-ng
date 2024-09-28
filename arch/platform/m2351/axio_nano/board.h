#pragma once

#include "NuMicro.h"

#define SPI_CONF_CONTROLLER_COUNT 0

#define EXT_FLASH_SPI_CONTROLLER    0  /* SPI0 */

#define EXT_FLASH_SPI_PORT_SCK       0 /* PA */
#define EXT_FLASH_SPI_PORT_MOSI      0 /* PA */
#define EXT_FLASH_SPI_PORT_MISO      0 /* PA */
#define EXT_FLASH_SPI_PORT_CS        0 /* PA */

#define EXT_FLASH_SPI_PIN_SCK       2  /* PA2 */
#define EXT_FLASH_SPI_PIN_MOSI      0  /* PA0 */
#define EXT_FLASH_SPI_PIN_MISO      1  /* PA1 */
#define EXT_FLASH_SPI_PIN_CS        3  /* PA3 */

#define EXT_FLASH_MID               0xc2
#define EXT_FLASH_DEVICE_ID         0x15

#define EXT_FLASH_PROGRAM_PAGE_SIZE 256
#define EXT_FLASH_ERASE_SECTOR_SIZE 4096

void platform_init_stage_secure(void);
