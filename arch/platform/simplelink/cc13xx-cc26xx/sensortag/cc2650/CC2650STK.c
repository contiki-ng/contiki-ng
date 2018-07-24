/*
 * Copyright (c) 2016-2018, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ====================== CC2650STK.c =========================================
 *  This file is responsible for setting up the board specific items for the
 *  CC2650STK board.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/ioc.h)
#include DeviceFamily_constructPath(driverlib/udma.h)
#include DeviceFamily_constructPath(inc/hw_ints.h)
#include DeviceFamily_constructPath(inc/hw_memmap.h)

#include "CC2650STK.h"

/*
 *  =============================== Crypto ===============================
 */
#include <ti/drivers/crypto/CryptoCC26XX.h>

CryptoCC26XX_Object cryptoCC26XXObjects[CC2650STK_CRYPTOCOUNT];

const CryptoCC26XX_HWAttrs cryptoCC26XXHWAttrs[CC2650STK_CRYPTOCOUNT] = {
    {
        .baseAddr       = CRYPTO_BASE,
        .powerMngrId    = PowerCC26XX_PERIPH_CRYPTO,
        .intNum         = INT_CRYPTO_RESULT_AVAIL_IRQ,
        .intPriority    = ~0,
    }
};

const CryptoCC26XX_Config CryptoCC26XX_config[CC2650STK_CRYPTOCOUNT] = {
    {
         .object  = &cryptoCC26XXObjects[CC2650STK_CRYPTO0],
         .hwAttrs = &cryptoCC26XXHWAttrs[CC2650STK_CRYPTO0]
    }
};

/*
 *  =============================== GPIO ===============================
 */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/gpio/GPIOCC26XX.h>

/*
 * Array of Pin configurations
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in CC2650STK.h
 * NOTE: Pins not used for interrupts should be placed at the end of the
 *       array. Callback entries can be omitted from callbacks array to
 *       reduce memory usage.
 */
GPIO_PinConfig gpioPinConfigs[] = {
    /* Input pins */
    GPIOCC26XX_DIO_15 | GPIO_DO_NOT_CONFIG,  /* Button 0 */
    GPIOCC26XX_DIO_04 | GPIO_DO_NOT_CONFIG,  /* Button 1 */

    /* Output pins */
    GPIOCC26XX_DIO_10 | GPIO_DO_NOT_CONFIG,  /* LED */

    /* SPI Flash CSN */
    GPIOCC26XX_DIO_14 | GPIO_DO_NOT_CONFIG,
};

/*
 * Array of callback function pointers
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in CC2650STK.h
 * NOTE: Pins not used for interrupts can be omitted from callbacks array to
 *       reduce memory usage (if placed at end of gpioPinConfigs array).
 */
GPIO_CallbackFxn gpioCallbackFunctions[] = {
    NULL,  /*  Button 0 */
    NULL,  /*  Button 1 */
};

const GPIOCC26XX_Config GPIOCC26XX_config = {
    .pinConfigs         = (GPIO_PinConfig *)gpioPinConfigs,
    .callbacks          = (GPIO_CallbackFxn *)gpioCallbackFunctions,
    .numberOfPinConfigs = CC2650STK_GPIOCOUNT,
    .numberOfCallbacks  = sizeof(gpioCallbackFunctions)/sizeof(GPIO_CallbackFxn),
    .intPriority        = (~0)
};

/*
 *  =============================== GPTimer ===============================
 *  Remove unused entries to reduce flash usage both in Board.c and Board.h
 */
#include <ti/drivers/timer/GPTimerCC26XX.h>

GPTimerCC26XX_Object gptimerCC26XXObjects[CC2650STK_GPTIMERCOUNT];

