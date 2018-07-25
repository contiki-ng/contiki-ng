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
 *  ====================== CC1350DK_7XD.c ===================================
 *  This file is responsible for setting up the board specific items for the
 *  CC1350DK_7XD board.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/ioc.h)
#include DeviceFamily_constructPath(driverlib/udma.h)
#include DeviceFamily_constructPath(inc/hw_ints.h)
#include DeviceFamily_constructPath(inc/hw_memmap.h)

#include "CC1350DK_7XD.h"

/*
 *  =============================== ADCBuf ===============================
 */
#include <ti/drivers/ADCBuf.h>
#include <ti/drivers/adcbuf/ADCBufCC26XX.h>

ADCBufCC26XX_Object adcBufCC26xxObjects[CC1350DK_7XD_ADCBUFCOUNT];

/*
 *  This table converts a virtual adc channel into a dio and internal analogue
 *  input signal. This table is necessary for the functioning of the adcBuf
 *  driver. Comment out unused entries to save flash. Dio and internal signal
 *  pairs are hardwired. Do not remap them in the table. You may reorder entire
 *  entries. The mapping of dio and internal signals is package dependent.
 */
const ADCBufCC26XX_AdcChannelLutEntry ADCBufCC26XX_adcChannelLut[CC1350DK_7XD_ADCBUF0CHANNELCOUNT] = {
    {CC1350DK_7XD_ALS_OUT, ADC_COMPB_IN_AUXIO7},
    {PIN_UNASSIGNED, ADC_COMPB_IN_DCOUPL},
    {PIN_UNASSIGNED, ADC_COMPB_IN_VSS},
    {PIN_UNASSIGNED, ADC_COMPB_IN_VDDS},
};

const ADCBufCC26XX_HWAttrs adcBufCC26xxHWAttrs[CC1350DK_7XD_ADCBUFCOUNT] = {
    {
        .intPriority       = ~0,
        .swiPriority       = 0,
        .adcChannelLut     = ADCBufCC26XX_adcChannelLut,
        .gpTimerUnit       = CC1350DK_7XD_GPTIMER0A,
        .gptDMAChannelMask = 1 << UDMA_CHAN_TIMER0_A,
    }
};

const ADCBuf_Config ADCBuf_config[CC1350DK_7XD_ADCBUFCOUNT] = {
    {
        &ADCBufCC26XX_fxnTable,
        &adcBufCC26xxObjects[CC1350DK_7XD_ADCBUF0],
        &adcBufCC26xxHWAttrs[CC1350DK_7XD_ADCBUF0]
    },
};

const uint_least8_t ADCBuf_count = CC1350DK_7XD_ADCBUFCOUNT;

/*
 *  =============================== ADC ===============================
 */
#include <ti/drivers/ADC.h>
#include <ti/drivers/adc/ADCCC26XX.h>

ADCCC26XX_Object adcCC26xxObjects[CC1350DK_7XD_ADCCOUNT];

const ADCCC26XX_HWAttrs adcCC26xxHWAttrs[CC1350DK_7XD_ADCCOUNT] = {
    {
        .adcDIO              = CC1350DK_7XD_DIO23_ANALOG,
        .adcCompBInput       = ADC_COMPB_IN_AUXIO7,
        .refSource           = ADCCC26XX_FIXED_REFERENCE,
        .samplingDuration    = ADCCC26XX_SAMPLING_DURATION_2P7_US,
        .inputScalingEnabled = true,
        .triggerSource       = ADCCC26XX_TRIGGER_MANUAL,
        .returnAdjustedVal   = false
    },
    {
        .adcDIO              = PIN_UNASSIGNED,
        .adcCompBInput       = ADC_COMPB_IN_DCOUPL,
        .refSource           = ADCCC26XX_FIXED_REFERENCE,
        .samplingDuration    = ADCCC26XX_SAMPLING_DURATION_2P7_US,
        .inputScalingEnabled = true,
        .triggerSource       = ADCCC26XX_TRIGGER_MANUAL,
        .returnAdjustedVal   = false
    },
    {
        .adcDIO              = PIN_UNASSIGNED,
        .adcCompBInput       = ADC_COMPB_IN_VSS,
        .refSource           = ADCCC26XX_FIXED_REFERENCE,
        .samplingDuration    = ADCCC26XX_SAMPLING_DURATION_2P7_US,
        .inputScalingEnabled = true,
        .triggerSource       = ADCCC26XX_TRIGGER_MANUAL,
        .returnAdjustedVal   = false
    },
    {
        .adcDIO              = PIN_UNASSIGNED,
        .adcCompBInput       = ADC_COMPB_IN_VDDS,
        .refSource           = ADCCC26XX_FIXED_REFERENCE,
        .samplingDuration    = ADCCC26XX_SAMPLING_DURATION_2P7_US,
        .inputScalingEnabled = true,
        .triggerSource       = ADCCC26XX_TRIGGER_MANUAL,
        .returnAdjustedVal   = false
    }
};