const GPTimerCC26XX_HWAttrs gptimerCC26xxHWAttrs[CC2650STK_GPTIMERPARTSCOUNT] = {
    { .baseAddr = GPT0_BASE, .intNum = INT_GPT0A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT0, .pinMux = GPT_PIN_0A, },
    { .baseAddr = GPT0_BASE, .intNum = INT_GPT0B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT0, .pinMux = GPT_PIN_0B, },
    { .baseAddr = GPT1_BASE, .intNum = INT_GPT1A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT1, .pinMux = GPT_PIN_1A, },
    { .baseAddr = GPT1_BASE, .intNum = INT_GPT1B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT1, .pinMux = GPT_PIN_1B, },
    { .baseAddr = GPT2_BASE, .intNum = INT_GPT2A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT2, .pinMux = GPT_PIN_2A, },
    { .baseAddr = GPT2_BASE, .intNum = INT_GPT2B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT2, .pinMux = GPT_PIN_2B, },
    { .baseAddr = GPT3_BASE, .intNum = INT_GPT3A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT3, .pinMux = GPT_PIN_3A, },
    { .baseAddr = GPT3_BASE, .intNum = INT_GPT3B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT3, .pinMux = GPT_PIN_3B, },
};

const GPTimerCC26XX_Config GPTimerCC26XX_config[CC2650STK_GPTIMERPARTSCOUNT] = {
    { &gptimerCC26XXObjects[CC2650STK_GPTIMER0], &gptimerCC26xxHWAttrs[CC2650STK_GPTIMER0A], GPT_A },
    { &gptimerCC26XXObjects[CC2650STK_GPTIMER0], &gptimerCC26xxHWAttrs[CC2650STK_GPTIMER0B], GPT_B },
    { &gptimerCC26XXObjects[CC2650STK_GPTIMER1], &gptimerCC26xxHWAttrs[CC2650STK_GPTIMER1A], GPT_A },
    { &gptimerCC26XXObjects[CC2650STK_GPTIMER1], &gptimerCC26xxHWAttrs[CC2650STK_GPTIMER1B], GPT_B },
    { &gptimerCC26XXObjects[CC2650STK_GPTIMER2], &gptimerCC26xxHWAttrs[CC2650STK_GPTIMER2A], GPT_A },
    { &gptimerCC26XXObjects[CC2650STK_GPTIMER2], &gptimerCC26xxHWAttrs[CC2650STK_GPTIMER2B], GPT_B },
    { &gptimerCC26XXObjects[CC2650STK_GPTIMER3], &gptimerCC26xxHWAttrs[CC2650STK_GPTIMER3A], GPT_A },
    { &gptimerCC26XXObjects[CC2650STK_GPTIMER3], &gptimerCC26xxHWAttrs[CC2650STK_GPTIMER3B], GPT_B },
};

/*
 *  =============================== I2C ===============================
*/
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CCC26XX.h>

#if TI_I2C_CONF_ENABLE

I2CCC26XX_Object i2cCC26xxObjects[CC2650STK_I2CCOUNT];

const I2CCC26XX_HWAttrsV1 i2cCC26xxHWAttrs[CC2650STK_I2CCOUNT] = {
#if TI_I2C_CONF_I2C0_ENABLE
    {
        .baseAddr    = I2C0_BASE,
        .powerMngrId = PowerCC26XX_PERIPH_I2C0,
        .intNum      = INT_I2C_IRQ,
        .intPriority = ~0,
        .swiPriority = 0,
        .sdaPin      = CC2650STK_I2C0_SDA0,
        .sclPin      = CC2650STK_I2C0_SCL0,
    },
#endif
};

const I2C_Config I2C_config[CC2650STK_I2CCOUNT] = {
#if TI_I2C_CONF_I2C0_ENABLE
    {
        .fxnTablePtr = &I2CCC26XX_fxnTable,
        .object      = &i2cCC26xxObjects[CC2650STK_I2C0],
        .hwAttrs     = &i2cCC26xxHWAttrs[CC2650STK_I2C0]
    },
#endif
};

const uint_least8_t I2C_count = CC2650STK_I2CCOUNT;

#endif /* TI_I2C_CONF_ENABLE */

/*
 *  =============================== NVS ===============================
 */
#include <ti/drivers/NVS.h>
#include <ti/drivers/nvs/NVSSPI25X.h>
#include <ti/drivers/nvs/NVSCC26XX.h>

#define NVS_REGIONS_BASE 0x1A000
#define SECTORSIZE       0x1000
#define REGIONSIZE       (SECTORSIZE * 4)

#if TI_NVS_CONF_ENABLE

#if TI_NVS_CONF_NVS_INTERNAL_ENABLE

/*
 * Reserve flash sectors for NVS driver use by placing an uninitialized byte
 * array at the desired flash address.
 */
#if defined(__TI_COMPILER_VERSION__)

/*
 * Place uninitialized array at NVS_REGIONS_BASE
 */
#pragma LOCATION(flashBuf, NVS_REGIONS_BASE);
#pragma NOINIT(flashBuf);
static char flashBuf[REGIONSIZE];

#elif defined(__IAR_SYSTEMS_ICC__)

/*
 * Place uninitialized array at NVS_REGIONS_BASE
 */
static __no_init char flashBuf[REGIONSIZE] @ NVS_REGIONS_BASE;

#elif defined(__GNUC__)

/*
 * Place the flash buffers in the .nvs section created in the gcc linker file.
 * The .nvs section enforces alignment on a sector boundary but may
 * be placed anywhere in flash memory.  If desired the .nvs section can be set
 * to a fixed address by changing the following in the gcc linker file:
 *
 * .nvs (FIXED_FLASH_ADDR) (NOLOAD) : AT (FIXED_FLASH_ADDR) {
 *      *(.nvs)
 * } > REGION_TEXT
 */
__attribute__ ((section (".nvs")))
static char flashBuf[REGIONSIZE];

#endif

NVSCC26XX_Object nvsCC26xxObjects[1];

/* Hardware attributes for NVS */
const NVSCC26XX_HWAttrs nvsCC26xxHWAttrs[1] = {
    {
        .regionBase = (void *)flashBuf,
        .regionSize = REGIONSIZE,
    },
};

#endif /* TI_NVS_CONF_NVS_INTERNAL_ENABLE */

#if TI_NVS_CONF_NVS_EXTERNAL_ENABLE

#define SPISECTORSIZE    0x1000
#define SPIREGIONSIZE    (SPISECTORSIZE * 32)
#define VERIFYBUFSIZE    64

static uint8_t verifyBuf[VERIFYBUFSIZE];

/* Allocate objects for NVS and NVS SPI */
NVSSPI25X_Object nvsSPI25XObjects[1];

/* Hardware attributes for NVS SPI */
const NVSSPI25X_HWAttrs nvsSPI25XHWAttrs[1] = {
    {
        .regionBaseOffset = 0,
        .regionSize = SPIREGIONSIZE,
        .sectorSize = SPISECTORSIZE,
        .verifyBuf = verifyBuf,
        .verifyBufSize = VERIFYBUFSIZE,
        .spiHandle = NULL,
        .spiIndex = 0,
        .spiBitRate = 4000000,
        .spiCsnGpioIndex = CC2650STK_GPIO_SPI_FLASH_CS,
    },
};

#endif /* Board_EXCLUDE_NVS_EXTERNAL_FLASH */

/* NVS Region index 0 and 1 refer to NVS and NVS SPI respectively */
const NVS_Config NVS_config[CC2650STK_NVSCOUNT] = {
#if TI_NVS_CONF_NVS_INTERNAL_ENABLE
    {
        .fxnTablePtr = &NVSCC26XX_fxnTable,
        .object = &nvsCC26xxObjects[0],
        .hwAttrs = &nvsCC26xxHWAttrs[0],
    },
#endif
#if TI_NVS_CONF_NVS_EXTERNAL_ENABLE
    {
        .fxnTablePtr = &NVSSPI25X_fxnTable,
        .object = &nvsSPI25XObjects[0],
        .hwAttrs = &nvsSPI25XHWAttrs[0],
    },
#endif
};

const uint_least8_t NVS_count = CC2650STK_NVSCOUNT;

#endif /* TI_NVS_CONF_ENABLE */

/*
 *  =============================== PDM ===============================
*/
#include <ti/drivers/pdm/PDMCC26XX.h>
#include <ti/drivers/pdm/PDMCC26XX_util.h>