const ADC_Config ADC_config[CC1350DK_7XD_ADCCOUNT] = {
    {&ADCCC26XX_fxnTable, &adcCC26xxObjects[CC1350DK_7XD_ADCALS], &adcCC26xxHWAttrs[CC1350DK_7XD_ADCALS]},
    {&ADCCC26XX_fxnTable, &adcCC26xxObjects[CC1350DK_7XD_ADCDCOUPL], &adcCC26xxHWAttrs[CC1350DK_7XD_ADCDCOUPL]},
    {&ADCCC26XX_fxnTable, &adcCC26xxObjects[CC1350DK_7XD_ADCVSS], &adcCC26xxHWAttrs[CC1350DK_7XD_ADCVSS]},
    {&ADCCC26XX_fxnTable, &adcCC26xxObjects[CC1350DK_7XD_ADCVDDS], &adcCC26xxHWAttrs[CC1350DK_7XD_ADCVDDS]},
};

const uint_least8_t ADC_count = CC1350DK_7XD_ADCCOUNT;

/*
 *  =============================== Crypto ===============================
 */
#include <ti/drivers/crypto/CryptoCC26XX.h>

CryptoCC26XX_Object cryptoCC26XXObjects[CC1350DK_7XD_CRYPTOCOUNT];

const CryptoCC26XX_HWAttrs cryptoCC26XXHWAttrs[CC1350DK_7XD_CRYPTOCOUNT] = {
    {
        .baseAddr       = CRYPTO_BASE,
        .powerMngrId    = PowerCC26XX_PERIPH_CRYPTO,
        .intNum         = INT_CRYPTO_RESULT_AVAIL_IRQ,
        .intPriority    = ~0,
    }
};

const CryptoCC26XX_Config CryptoCC26XX_config[CC1350DK_7XD_CRYPTOCOUNT] = {
    {
         .object  = &cryptoCC26XXObjects[CC1350DK_7XD_CRYPTO0],
         .hwAttrs = &cryptoCC26XXHWAttrs[CC1350DK_7XD_CRYPTO0]
    },
};

/*
 *  =============================== GPIO ===============================
 */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/gpio/GPIOCC26XX.h>

/*
 * Array of Pin configurations
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in CC1350DK_7XD.h
 * NOTE: Pins not used for interrupts should be placed at the end of the
 *       array. Callback entries can be omitted from callbacks array to
 *       reduce memory usage.
 */
GPIO_PinConfig gpioPinConfigs[] = {
    /* Input pins */
    GPIOCC26XX_DIO_11 | GPIO_DO_NOT_CONFIG,  /* Key Select */
    GPIOCC26XX_DIO_19 | GPIO_DO_NOT_CONFIG,  /* Key Up */
    GPIOCC26XX_DIO_12 | GPIO_DO_NOT_CONFIG,  /* Key Down */
    GPIOCC26XX_DIO_15 | GPIO_DO_NOT_CONFIG,  /* Key Left */
    GPIOCC26XX_DIO_18 | GPIO_DO_NOT_CONFIG,  /* Key Right */

    GPIOCC26XX_DIO_15 | GPIO_DO_NOT_CONFIG,  /* CC1350DK_7XD_SPI_MASTER_READY */
    GPIOCC26XX_DIO_21 | GPIO_DO_NOT_CONFIG,  /* CC1350DK_7XD_SPI_SLAVE_READY */

    /* Output pins */
    GPIOCC26XX_DIO_25 | GPIO_DO_NOT_CONFIG,  /* LED 1 */
    GPIOCC26XX_DIO_27 | GPIO_DO_NOT_CONFIG,  /* LED 2 */
    GPIOCC26XX_DIO_07 | GPIO_DO_NOT_CONFIG,  /* LED 3 */
    GPIOCC26XX_DIO_06 | GPIO_DO_NOT_CONFIG,  /* LED 4 */

    /* SDCARD */
    GPIOCC26XX_DIO_30 | GPIO_DO_NOT_CONFIG,  /* SPI chip select */

    /* Accelerometer */
    GPIOCC26XX_DIO_24 | GPIO_DO_NOT_CONFIG,  /* SPI chip select */
};