PDMCC26XX_Object        pdmCC26XXObjects[CC2650STK_PDMCOUNT];
PDMCC26XX_I2S_Object    pdmCC26XXI2SObjects[CC2650STK_PDMCOUNT];

const PDMCC26XX_HWAttrs pdmCC26XXHWAttrs[CC2650STK_PDMCOUNT] = {
    {
        .micPower     = CC2650STK_MIC_POWER,
        .taskPriority = 2
    }
};

const PDMCC26XX_Config PDMCC26XX_config[CC2650STK_PDMCOUNT] = {
    {
        .object  = &pdmCC26XXObjects[CC2650STK_PDM0],
        .hwAttrs = &pdmCC26XXHWAttrs[CC2650STK_PDM0]
    }
};

const PDMCC26XX_I2S_HWAttrs pdmC26XXI2SHWAttrs[CC2650STK_PDMCOUNT] = {
    {
        .baseAddr       = I2S0_BASE,
        .intNum         = INT_I2S_IRQ,
        .powerMngrId    = PowerCC26XX_PERIPH_I2S,
        .intPriority    = ~0,
        .mclkPin        = PIN_UNASSIGNED,
        .bclkPin        = CC2650STK_AUDIO_CLK,
        .wclkPin        = PIN_UNASSIGNED,
        .ad0Pin         = CC2650STK_AUDIO_DI,
    }
};

const PDMCC26XX_I2S_Config PDMCC26XX_I2S_config[CC2650STK_PDMCOUNT] = {
    {
        .object  = &pdmCC26XXI2SObjects[CC2650STK_PDM0],
        .hwAttrs = &pdmC26XXI2SHWAttrs[CC2650STK_PDM0]
    }
};

/*
 *  =============================== PIN ===============================
 */
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>

const PIN_Config BoardGpioInitTable[] = {

    CC2650STK_PIN_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,           /* LED initially off */
    CC2650STK_KEY_LEFT | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,          /* Button is active low */
    CC2650STK_KEY_RIGHT | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,         /* Button is active low */
    CC2650STK_RELAY | PIN_INPUT_EN | PIN_PULLDOWN | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,           /* Relay is active high */
    CC2650STK_MPU_INT | PIN_INPUT_EN | PIN_PULLDOWN | PIN_IRQ_NEGEDGE | PIN_HYSTERESIS,           /* MPU_INT is active low */
    CC2650STK_TMP_RDY | PIN_INPUT_EN | PIN_PULLUP | PIN_HYSTERESIS,                               /* TMP_RDY is active high */
    CC2650STK_BUZZER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,         /* Buzzer initially off */
    CC2650STK_MPU_POWER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,     /* MPU initially on */
    CC2650STK_MIC_POWER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,      /* MIC initially off */
    CC2650STK_SPI_FLASH_CS | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MIN,  /* External flash chip select */
    CC2650STK_SPI_DEVPK_CS | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,   /* DevPack chip select */
    CC2650STK_AUDIO_DI | PIN_INPUT_EN | PIN_PULLDOWN,                                             /* Audio DI */
    CC2650STK_AUDIODO | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MIN,       /* Audio data out */
    CC2650STK_AUDIO_CLK | PIN_INPUT_EN | PIN_PULLDOWN,                                            /* DevPack */
    CC2650STK_DP2 | PIN_INPUT_EN | PIN_PULLDOWN,                                                  /* DevPack */
    CC2650STK_DP1 | PIN_INPUT_EN | PIN_PULLDOWN,                                                  /* DevPack */
    CC2650STK_DP0 | PIN_INPUT_EN | PIN_PULLDOWN,                                                  /* DevPack */
    CC2650STK_DP3 | PIN_INPUT_EN | PIN_PULLDOWN,                                                  /* DevPack */
    CC2650STK_UART_TX | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL,                        /* DevPack */
    CC2650STK_UART_RX | PIN_INPUT_EN | PIN_PULLDOWN,                                              /* DevPack */
    CC2650STK_DEVPK_ID | PIN_INPUT_EN | PIN_NOPULL,                                               /* Device pack ID - external PU */
    CC2650STK_SPI0_MOSI | PIN_INPUT_EN | PIN_PULLDOWN,                                            /* SPI master out - slave in */
    CC2650STK_SPI0_MISO | PIN_INPUT_EN | PIN_PULLDOWN,                                            /* SPI master in - slave out */
    CC2650STK_SPI0_CLK | PIN_INPUT_EN | PIN_PULLDOWN,                                             /* SPI clock */

    PIN_TERMINATE
};