/*
 * Array of callback function pointers
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in CC2650_LAUNCH.h
 * NOTE: Pins not used for interrupts can be omitted from callbacks array to
 *       reduce memory usage (if placed at end of gpioPinConfigs array).
 */
GPIO_CallbackFxn gpioCallbackFunctions[] = {
    NULL,  /* Button 0 */
    NULL,  /* Button 1 */
    NULL,  /* CC1350DK_7XD_SPI_MASTER_READY */
    NULL,  /* CC1350DK_7XD_SPI_SLAVE_READY */
};

const GPIOCC26XX_Config GPIOCC26XX_config = {
    .pinConfigs         = (GPIO_PinConfig *)gpioPinConfigs,
    .callbacks          = (GPIO_CallbackFxn *)gpioCallbackFunctions,
    .numberOfPinConfigs = CC1350DK_7XD_GPIOCOUNT,
    .numberOfCallbacks  = sizeof(gpioCallbackFunctions)/sizeof(GPIO_CallbackFxn),
    .intPriority        = (~0)
};

/*
 *  =============================== GPTimer ===============================
 *  Remove unused entries to reduce flash usage both in Board.c and Board.h
 */
#include <ti/drivers/timer/GPTimerCC26XX.h>

GPTimerCC26XX_Object gptimerCC26XXObjects[CC1350DK_7XD_GPTIMERCOUNT];

const GPTimerCC26XX_HWAttrs gptimerCC26xxHWAttrs[CC1350DK_7XD_GPTIMERPARTSCOUNT] = {
    { .baseAddr = GPT0_BASE, .intNum = INT_GPT0A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT0, .pinMux = GPT_PIN_0A, },
    { .baseAddr = GPT0_BASE, .intNum = INT_GPT0B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT0, .pinMux = GPT_PIN_0B, },
    { .baseAddr = GPT1_BASE, .intNum = INT_GPT1A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT1, .pinMux = GPT_PIN_1A, },
    { .baseAddr = GPT1_BASE, .intNum = INT_GPT1B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT1, .pinMux = GPT_PIN_1B, },
    { .baseAddr = GPT2_BASE, .intNum = INT_GPT2A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT2, .pinMux = GPT_PIN_2A, },
    { .baseAddr = GPT2_BASE, .intNum = INT_GPT2B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT2, .pinMux = GPT_PIN_2B, },
    { .baseAddr = GPT3_BASE, .intNum = INT_GPT3A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT3, .pinMux = GPT_PIN_3A, },
    { .baseAddr = GPT3_BASE, .intNum = INT_GPT3B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT3, .pinMux = GPT_PIN_3B, },
};

const GPTimerCC26XX_Config GPTimerCC26XX_config[CC1350DK_7XD_GPTIMERPARTSCOUNT] = {
    { &gptimerCC26XXObjects[CC1350DK_7XD_GPTIMER0], &gptimerCC26xxHWAttrs[CC1350DK_7XD_GPTIMER0A], GPT_A },
    { &gptimerCC26XXObjects[CC1350DK_7XD_GPTIMER0], &gptimerCC26xxHWAttrs[CC1350DK_7XD_GPTIMER0B], GPT_B },
    { &gptimerCC26XXObjects[CC1350DK_7XD_GPTIMER1], &gptimerCC26xxHWAttrs[CC1350DK_7XD_GPTIMER1A], GPT_A },
    { &gptimerCC26XXObjects[CC1350DK_7XD_GPTIMER1], &gptimerCC26xxHWAttrs[CC1350DK_7XD_GPTIMER1B], GPT_B },
    { &gptimerCC26XXObjects[CC1350DK_7XD_GPTIMER2], &gptimerCC26xxHWAttrs[CC1350DK_7XD_GPTIMER2A], GPT_A },
    { &gptimerCC26XXObjects[CC1350DK_7XD_GPTIMER2], &gptimerCC26xxHWAttrs[CC1350DK_7XD_GPTIMER2B], GPT_B },
    { &gptimerCC26XXObjects[CC1350DK_7XD_GPTIMER3], &gptimerCC26xxHWAttrs[CC1350DK_7XD_GPTIMER3A], GPT_A },
    { &gptimerCC26XXObjects[CC1350DK_7XD_GPTIMER3], &gptimerCC26xxHWAttrs[CC1350DK_7XD_GPTIMER3B], GPT_B },
};

/*
 *  =============================== I2C ===============================
*/
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CCC26XX.h>

#if TI_I2C_CONF_ENABLE

I2CCC26XX_Object i2cCC26xxObjects[CC1350DK_7XD_I2CCOUNT];

const I2CCC26XX_HWAttrsV1 i2cCC26xxHWAttrs[CC1350DK_7XD_I2CCOUNT] = {
#if TI_I2C_CONF_I2C0_ENABLE
    {
        .baseAddr    = I2C0_BASE,
        .powerMngrId = PowerCC26XX_PERIPH_I2C0,
        .intNum      = INT_I2C_IRQ,
        .intPriority = ~0,
        .swiPriority = 0,
        .sdaPin      = CC1350DK_7XD_I2C0_SDA0,
        .sclPin      = CC1350DK_7XD_I2C0_SCL0,
    },
#endif
};

const I2C_Config I2C_config[CC1350DK_7XD_I2CCOUNT] = {
#if TI_I2C_CONF_I2C0_ENABLE
    {
        .fxnTablePtr = &I2CCC26XX_fxnTable,
        .object      = &i2cCC26xxObjects[CC1350DK_7XD_I2C0],
        .hwAttrs     = &i2cCC26xxHWAttrs[CC1350DK_7XD_I2C0]
    },
#endif
};

const uint_least8_t I2C_count = CC1350DK_7XD_I2CCOUNT;

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

/* Allocate objects for NVS Internal Regions */
NVSCC26XX_Object nvsCC26xxObjects[1];

/* Hardware attributes for NVS Internal Regions */
const NVSCC26XX_HWAttrs nvsCC26xxHWAttrs[1] = {
    {
        .regionBase = (void *)flashBuf,
        .regionSize = REGIONSIZE,
    },
};

#endif /* TI_NVS_CONF_NVS_INTERNAL_ENABLE */

/* NVS Region index 0 and 1 refer to NVS and NVS SPI respectively */
const NVS_Config NVS_config[CC1350DK_7XD_NVSCOUNT] = {
#if TI_NVS_CONF_NVS_INTERNAL_ENABLE
    {
        .fxnTablePtr = &NVSCC26XX_fxnTable,
        .object = &nvsCC26xxObjects[0],
        .hwAttrs = &nvsCC26xxHWAttrs[0],
    },
#endif
};

const uint_least8_t NVS_count = CC1350DK_7XD_NVSCOUNT;

#endif /* TI_NVS_CONF_ENABLE */

/*
 *  =============================== PIN ===============================
 */
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>

const PIN_Config BoardGpioInitTable[] = {

    CC1350DK_7XD_PIN_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,       /* LED initially off */
    CC1350DK_7XD_PIN_LED2 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,       /* LED initially off */
    CC1350DK_7XD_PIN_LED3 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,       /* LED initially off */
    CC1350DK_7XD_PIN_LED4 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,       /* LED initially off */
    CC1350DK_7XD_PIN_KEY_SELECT | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,    /* Button is active low */
    CC1350DK_7XD_PIN_KEY_UP | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,        /* Button is active low */
    CC1350DK_7XD_PIN_KEY_DOWN | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,      /* Button is active low */
    CC1350DK_7XD_PIN_KEY_LEFT | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,      /* Button is active low */
    CC1350DK_7XD_PIN_KEY_UP | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,        /* Button is active low */
    CC1350DK_7XD_SDCARD_CS | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MIN,     /* External flash chip select */
    CC1350DK_7XD_ACC_CS | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MIN,        /* External flash chip select */
    CC1350DK_7XD_UART_RX | PIN_INPUT_EN | PIN_PULLDOWN,                                              /* UART RX via debugger back channel */
    CC1350DK_7XD_UART_TX | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL,                        /* UART TX via debugger back channel */
    CC1350DK_7XD_SPI0_MOSI | PIN_INPUT_EN | PIN_PULLDOWN,                                            /* SPI master out - slave in */
    CC1350DK_7XD_SPI0_MISO | PIN_INPUT_EN | PIN_PULLDOWN,                                            /* SPI master in - slave out */
    CC1350DK_7XD_SPI0_CLK | PIN_INPUT_EN | PIN_PULLDOWN,                                             /* SPI clock */

    PIN_TERMINATE
};