const PINCC26XX_HWAttrs PINCC26XX_hwAttrs = {
    .intPriority = ~0,
    .swiPriority = 0
};

/*
 *  =============================== Power ===============================
 */
const PowerCC26XX_Config PowerCC26XX_config = {
    .policyInitFxn      = NULL,
    .policyFxn          = &PowerCC26XX_standbyPolicy,
    .calibrateFxn       = &PowerCC26XX_calibrate,
    .enablePolicy       = true,
    .calibrateRCOSC_LF  = true,
    .calibrateRCOSC_HF  = true,
};

/*
 *  =============================== PWM ===============================
 *  Remove unused entries to reduce flash usage both in Board.c and Board.h
 */
#include <ti/drivers/PWM.h>
#include <ti/drivers/pwm/PWMTimerCC26XX.h>

PWMTimerCC26XX_Object pwmtimerCC26xxObjects[CC2650STK_PWMCOUNT];

const PWMTimerCC26XX_HwAttrs pwmtimerCC26xxHWAttrs[CC2650STK_PWMCOUNT] = {
    { .pwmPin = CC2650STK_PWMPIN0, .gpTimerUnit = CC2650STK_GPTIMER0A },
    { .pwmPin = CC2650STK_PWMPIN1, .gpTimerUnit = CC2650STK_GPTIMER0B },
    { .pwmPin = CC2650STK_PWMPIN2, .gpTimerUnit = CC2650STK_GPTIMER1A },
    { .pwmPin = CC2650STK_PWMPIN3, .gpTimerUnit = CC2650STK_GPTIMER1B },
    { .pwmPin = CC2650STK_PWMPIN4, .gpTimerUnit = CC2650STK_GPTIMER2A },
    { .pwmPin = CC2650STK_PWMPIN5, .gpTimerUnit = CC2650STK_GPTIMER2B },
    { .pwmPin = CC2650STK_PWMPIN6, .gpTimerUnit = CC2650STK_GPTIMER3A },
    { .pwmPin = CC2650STK_PWMPIN7, .gpTimerUnit = CC2650STK_GPTIMER3B },
};

const PWM_Config PWM_config[CC2650STK_PWMCOUNT] = {
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC2650STK_PWM0], &pwmtimerCC26xxHWAttrs[CC2650STK_PWM0] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC2650STK_PWM1], &pwmtimerCC26xxHWAttrs[CC2650STK_PWM1] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC2650STK_PWM2], &pwmtimerCC26xxHWAttrs[CC2650STK_PWM2] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC2650STK_PWM3], &pwmtimerCC26xxHWAttrs[CC2650STK_PWM3] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC2650STK_PWM4], &pwmtimerCC26xxHWAttrs[CC2650STK_PWM4] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC2650STK_PWM5], &pwmtimerCC26xxHWAttrs[CC2650STK_PWM5] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC2650STK_PWM6], &pwmtimerCC26xxHWAttrs[CC2650STK_PWM6] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC2650STK_PWM7], &pwmtimerCC26xxHWAttrs[CC2650STK_PWM7] },
};

const uint_least8_t PWM_count = CC2650STK_PWMCOUNT;

/*
 *  =============================== RF Driver ===============================
 */
#include <ti/drivers/rf/RF.h>

const RFCC26XX_HWAttrsV2 RFCC26XX_hwAttrs = {
    .hwiPriority        = ~0,       /* Lowest HWI priority */
    .swiPriority        = 0,        /* Lowest SWI priority */
    .xoscHfAlwaysNeeded = true,     /* Keep XOSC dependency while in stanby */
    .globalCallback     = NULL,     /* No board specific callback */
    .globalEventMask    = 0         /* No events subscribed to */
};