const PINCC26XX_HWAttrs PINCC26XX_hwAttrs = {
    .intPriority = ~0,
    .swiPriority = 0
};

/*
 *  =============================== Power ===============================
 */
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>

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

PWMTimerCC26XX_Object pwmtimerCC26xxObjects[CC1350DK_7XD_PWMCOUNT];

const PWMTimerCC26XX_HwAttrs pwmtimerCC26xxHWAttrs[CC1350DK_7XD_PWMCOUNT] = {
    { .pwmPin = CC1350DK_7XD_PWMPIN0, .gpTimerUnit = CC1350DK_7XD_GPTIMER0A },
    { .pwmPin = CC1350DK_7XD_PWMPIN1, .gpTimerUnit = CC1350DK_7XD_GPTIMER0B },
    { .pwmPin = CC1350DK_7XD_PWMPIN2, .gpTimerUnit = CC1350DK_7XD_GPTIMER1A },
    { .pwmPin = CC1350DK_7XD_PWMPIN3, .gpTimerUnit = CC1350DK_7XD_GPTIMER1B },
    { .pwmPin = CC1350DK_7XD_PWMPIN4, .gpTimerUnit = CC1350DK_7XD_GPTIMER2A },
    { .pwmPin = CC1350DK_7XD_PWMPIN5, .gpTimerUnit = CC1350DK_7XD_GPTIMER2B },
    { .pwmPin = CC1350DK_7XD_PWMPIN6, .gpTimerUnit = CC1350DK_7XD_GPTIMER3A },
    { .pwmPin = CC1350DK_7XD_PWMPIN7, .gpTimerUnit = CC1350DK_7XD_GPTIMER3B },
};

const PWM_Config PWM_config[CC1350DK_7XD_PWMCOUNT] = {
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC1350DK_7XD_PWM0], &pwmtimerCC26xxHWAttrs[CC1350DK_7XD_PWM0] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC1350DK_7XD_PWM1], &pwmtimerCC26xxHWAttrs[CC1350DK_7XD_PWM1] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC1350DK_7XD_PWM2], &pwmtimerCC26xxHWAttrs[CC1350DK_7XD_PWM2] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC1350DK_7XD_PWM3], &pwmtimerCC26xxHWAttrs[CC1350DK_7XD_PWM3] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC1350DK_7XD_PWM4], &pwmtimerCC26xxHWAttrs[CC1350DK_7XD_PWM4] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC1350DK_7XD_PWM5], &pwmtimerCC26xxHWAttrs[CC1350DK_7XD_PWM5] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC1350DK_7XD_PWM6], &pwmtimerCC26xxHWAttrs[CC1350DK_7XD_PWM6] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[CC1350DK_7XD_PWM7], &pwmtimerCC26xxHWAttrs[CC1350DK_7XD_PWM7] },
};

const uint_least8_t PWM_count = CC1350DK_7XD_PWMCOUNT;

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
 *  =============================== SD ===============================
 */
#include <ti/drivers/SD.h>
#include <ti/drivers/sd/SDSPI.h>

#if TI_SD_CONF_ENABLE

#if !(TI_SPI_CONF_SPI0_ENABLE)
#error "SD driver requires SPI0 enabled"
#endif

SDSPI_Object sdspiObjects[CC1350DK_7XD_SDCOUNT];

const SDSPI_HWAttrs sdspiHWAttrs[CC1350DK_7XD_SDCOUNT] = {
    {
        .spiIndex = CC1350DK_7XD_SPI0,
        .spiCsGpioIndex = CC1350DK_7XD_SDCARD_CS
    }
};

const SD_Config SD_config[CC1350DK_7XD_SDCOUNT] = {
    {
        .fxnTablePtr = &SDSPI_fxnTable,
        .object = &sdspiObjects[CC1350DK_7XD_SDSPI0],
        .hwAttrs = &sdspiHWAttrs[CC1350DK_7XD_SDSPI0]
    },
};