/*
 *  =============================== SPI DMA ===============================
 */
#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPICC26XXDMA.h>

#if TI_SPI_CONF_ENABLE

SPICC26XXDMA_Object spiCC26XXDMAObjects[CC2650STK_SPICOUNT];

const SPICC26XXDMA_HWAttrsV1 spiCC26XXDMAHWAttrs[CC2650STK_SPICOUNT] = {
#if TI_SPI_CONF_SPI0_ENABLE
    {
        .baseAddr           = SSI0_BASE,
        .intNum             = INT_SSI0_COMB,
        .intPriority        = ~0,
        .swiPriority        = 0,
        .powerMngrId        = PowerCC26XX_PERIPH_SSI0,
        .defaultTxBufValue  = 0,
        .rxChannelBitMask   = 1<<UDMA_CHAN_SSI0_RX,
        .txChannelBitMask   = 1<<UDMA_CHAN_SSI0_TX,
        .mosiPin            = CC2650STK_SPI0_MOSI,
        .misoPin            = CC2650STK_SPI0_MISO,
        .clkPin             = CC2650STK_SPI0_CLK,
        .csnPin             = CC2650STK_SPI0_CSN,
        .minDmaTransferSize = 10
    },
#endif
#if TI_SPI_CONF_SPI1_ENABLE
    {
        .baseAddr           = SSI1_BASE,
        .intNum             = INT_SSI1_COMB,
        .intPriority        = ~0,
        .swiPriority        = 0,
        .powerMngrId        = PowerCC26XX_PERIPH_SSI1,
        .defaultTxBufValue  = 0,
        .rxChannelBitMask   = 1<<UDMA_CHAN_SSI1_RX,
        .txChannelBitMask   = 1<<UDMA_CHAN_SSI1_TX,
        .mosiPin            = CC2650STK_SPI1_MOSI,
        .misoPin            = CC2650STK_SPI1_MISO,
        .clkPin             = CC2650STK_SPI1_CLK,
        .csnPin             = CC2650STK_SPI1_CSN,
        .minDmaTransferSize = 10
    },
#endif
};

const SPI_Config SPI_config[CC2650STK_SPICOUNT] = {
#if TI_SPI_CONF_SPI0_ENABLE
    {
         .fxnTablePtr = &SPICC26XXDMA_fxnTable,
         .object      = &spiCC26XXDMAObjects[CC2650STK_SPI0],
         .hwAttrs     = &spiCC26XXDMAHWAttrs[CC2650STK_SPI0]
    },
#endif
#if TI_SPI_CONF_SPI1_ENABLE
    {
         .fxnTablePtr = &SPICC26XXDMA_fxnTable,
         .object      = &spiCC26XXDMAObjects[CC2650STK_SPI1],
         .hwAttrs     = &spiCC26XXDMAHWAttrs[CC2650STK_SPI1]
    },
#endif
};

const uint_least8_t SPI_count = CC2650STK_SPICOUNT;

#endif /* TI_SPI_CONF_ENABLE */


/*
 *  =============================== TRNG ===============================
 */
#include <ti/drivers/TRNG.h>
#include <ti/drivers/trng/TRNGCC26X0.h>

TRNGCC26X0_Object trngCC26X0Object[CC2650STK_TRNGCOUNT];

const TRNGCC26X0_HWAttrs trngCC26X0HWAttrs[CC2650STK_TRNGCOUNT] = {
    {
         .swiPriority = 0,
         .intPriority = ~0,
    }
};

const TRNG_Config TRNG_config[] = {
    { &trngCC26X0Object[0], &trngCC26X0HWAttrs[0] },
};

const uint8_t TRNG_count = CC2650STK_TRNGCOUNT;


/*
 *  =============================== UART ===============================
 */
#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTCC26XX.h>

#if TI_UART_CONF_ENABLE

UARTCC26XX_Object uartCC26XXObjects[CC2650STK_UARTCOUNT];