const uint_least8_t SD_count = CC1350DK_7XD_SDCOUNT;

#endif /* TI_SD_CONF_ENABLE */

/*
 *  =============================== SPI DMA ===============================
 */
#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPICC26XXDMA.h>

#if TI_SPI_CONF_ENABLE

SPICC26XXDMA_Object spiCC26XXDMAObjects[CC1350DK_7XD_SPICOUNT];

/*
 * NOTE: The SPI instances below can be used by the SD driver to communicate
 * with a SD card via SPI.  The 'defaultTxBufValue' fields below are set to 0xFF
 * to satisfy the SDSPI driver requirement.
 */
const SPICC26XXDMA_HWAttrsV1 spiCC26XXDMAHWAttrs[CC1350DK_7XD_SPICOUNT] = {
#if TI_SPI_CONF_SPI0_ENABLE
    {
        .baseAddr           = SSI0_BASE,
        .intNum             = INT_SSI0_COMB,
        .intPriority        = ~0,
        .swiPriority        = 0,
        .powerMngrId        = PowerCC26XX_PERIPH_SSI0,
        .defaultTxBufValue  = 0xFF,
        .rxChannelBitMask   = 1<<UDMA_CHAN_SSI0_RX,
        .txChannelBitMask   = 1<<UDMA_CHAN_SSI0_TX,
        .mosiPin            = CC1350DK_7XD_SPI0_MOSI,
        .misoPin            = CC1350DK_7XD_SPI0_MISO,
        .clkPin             = CC1350DK_7XD_SPI0_CLK,
        .csnPin             = CC1350DK_7XD_SPI0_CSN,
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
        .defaultTxBufValue  = 0xFF,
        .rxChannelBitMask   = 1<<UDMA_CHAN_SSI1_RX,
        .txChannelBitMask   = 1<<UDMA_CHAN_SSI1_TX,
        .mosiPin            = CC1350DK_7XD_SPI1_MOSI,
        .misoPin            = CC1350DK_7XD_SPI1_MISO,
        .clkPin             = CC1350DK_7XD_SPI1_CLK,
        .csnPin             = CC1350DK_7XD_SPI1_CSN,
        .minDmaTransferSize = 10
    },
#endif
};

const SPI_Config SPI_config[CC1350DK_7XD_SPICOUNT] = {
#if TI_SPI_CONF_SPI0_ENABLE
    {
         .fxnTablePtr = &SPICC26XXDMA_fxnTable,
         .object      = &spiCC26XXDMAObjects[CC1350DK_7XD_SPI0],
         .hwAttrs     = &spiCC26XXDMAHWAttrs[CC1350DK_7XD_SPI0]
    },
#endif
#if TI_SPI_CONF_SPI1_ENABLE
    {
         .fxnTablePtr = &SPICC26XXDMA_fxnTable,
         .object      = &spiCC26XXDMAObjects[CC1350DK_7XD_SPI1],
         .hwAttrs     = &spiCC26XXDMAHWAttrs[CC1350DK_7XD_SPI1]
    },
#endif
};

const uint_least8_t SPI_count = CC1350DK_7XD_SPICOUNT;

#endif /* TI_SPI_CONF_ENABLE */


/*
 *  =============================== TRNG ===============================
 */
#include <ti/drivers/TRNG.h>
#include <ti/drivers/trng/TRNGCC26X0.h>

TRNGCC26X0_Object trngCC26X0Object[CC1350DK_7XD_TRNGCOUNT];

const TRNGCC26X0_HWAttrs trngCC26X0HWAttrs[CC1350DK_7XD_TRNGCOUNT] = {
    {
         .swiPriority = 0,
         .intPriority = ~0,
    }
};

const TRNG_Config TRNG_config[] = {
    { &trngCC26X0Object[0], &trngCC26X0HWAttrs[0] },
};

const uint8_t TRNG_count = CC1350DK_7XD_TRNGCOUNT;


/*
 *  =============================== UART ===============================
 */
#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTCC26XX.h>

#if TI_UART_CONF_ENABLE

UARTCC26XX_Object uartCC26XXObjects[CC1350DK_7XD_UARTCOUNT];

uint8_t uartCC26XXRingBuffer[CC1350DK_7XD_UARTCOUNT][32];

const UARTCC26XX_HWAttrsV2 uartCC26XXHWAttrs[CC1350DK_7XD_UARTCOUNT] = {
#if TI_UART_CONF_UART0_ENABLE
    {
        .baseAddr       = UART0_BASE,
        .powerMngrId    = PowerCC26XX_PERIPH_UART0,
        .intNum         = INT_UART0_COMB,
        .intPriority    = ~0,
        .swiPriority    = 0,
        .txPin          = CC1350DK_7XD_UART_TX,
        .rxPin          = CC1350DK_7XD_UART_RX,
        .ctsPin         = PIN_UNASSIGNED,
        .rtsPin         = PIN_UNASSIGNED,
        .ringBufPtr     = uartCC26XXRingBuffer[CC1350DK_7XD_UART0],
        .ringBufSize    = sizeof(uartCC26XXRingBuffer[CC1350DK_7XD_UART0]),
        .txIntFifoThr   = UARTCC26XX_FIFO_THRESHOLD_1_8,
        .rxIntFifoThr   = UARTCC26XX_FIFO_THRESHOLD_4_8,
        .errorFxn       = NULL
    },
#endif
};

const UART_Config UART_config[CC1350DK_7XD_UARTCOUNT] = {
#if TI_UART_CONF_UART0_ENABLE
    {
        .fxnTablePtr = &UARTCC26XX_fxnTable,
        .object      = &uartCC26XXObjects[CC1350DK_7XD_UART0],
        .hwAttrs     = &uartCC26XXHWAttrs[CC1350DK_7XD_UART0]
    },
#endif
};

const uint_least8_t UART_count = CC1350DK_7XD_UARTCOUNT;

#endif /* TI_UART_CONF_ENABLE */

/*
 *  =============================== UDMA ===============================
 */
#include <ti/drivers/dma/UDMACC26XX.h>

UDMACC26XX_Object udmaObjects[CC1350DK_7XD_UDMACOUNT];

const UDMACC26XX_HWAttrs udmaHWAttrs[CC1350DK_7XD_UDMACOUNT] = {
    {
        .baseAddr    = UDMA0_BASE,
        .powerMngrId = PowerCC26XX_PERIPH_UDMA,
        .intNum      = INT_DMA_ERR,
        .intPriority = ~0
    }
};

const UDMACC26XX_Config UDMACC26XX_config[CC1350DK_7XD_UDMACOUNT] = {
    {
         .object  = &udmaObjects[CC1350DK_7XD_UDMA0],
         .hwAttrs = &udmaHWAttrs[CC1350DK_7XD_UDMA0]
    },
};



/*
 *  =============================== Watchdog ===============================
 */
#include <ti/drivers/Watchdog.h>
#include <ti/drivers/watchdog/WatchdogCC26XX.h>

WatchdogCC26XX_Object watchdogCC26XXObjects[CC1350DK_7XD_WATCHDOGCOUNT];

const WatchdogCC26XX_HWAttrs watchdogCC26XXHWAttrs[CC1350DK_7XD_WATCHDOGCOUNT] = {
    {
        .baseAddr    = WDT_BASE,
        .reloadValue = 1000 /* Reload value in milliseconds */
    },
};

const Watchdog_Config Watchdog_config[CC1350DK_7XD_WATCHDOGCOUNT] = {
    {
        .fxnTablePtr = &WatchdogCC26XX_fxnTable,
        .object      = &watchdogCC26XXObjects[CC1350DK_7XD_WATCHDOG0],
        .hwAttrs     = &watchdogCC26XXHWAttrs[CC1350DK_7XD_WATCHDOG0]
    },
};

const uint_least8_t Watchdog_count = CC1350DK_7XD_WATCHDOGCOUNT;

/*
 *  Board-specific initialization function to disable external flash.
 *  This function is defined in the file CC1350DK_7XD_fxns.c
 */
extern void Board_initHook(void);

/*
 *  ======== CC1350DK_7XD_initGeneral ========
 */
void CC1350DK_7XD_initGeneral(void)
{
    Power_init();

    if (PIN_init(BoardGpioInitTable) != PIN_SUCCESS) {
        /* Error with PIN_init */
        while (1);
    }

    /* Perform board-specific initialization */
    Board_initHook();
}