uint8_t uartCC26XXRingBuffer[CC2650STK_UARTCOUNT][32];

const UARTCC26XX_HWAttrsV2 uartCC26XXHWAttrs[CC2650STK_UARTCOUNT] = {
#if TI_UART_CONF_UART0_ENABLE
    {
        .baseAddr       = UART0_BASE,
        .powerMngrId    = PowerCC26XX_PERIPH_UART0,
        .intNum         = INT_UART0_COMB,
        .intPriority    = ~0,
        .swiPriority    = 0,
        .txPin          = CC2650STK_UART_TX,
        .rxPin          = CC2650STK_UART_RX,
        .ctsPin         = PIN_UNASSIGNED,
        .rtsPin         = PIN_UNASSIGNED,
        .ringBufPtr     = uartCC26XXRingBuffer[CC2650STK_UART0],
        .ringBufSize    = sizeof(uartCC26XXRingBuffer[CC2650STK_UART0]),
        .txIntFifoThr   = UARTCC26XX_FIFO_THRESHOLD_1_8,
        .rxIntFifoThr   = UARTCC26XX_FIFO_THRESHOLD_4_8,
        .errorFxn       = NULL
    },
#endif
};

const UART_Config UART_config[CC2650STK_UARTCOUNT] = {
#if TI_UART_CONF_UART0_ENABLE
    {
        .fxnTablePtr = &UARTCC26XX_fxnTable,
        .object      = &uartCC26XXObjects[CC2650STK_UART0],
        .hwAttrs     = &uartCC26XXHWAttrs[CC2650STK_UART0]
    },
#endif
};

const uint_least8_t UART_count = CC2650STK_UARTCOUNT;

#endif /* TI_UART_CONF_ENABLE */

/*
 *  =============================== UDMA ===============================
 */
#include <ti/drivers/dma/UDMACC26XX.h>

UDMACC26XX_Object udmaObjects[CC2650STK_UDMACOUNT];

const UDMACC26XX_HWAttrs udmaHWAttrs[CC2650STK_UDMACOUNT] = {
    {
        .baseAddr    = UDMA0_BASE,
        .powerMngrId = PowerCC26XX_PERIPH_UDMA,
        .intNum      = INT_DMA_ERR,
        .intPriority = ~0
    }
};

const UDMACC26XX_Config UDMACC26XX_config[CC2650STK_UDMACOUNT] = {
    {
         .object  = &udmaObjects[CC2650STK_UDMA0],
         .hwAttrs = &udmaHWAttrs[CC2650STK_UDMA0]
    },
};

/*
 *  =============================== Watchdog ===============================
 */
#include <ti/drivers/Watchdog.h>
#include <ti/drivers/watchdog/WatchdogCC26XX.h>

WatchdogCC26XX_Object watchdogCC26XXObjects[CC2650STK_WATCHDOGCOUNT];

const WatchdogCC26XX_HWAttrs watchdogCC26XXHWAttrs[CC2650STK_WATCHDOGCOUNT] = {
    {
        .baseAddr    = WDT_BASE,
        .reloadValue = 1000 /* Reload value in milliseconds */
    },
};

const Watchdog_Config Watchdog_config[CC2650STK_WATCHDOGCOUNT] = {
    {
        .fxnTablePtr = &WatchdogCC26XX_fxnTable,
        .object      = &watchdogCC26XXObjects[CC2650STK_WATCHDOG0],
        .hwAttrs     = &watchdogCC26XXHWAttrs[CC2650STK_WATCHDOG0]
    },
};

const uint_least8_t Watchdog_count = CC2650STK_WATCHDOGCOUNT;

/*
 *  Board-specific initialization function to disable external flash.
 *  This function is defined in the file CC2650STK_fxns.c
 */
extern void Board_initHook(void);

/*
 *  ======== CC2650STK_initGeneral ========
 */
void CC2650STK_initGeneral(void)
{
    Power_init();

    if ( PIN_init(BoardGpioInitTable) != PIN_SUCCESS) {
        /* Error with PIN_init */
        while (1);
    }

    /* Perform board-specific initialization */
    Board_initHook();
}
