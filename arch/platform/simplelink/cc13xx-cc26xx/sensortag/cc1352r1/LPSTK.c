/*
 * Copyright (c) 2017-2019, Texas Instruments Incorporated
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
 *  ====================== LPSTK.c ===================================
 *  This file is responsible for setting up the board specific items for the
 *  LPSTK board.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/ioc.h)
#include DeviceFamily_constructPath(driverlib/udma.h)
#include DeviceFamily_constructPath(inc/hw_ints.h)
#include DeviceFamily_constructPath(inc/hw_memmap.h)

#include "LPSTK.h"

/*
 *  =============================== ADCBuf ===============================
 */
#include <ti/drivers/ADCBuf.h>
#include <ti/drivers/adcbuf/ADCBufCC26X2.h>

ADCBufCC26X2_Object adcBufCC26xxObjects[LPSTK_ADCBUFCOUNT];

/*
 *  This table converts a virtual adc channel into a dio and internal analogue
 *  input signal. This table is necessary for the functioning of the adcBuf
 *  driver. Comment out unused entries to save flash. Dio and internal signal
 *  pairs are hardwired. Do not remap them in the table. You may reorder entire
 *  entries. The mapping of dio and internal signals is package dependent.
 */
const ADCBufCC26X2_AdcChannelLutEntry ADCBufCC26X2_adcChannelLut[LPSTK_ADCBUF0CHANNELCOUNT] = {
    {LPSTK_DIO24_ANALOG, ADC_COMPB_IN_AUXIO6},
    {LPSTK_DIO26_ANALOG, ADC_COMPB_IN_AUXIO4},
    {LPSTK_DIO28_ANALOG, ADC_COMPB_IN_AUXIO2},
    {LPSTK_DIO29_ANALOG, ADC_COMPB_IN_AUXIO1},
    {PIN_UNASSIGNED, ADC_COMPB_IN_VDDS},
    {PIN_UNASSIGNED, ADC_COMPB_IN_DCOUPL},
    {PIN_UNASSIGNED, ADC_COMPB_IN_VSS},
};

const ADCBufCC26X2_HWAttrs adcBufCC26xxHWAttrs[LPSTK_ADCBUFCOUNT] = {
    {
        .intPriority       = ~0,
        .swiPriority       = 0,
        .adcChannelLut     = ADCBufCC26X2_adcChannelLut,
        .gpTimerUnit       = LPSTK_GPTIMER0A,
    }
};

const ADCBuf_Config ADCBuf_config[LPSTK_ADCBUFCOUNT] = {
    {
        &ADCBufCC26X2_fxnTable,
        &adcBufCC26xxObjects[LPSTK_ADCBUF0],
        &adcBufCC26xxHWAttrs[LPSTK_ADCBUF0]
    },
};

const uint_least8_t ADCBuf_count = LPSTK_ADCBUFCOUNT;

/*
 *  =============================== ADC ===============================
 */
#include <ti/drivers/ADC.h>
#include <ti/drivers/adc/ADCCC26XX.h>

ADCCC26XX_Object adcCC26xxObjects[LPSTK_ADCCOUNT];

const ADCCC26XX_HWAttrs adcCC26xxHWAttrs[LPSTK_ADCCOUNT] = {
    {
        .adcDIO              = LPSTK_DIO24_ANALOG,
        .adcCompBInput       = ADC_COMPB_IN_AUXIO6,
        .refSource           = ADCCC26XX_FIXED_REFERENCE,
        .samplingDuration    = ADCCC26XX_SAMPLING_DURATION_2P7_US,
        .inputScalingEnabled = true,
        .triggerSource       = ADCCC26XX_TRIGGER_MANUAL,
        .returnAdjustedVal   = false
    },
    {
        .adcDIO              = LPSTK_DIO26_ANALOG,
        .adcCompBInput       = ADC_COMPB_IN_AUXIO4,
        .refSource           = ADCCC26XX_FIXED_REFERENCE,
        .samplingDuration    = ADCCC26XX_SAMPLING_DURATION_2P7_US,
        .inputScalingEnabled = true,
        .triggerSource       = ADCCC26XX_TRIGGER_MANUAL,
        .returnAdjustedVal   = false
    },
    {
        .adcDIO              = LPSTK_DIO28_ANALOG,
        .adcCompBInput       = ADC_COMPB_IN_AUXIO2,
        .refSource           = ADCCC26XX_FIXED_REFERENCE,
        .samplingDuration    = ADCCC26XX_SAMPLING_DURATION_2P7_US,
        .inputScalingEnabled = true,
        .triggerSource       = ADCCC26XX_TRIGGER_MANUAL,
        .returnAdjustedVal   = false
    },
    {
        .adcDIO              = LPSTK_DIO29_ANALOG,
        .adcCompBInput       = ADC_COMPB_IN_AUXIO1,
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

const ADC_Config ADC_config[LPSTK_ADCCOUNT] = {
    {&ADCCC26XX_fxnTable, &adcCC26xxObjects[LPSTK_ADC0], &adcCC26xxHWAttrs[LPSTK_ADC0]},
    {&ADCCC26XX_fxnTable, &adcCC26xxObjects[LPSTK_ADC1], &adcCC26xxHWAttrs[LPSTK_ADC1]},
    {&ADCCC26XX_fxnTable, &adcCC26xxObjects[LPSTK_ADC2], &adcCC26xxHWAttrs[LPSTK_ADC2]},
    {&ADCCC26XX_fxnTable, &adcCC26xxObjects[LPSTK_ADC3], &adcCC26xxHWAttrs[LPSTK_ADC3]},
    {&ADCCC26XX_fxnTable, &adcCC26xxObjects[LPSTK_ADC4], &adcCC26xxHWAttrs[LPSTK_ADC4]},
    {&ADCCC26XX_fxnTable, &adcCC26xxObjects[LPSTK_ADC5], &adcCC26xxHWAttrs[LPSTK_ADC5]},
    {&ADCCC26XX_fxnTable, &adcCC26xxObjects[LPSTK_ADC6], &adcCC26xxHWAttrs[LPSTK_ADC6]},
    {&ADCCC26XX_fxnTable, &adcCC26xxObjects[LPSTK_ADCDCOUPL], &adcCC26xxHWAttrs[LPSTK_ADCDCOUPL]},
    {&ADCCC26XX_fxnTable, &adcCC26xxObjects[LPSTK_ADCVSS], &adcCC26xxHWAttrs[LPSTK_ADCVSS]},
    {&ADCCC26XX_fxnTable, &adcCC26xxObjects[LPSTK_ADCVDDS], &adcCC26xxHWAttrs[LPSTK_ADCVDDS]},
};

const uint_least8_t ADC_count = LPSTK_ADCCOUNT;

/*
 *  =============================== ECDH ===============================
 */
#include <ti/drivers/ECDH.h>
#include <ti/drivers/ecdh/ECDHCC26X2.h>

ECDHCC26X2_Object ecdhCC26X2Objects[LPSTK_ECDHCOUNT];

const ECDHCC26X2_HWAttrs ecdhCC26X2HWAttrs[LPSTK_ECDHCOUNT] = {
    {
        .intPriority       = ~0,
    }
};

const ECDH_Config ECDH_config[LPSTK_ECDHCOUNT] = {
    {
         .object  = &ecdhCC26X2Objects[LPSTK_ECDH0],
         .hwAttrs = &ecdhCC26X2HWAttrs[LPSTK_ECDH0]
    },
};

const uint_least8_t ECDH_count = LPSTK_ECDHCOUNT;

/*
 *  =============================== ECDSA ===============================
 */
#include <ti/drivers/ECDSA.h>
#include <ti/drivers/ecdsa/ECDSACC26X2.h>

ECDSACC26X2_Object ecdsaCC26X2Objects[LPSTK_ECDSACOUNT];

const ECDSACC26X2_HWAttrs ecdsaCC26X2HWAttrs[LPSTK_ECDSACOUNT] = {
    {
        .intPriority       = ~0,
    }
};

const ECDSA_Config ECDSA_config[LPSTK_ECDSACOUNT] = {
    {
         .object  = &ecdsaCC26X2Objects[LPSTK_ECDSA0],
         .hwAttrs = &ecdsaCC26X2HWAttrs[LPSTK_ECDSA0]
    },
};

const uint_least8_t ECDSA_count = LPSTK_ECDSACOUNT;

/*
 *  =============================== ECJPAKE ===============================
 */
#include <ti/drivers/ECJPAKE.h>
#include <ti/drivers/ecjpake/ECJPAKECC26X2.h>

ECJPAKECC26X2_Object ecjpakeCC26X2Objects[LPSTK_ECJPAKECOUNT];

const ECJPAKECC26X2_HWAttrs ecjpakeCC26X2HWAttrs[LPSTK_ECJPAKECOUNT] = {
    {
        .intPriority       = ~0,
    }
};

const ECJPAKE_Config ECJPAKE_config[LPSTK_ECJPAKECOUNT] = {
    {
         .object  = &ecjpakeCC26X2Objects[LPSTK_ECJPAKE0],
         .hwAttrs = &ecjpakeCC26X2HWAttrs[LPSTK_ECJPAKE0]
    },
};

const uint_least8_t ECJPAKE_count = LPSTK_ECJPAKECOUNT;


/*
 *  =============================== SHA2 ===============================
 */
#include <ti/drivers/SHA2.h>
#include <ti/drivers/sha2/SHA2CC26X2.h>

SHA2CC26X2_Object sha2CC26X2Objects[LPSTK_SHA2COUNT];

const SHA2CC26X2_HWAttrs sha2CC26X2HWAttrs[LPSTK_SHA2COUNT] = {
    {
        .intPriority       = ~0,
    }
};

const SHA2_Config SHA2_config[LPSTK_SHA2COUNT] = {
    {
         .object  = &sha2CC26X2Objects[LPSTK_SHA20],
         .hwAttrs = &sha2CC26X2HWAttrs[LPSTK_SHA20]
    },
};

const uint_least8_t SHA2_count = LPSTK_SHA2COUNT;

/*
 *  =============================== AESCCM ===============================
 */
#include <ti/drivers/AESCCM.h>
#include <ti/drivers/aesccm/AESCCMCC26XX.h>

AESCCMCC26XX_Object aesccmCC26XXObjects[LPSTK_AESCCMCOUNT];

const AESCCMCC26XX_HWAttrs aesccmCC26XXHWAttrs[LPSTK_AESCCMCOUNT] = {
    {
        .intPriority       = ~0,
    }
};

const AESCCM_Config AESCCM_config[LPSTK_AESCCMCOUNT] = {
    {
         .object  = &aesccmCC26XXObjects[LPSTK_AESCCM0],
         .hwAttrs = &aesccmCC26XXHWAttrs[LPSTK_AESCCM0]
    },
};

const uint_least8_t AESCCM_count = LPSTK_AESCCMCOUNT;

/*
 *  =============================== AESGCM ===============================
 */
#include <ti/drivers/AESGCM.h>
#include <ti/drivers/aesgcm/AESGCMCC26XX.h>

AESGCMCC26XX_Object aesgcmCC26XXObjects[LPSTK_AESGCMCOUNT];

const AESGCMCC26XX_HWAttrs aesgcmCC26XXHWAttrs[LPSTK_AESGCMCOUNT] = {
    {
        .intPriority       = ~0,
    }
};

const AESGCM_Config AESGCM_config[LPSTK_AESGCMCOUNT] = {
    {
         .object  = &aesgcmCC26XXObjects[LPSTK_AESGCM0],
         .hwAttrs = &aesgcmCC26XXHWAttrs[LPSTK_AESGCM0]
    },
};

const uint_least8_t AESGCM_count = LPSTK_AESGCMCOUNT;

/*
 *  =============================== AESCBC ===============================
 */
#include <ti/drivers/AESCBC.h>
#include <ti/drivers/aescbc/AESCBCCC26XX.h>

AESCBCCC26XX_Object aescbcCC26XXObjects[LPSTK_AESCBCCOUNT];

const AESCBCCC26XX_HWAttrs aescbcCC26XXHWAttrs[LPSTK_AESCBCCOUNT] = {
    {
        .intPriority       = ~0,
    }
};

const AESCBC_Config AESCBC_config[LPSTK_AESCBCCOUNT] = {
    {
         .object  = &aescbcCC26XXObjects[LPSTK_AESCBC0],
         .hwAttrs = &aescbcCC26XXHWAttrs[LPSTK_AESCBC0]
    },
};

const uint_least8_t AESCBC_count = LPSTK_AESCBCCOUNT;

/*
 *  =============================== AESCTR ===============================
 */
#include <ti/drivers/AESCTR.h>
#include <ti/drivers/aesctr/AESCTRCC26XX.h>

AESCTRCC26XX_Object aesctrCC26XXObjects[LPSTK_AESCTRCOUNT];

const AESCTRCC26XX_HWAttrs aesctrCC26XXHWAttrs[LPSTK_AESCTRCOUNT] = {
    {
        .intPriority       = ~0,
    }
};

const AESCTR_Config AESCTR_config[LPSTK_AESCTRCOUNT] = {
    {
         .object  = &aesctrCC26XXObjects[LPSTK_AESCTR0],
         .hwAttrs = &aesctrCC26XXHWAttrs[LPSTK_AESCTR0]
    },
};

const uint_least8_t AESCTR_count = LPSTK_AESCTRCOUNT;

/*
 *  =============================== AESECB ===============================
 */
#include <ti/drivers/AESECB.h>
#include <ti/drivers/aesecb/AESECBCC26XX.h>

AESECBCC26XX_Object aesecbCC26XXObjects[LPSTK_AESECBCOUNT];

const AESECBCC26XX_HWAttrs aesecbCC26XXHWAttrs[LPSTK_AESECBCOUNT] = {
    {
        .intPriority       = ~0,
    }
};

const AESECB_Config AESECB_config[LPSTK_AESECBCOUNT] = {
    {
         .object  = &aesecbCC26XXObjects[LPSTK_AESECB0],
         .hwAttrs = &aesecbCC26XXHWAttrs[LPSTK_AESECB0]
    },
};

const uint_least8_t AESECB_count = LPSTK_AESECBCOUNT;

/*
 *  =============================== AESCTRDRBG ===============================
 */
#include <ti/drivers/AESCTRDRBG.h>
#include <ti/drivers/aesctrdrbg/AESCTRDRBGXX.h>

AESCTRDRBGXX_Object aesctrdrbgXXObjects[LPSTK_AESCTRDRBGCOUNT];

const AESCTRDRBGXX_HWAttrs aesctrdrbgXXHWAttrs[LPSTK_AESCTRDRBGCOUNT] = {
    {
        .aesctrIndex       = LPSTK_AESCTR0,
    }
};

const AESCTRDRBG_Config AESCTRDRBG_config[LPSTK_AESCTRDRBGCOUNT] = {
    {
         .object  = &aesctrdrbgXXObjects[LPSTK_AESCTRDRBG0],
         .hwAttrs = &aesctrdrbgXXHWAttrs[LPSTK_AESCTRDRBG0]
    },
};

const uint_least8_t AESCTRDRBG_count = LPSTK_AESCTRDRBGCOUNT;

/*
 *  =============================== TRNG ===============================
 */
#include <ti/drivers/TRNG.h>
#include <ti/drivers/trng/TRNGCC26XX.h>

TRNGCC26XX_Object trngCC26XXObjects[LPSTK_TRNGCOUNT];

const TRNGCC26XX_HWAttrs trngCC26X2HWAttrs[LPSTK_TRNGCOUNT] = {
    {
        .intPriority       = ~0,
        .swiPriority       = 0,
        .samplesPerCycle   = 240000,
    }
};

const TRNG_Config TRNG_config[LPSTK_TRNGCOUNT] = {
    {
         .object  = &trngCC26XXObjects[LPSTK_TRNG0],
         .hwAttrs = &trngCC26X2HWAttrs[LPSTK_TRNG0]
    },
};

const uint_least8_t TRNG_count = LPSTK_TRNGCOUNT;

/*
 *  =============================== GPIO ===============================
 */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/gpio/GPIOCC26XX.h>

/*
 * Array of Pin configurations
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in LPSTK.h
 * NOTE: Pins not used for interrupts should be placed at the end of the
 *       array. Callback entries can be omitted from callbacks array to
 *       reduce memory usage.
 */
GPIO_PinConfig gpioPinConfigs[] = {
    /* Input pins */
    GPIOCC26XX_DIO_15 | GPIO_DO_NOT_CONFIG,  /* Button 0 */
    GPIOCC26XX_DIO_14 | GPIO_DO_NOT_CONFIG,  /* Button 1 */

    GPIOCC26XX_DIO_15 | GPIO_DO_NOT_CONFIG,  /* LPSTK_SPI_MASTER_READY */
    GPIOCC26XX_DIO_21 | GPIO_DO_NOT_CONFIG,  /* LPSTK_SPI_SLAVE_READY */

    /* Output pins */
    GPIOCC26XX_DIO_07 | GPIO_DO_NOT_CONFIG,  /* Green LED */
    GPIOCC26XX_DIO_06 | GPIO_DO_NOT_CONFIG,  /* Red LED */

    /* SPI Flash CSN */
    GPIOCC26XX_DIO_20 | GPIO_DO_NOT_CONFIG,

    /* SD CS */
    GPIOCC26XX_DIO_21 | GPIO_DO_NOT_CONFIG,
};

/*
 * Array of callback function pointers
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in CC1352R1_LAUNCH.h
 * NOTE: Pins not used for interrupts can be omitted from callbacks array to
 *       reduce memory usage (if placed at end of gpioPinConfigs array).
 */
GPIO_CallbackFxn gpioCallbackFunctions[] = {
    NULL,  /* Button 0 */
    NULL,  /* Button 1 */
    NULL,  /* LPSTK_SPI_MASTER_READY */
    NULL,  /* LPSTK_SPI_SLAVE_READY */
};

const GPIOCC26XX_Config GPIOCC26XX_config = {
    .pinConfigs         = (GPIO_PinConfig *)gpioPinConfigs,
    .callbacks          = (GPIO_CallbackFxn *)gpioCallbackFunctions,
    .numberOfPinConfigs = LPSTK_GPIOCOUNT,
    .numberOfCallbacks  = sizeof(gpioCallbackFunctions)/sizeof(GPIO_CallbackFxn),
    .intPriority        = (~0)
};

/*
 *  =============================== GPTimer ===============================
 *  Remove unused entries to reduce flash usage both in Board.c and Board.h
 */
#include <ti/drivers/timer/GPTimerCC26XX.h>

GPTimerCC26XX_Object gptimerCC26XXObjects[LPSTK_GPTIMERCOUNT];

const GPTimerCC26XX_HWAttrs gptimerCC26xxHWAttrs[LPSTK_GPTIMERPARTSCOUNT] = {
    { .baseAddr = GPT0_BASE, .intNum = INT_GPT0A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT0, .pinMux = GPT_PIN_0A, },
    { .baseAddr = GPT0_BASE, .intNum = INT_GPT0B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT0, .pinMux = GPT_PIN_0B, },
    { .baseAddr = GPT1_BASE, .intNum = INT_GPT1A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT1, .pinMux = GPT_PIN_1A, },
    { .baseAddr = GPT1_BASE, .intNum = INT_GPT1B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT1, .pinMux = GPT_PIN_1B, },
    { .baseAddr = GPT2_BASE, .intNum = INT_GPT2A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT2, .pinMux = GPT_PIN_2A, },
    { .baseAddr = GPT2_BASE, .intNum = INT_GPT2B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT2, .pinMux = GPT_PIN_2B, },
    { .baseAddr = GPT3_BASE, .intNum = INT_GPT3A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT3, .pinMux = GPT_PIN_3A, },
    { .baseAddr = GPT3_BASE, .intNum = INT_GPT3B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT3, .pinMux = GPT_PIN_3B, },
};

const GPTimerCC26XX_Config GPTimerCC26XX_config[LPSTK_GPTIMERPARTSCOUNT] = {
    { &gptimerCC26XXObjects[LPSTK_GPTIMER0], &gptimerCC26xxHWAttrs[LPSTK_GPTIMER0A], GPT_A },
    { &gptimerCC26XXObjects[LPSTK_GPTIMER0], &gptimerCC26xxHWAttrs[LPSTK_GPTIMER0B], GPT_B },
    { &gptimerCC26XXObjects[LPSTK_GPTIMER1], &gptimerCC26xxHWAttrs[LPSTK_GPTIMER1A], GPT_A },
    { &gptimerCC26XXObjects[LPSTK_GPTIMER1], &gptimerCC26xxHWAttrs[LPSTK_GPTIMER1B], GPT_B },
    { &gptimerCC26XXObjects[LPSTK_GPTIMER2], &gptimerCC26xxHWAttrs[LPSTK_GPTIMER2A], GPT_A },
    { &gptimerCC26XXObjects[LPSTK_GPTIMER2], &gptimerCC26xxHWAttrs[LPSTK_GPTIMER2B], GPT_B },
    { &gptimerCC26XXObjects[LPSTK_GPTIMER3], &gptimerCC26xxHWAttrs[LPSTK_GPTIMER3A], GPT_A },
    { &gptimerCC26XXObjects[LPSTK_GPTIMER3], &gptimerCC26xxHWAttrs[LPSTK_GPTIMER3B], GPT_B },
};

/*
 *  =============================== I2C ===============================
*/
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CCC26XX.h>

#if TI_I2C_CONF_ENABLE

I2CCC26XX_Object i2cCC26xxObjects[LPSTK_I2CCOUNT];

const I2CCC26XX_HWAttrsV1 i2cCC26xxHWAttrs[LPSTK_I2CCOUNT] = {
#if TI_I2C_CONF_I2C0_ENABLE
    {
        .baseAddr    = I2C0_BASE,
        .powerMngrId = PowerCC26XX_PERIPH_I2C0,
        .intNum      = INT_I2C_IRQ,
        .intPriority = ~0,
        .swiPriority = 0,
        .sdaPin      = LPSTK_I2C0_SDA0,
        .sclPin      = LPSTK_I2C0_SCL0,
    },
    {
        .baseAddr    = I2C0_BASE,
        .powerMngrId = PowerCC26XX_PERIPH_I2C0,
        .intNum      = INT_I2C_IRQ,
        .intPriority = ~0,
        .swiPriority = 0,
        .sdaPin      = LPSTK_I2C0_SDA1,
        .sclPin      = LPSTK_I2C0_SCL1,
    },
#endif
};

const I2C_Config I2C_config[LPSTK_I2CCOUNT] = {
#if TI_I2C_CONF_I2C0_ENABLE
    {
        .fxnTablePtr = &I2CCC26XX_fxnTable,
        .object      = &i2cCC26xxObjects[LPSTK_I2C0],
        .hwAttrs     = &i2cCC26xxHWAttrs[LPSTK_I2C0]
    },
    {
        .fxnTablePtr = &I2CCC26XX_fxnTable,
        .object      = &i2cCC26xxObjects[LPSTK_I2C1],
        .hwAttrs     = &i2cCC26xxHWAttrs[LPSTK_I2C1]
    },
#endif
};

const uint_least8_t I2C_count = LPSTK_I2CCOUNT;

#endif /* TI_I2C_CONF_ENABLE */

/*
 *  =============================== I2S ===============================
*/
#include <ti/drivers/I2S.h>
#include <ti/drivers/i2s/I2SCC26XX.h>

I2SCC26XX_Object i2sCC26XXObjects[LPSTK_I2SCOUNT];

const I2SCC26XX_HWAttrs i2sCC26XXHWAttrs[LPSTK_I2SCOUNT] = {
    {
        .pinSD1      =  LPSTK_I2S_ADI,
        .pinSD0      =  LPSTK_I2S_ADO,
        .pinSCK      =  LPSTK_I2S_BCLK,
        .pinMCLK     =  LPSTK_I2S_MCLK,
        .pinWS       =  LPSTK_I2S_WCLK,
        .intPriority = ~0,
    }
};

const I2S_Config I2S_config[LPSTK_I2SCOUNT] = {
    {
        .object      = &i2sCC26XXObjects[LPSTK_I2S0],
        .hwAttrs     = &i2sCC26XXHWAttrs[LPSTK_I2S0]
    },
};

const uint_least8_t I2S_count = LPSTK_I2SCOUNT;

/*
 *  =============================== NVS ===============================
 */
#include <ti/drivers/NVS.h>
#include <ti/drivers/nvs/NVSSPI25X.h>
#include <ti/drivers/nvs/NVSCC26XX.h>

#define NVS_REGIONS_BASE 0x48000
#define SECTORSIZE       0x2000
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

#if TI_NVS_CONF_NVS_EXTERNAL_ENABLE

#define SPISECTORSIZE    0x1000
#define SPIREGIONSIZE    (SPISECTORSIZE * 32)
#define VERIFYBUFSIZE    64

static uint8_t verifyBuf[VERIFYBUFSIZE];

/* Allocate objects for NVS External Regions */
NVSSPI25X_Object nvsSPI25XObjects[1];

/* Hardware attributes for NVS External Regions */
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
        .spiCsnGpioIndex = LPSTK_GPIO_SPI_FLASH_CS,
        .statusPollDelayUs = 100,
    },
};

#endif /* TI_NVS_CONF_NVS_EXTERNAL_ENABLE */

/* NVS Region index 0 and 1 refer to NVS and NVS SPI respectively */
const NVS_Config NVS_config[LPSTK_NVSCOUNT] = {
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

const uint_least8_t NVS_count = LPSTK_NVSCOUNT;

#endif /* TI_NVS_CONF_ENABLE */

/*
 *  =============================== PIN ===============================
 */
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>

const PIN_Config BoardGpioInitTable[] = {

    LPSTK_PIN_RLED | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,          /* LED initially off */
    LPSTK_PIN_GLED | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,          /* LED initially off */
    LPSTK_PIN_BTN1 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,             /* Button is active low */
    LPSTK_PIN_BTN2 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,             /* Button is active low */
    LPSTK_SPI_FLASH_CS | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MIN,     /* External flash chip select */
    LPSTK_UART0_RX | PIN_INPUT_EN | PIN_PULLDOWN,                                                /* UART RX via debugger back channel */
    LPSTK_UART0_TX | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL,                           /* UART TX via debugger back channel */
    LPSTK_SPI0_MOSI | PIN_INPUT_EN | PIN_PULLDOWN,                                               /* SPI master out - slave in */
    LPSTK_SPI0_MISO | PIN_INPUT_EN | PIN_PULLDOWN,                                               /* SPI master in - slave out */
    LPSTK_SPI0_CLK | PIN_INPUT_EN | PIN_PULLDOWN,                                                /* SPI clock */
    LPSTK_RELAY | PIN_INPUT_EN | PIN_PULLDOWN | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,           /* Relay is active high */
    LPSTK_MPU_INT | PIN_INPUT_EN | PIN_PULLDOWN | PIN_IRQ_NEGEDGE | PIN_HYSTERESIS,           /* MPU_INT is active low */
    LPSTK_TMP_RDY | PIN_INPUT_EN | PIN_PULLUP | PIN_HYSTERESIS,                               /* TMP_RDY is active high */
    LPSTK_BUZZER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,         /* Buzzer initially off */
    LPSTK_MPU_POWER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,     /* MPU initially on */

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
#include <ti/drivers/power/PowerCC26X2.h>
#include "clock-arch.h"

const PowerCC26X2_Config PowerCC26X2_config = {
    .policyInitFxn      = NULL,
    .policyFxn          = &clock_arch_standby_policy,
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

PWMTimerCC26XX_Object pwmtimerCC26xxObjects[LPSTK_PWMCOUNT];

const PWMTimerCC26XX_HwAttrs pwmtimerCC26xxHWAttrs[LPSTK_PWMCOUNT] = {
    { .pwmPin = LPSTK_PWMPIN0, .gpTimerUnit = LPSTK_GPTIMER0A },
    { .pwmPin = LPSTK_PWMPIN1, .gpTimerUnit = LPSTK_GPTIMER0B },
    { .pwmPin = LPSTK_PWMPIN2, .gpTimerUnit = LPSTK_GPTIMER1A },
    { .pwmPin = LPSTK_PWMPIN3, .gpTimerUnit = LPSTK_GPTIMER1B },
    { .pwmPin = LPSTK_PWMPIN4, .gpTimerUnit = LPSTK_GPTIMER2A },
    { .pwmPin = LPSTK_PWMPIN5, .gpTimerUnit = LPSTK_GPTIMER2B },
    { .pwmPin = LPSTK_PWMPIN6, .gpTimerUnit = LPSTK_GPTIMER3A },
    { .pwmPin = LPSTK_PWMPIN7, .gpTimerUnit = LPSTK_GPTIMER3B },
};

const PWM_Config PWM_config[LPSTK_PWMCOUNT] = {
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[LPSTK_PWM0], &pwmtimerCC26xxHWAttrs[LPSTK_PWM0] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[LPSTK_PWM1], &pwmtimerCC26xxHWAttrs[LPSTK_PWM1] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[LPSTK_PWM2], &pwmtimerCC26xxHWAttrs[LPSTK_PWM2] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[LPSTK_PWM3], &pwmtimerCC26xxHWAttrs[LPSTK_PWM3] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[LPSTK_PWM4], &pwmtimerCC26xxHWAttrs[LPSTK_PWM4] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[LPSTK_PWM5], &pwmtimerCC26xxHWAttrs[LPSTK_PWM5] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[LPSTK_PWM6], &pwmtimerCC26xxHWAttrs[LPSTK_PWM6] },
    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[LPSTK_PWM7], &pwmtimerCC26xxHWAttrs[LPSTK_PWM7] },
};

const uint_least8_t PWM_count = LPSTK_PWMCOUNT;

/*
 *  =============================== RF Driver ===============================
 */
#include <ti/drivers/rf/RF.h>

const RFCC26XX_HWAttrsV2 RFCC26XX_hwAttrs = {
    .hwiPriority        = ~0,     /* Lowest HWI priority */
    .swiPriority        = 0,      /* Lowest SWI priority */
    .xoscHfAlwaysNeeded = true,   /* Keep XOSC dependency while in standby */
    .globalCallback     = NULL,   /* No board specific callback */
    .globalEventMask    = 0       /* No events subscribed to */
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

SDSPI_Object sdspiObjects[LPSTK_SDCOUNT];

const SDSPI_HWAttrs sdspiHWAttrs[LPSTK_SDCOUNT] = {
    {
        .spiIndex = LPSTK_SPI0,
        .spiCsGpioIndex = LPSTK_SDSPI_CS
    }
};

const SD_Config SD_config[LPSTK_SDCOUNT] = {
    {
        .fxnTablePtr = &SDSPI_fxnTable,
        .object = &sdspiObjects[LPSTK_SDSPI0],
        .hwAttrs = &sdspiHWAttrs[LPSTK_SDSPI0]
    },
};

const uint_least8_t SD_count = LPSTK_SDCOUNT;

#endif /* TI_SD_CONF_ENABLE */

/*
 *  =============================== SPI DMA ===============================
 */
#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPICC26X2DMA.h>

#if TI_SPI_CONF_ENABLE

SPICC26X2DMA_Object spiCC26X2DMAObjects[LPSTK_SPICOUNT];

/*
 * NOTE: The SPI instances below can be used by the SD driver to communicate
 * with a SD card via SPI.  The 'defaultTxBufValue' fields below are set to 0xFF
 * to satisfy the SDSPI driver requirement.
 */
const SPICC26X2DMA_HWAttrs spiCC26X2DMAHWAttrs[LPSTK_SPICOUNT] = {
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
        .mosiPin            = LPSTK_SPI0_MOSI,
        .misoPin            = LPSTK_SPI0_MISO,
        .clkPin             = LPSTK_SPI0_CLK,
        .csnPin             = LPSTK_SPI0_CSN,
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
        .mosiPin            = LPSTK_SPI1_MOSI,
        .misoPin            = LPSTK_SPI1_MISO,
        .clkPin             = LPSTK_SPI1_CLK,
        .csnPin             = LPSTK_SPI1_CSN,
        .minDmaTransferSize = 10
    },
#endif
};

const SPI_Config SPI_config[LPSTK_SPICOUNT] = {
#if TI_SPI_CONF_SPI0_ENABLE
    {
         .fxnTablePtr = &SPICC26X2DMA_fxnTable,
         .object      = &spiCC26X2DMAObjects[LPSTK_SPI0],
         .hwAttrs     = &spiCC26X2DMAHWAttrs[LPSTK_SPI0]
    },
#endif
#if TI_SPI_CONF_SPI1_ENABLE
    {
         .fxnTablePtr = &SPICC26X2DMA_fxnTable,
         .object      = &spiCC26X2DMAObjects[LPSTK_SPI1],
         .hwAttrs     = &spiCC26X2DMAHWAttrs[LPSTK_SPI1]
    },
#endif
};

const uint_least8_t SPI_count = LPSTK_SPICOUNT;

#endif /* TI_SPI_CONF_ENABLE */

/*
 *  =============================== UART ===============================
 */
#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTCC26XX.h>

#if TI_UART_CONF_ENABLE

UARTCC26XX_Object uartCC26XXObjects[LPSTK_UARTCOUNT];

uint8_t uartCC26XXRingBuffer[LPSTK_UARTCOUNT][32];

const UARTCC26XX_HWAttrsV2 uartCC26XXHWAttrs[LPSTK_UARTCOUNT] = {
#if TI_UART_CONF_UART0_ENABLE
    {
        .baseAddr       = UART0_BASE,
        .powerMngrId    = PowerCC26XX_PERIPH_UART0,
        .intNum         = INT_UART0_COMB,
        .intPriority    = ~0,
        .swiPriority    = 0,
        .txPin          = LPSTK_UART0_TX,
        .rxPin          = LPSTK_UART0_RX,
        .ctsPin         = PIN_UNASSIGNED,
        .rtsPin         = PIN_UNASSIGNED,
        .ringBufPtr     = uartCC26XXRingBuffer[LPSTK_UART0],
        .ringBufSize    = sizeof(uartCC26XXRingBuffer[LPSTK_UART0]),
        .txIntFifoThr   = UARTCC26XX_FIFO_THRESHOLD_1_8,
        .rxIntFifoThr   = UARTCC26XX_FIFO_THRESHOLD_4_8,
        .errorFxn       = NULL
    },
#endif
#if TI_UART_CONF_UART1_ENABLE
    {
        .baseAddr       = UART1_BASE,
        .powerMngrId    = PowerCC26X2_PERIPH_UART1,
        .intNum         = INT_UART1_COMB,
        .intPriority    = ~0,
        .swiPriority    = 0,
        .txPin          = LPSTK_UART1_TX,
        .rxPin          = LPSTK_UART1_RX,
        .ctsPin         = PIN_UNASSIGNED,
        .rtsPin         = PIN_UNASSIGNED,
        .ringBufPtr     = uartCC26XXRingBuffer[LPSTK_UART1],
        .ringBufSize    = sizeof(uartCC26XXRingBuffer[LPSTK_UART1]),
        .txIntFifoThr   = UARTCC26XX_FIFO_THRESHOLD_1_8,
        .rxIntFifoThr   = UARTCC26XX_FIFO_THRESHOLD_4_8,
        .errorFxn       = NULL
    },
#endif
};

const UART_Config UART_config[LPSTK_UARTCOUNT] = {
#if TI_UART_CONF_UART0_ENABLE
    {
        .fxnTablePtr = &UARTCC26XX_fxnTable,
        .object      = &uartCC26XXObjects[LPSTK_UART0],
        .hwAttrs     = &uartCC26XXHWAttrs[LPSTK_UART0]
    },
#endif
#if TI_UART_CONF_UART1_ENABLE
    {
        .fxnTablePtr = &UARTCC26XX_fxnTable,
        .object      = &uartCC26XXObjects[LPSTK_UART1],
        .hwAttrs     = &uartCC26XXHWAttrs[LPSTK_UART1]
    },
#endif
};

const uint_least8_t UART_count = LPSTK_UARTCOUNT;

#endif /* TI_UART_CONF_ENABLE */

/*
 *  =============================== UDMA ===============================
 */
#include <ti/drivers/dma/UDMACC26XX.h>

UDMACC26XX_Object udmaObjects[LPSTK_UDMACOUNT];

const UDMACC26XX_HWAttrs udmaHWAttrs[LPSTK_UDMACOUNT] = {
    {
        .baseAddr    = UDMA0_BASE,
        .powerMngrId = PowerCC26XX_PERIPH_UDMA,
        .intNum      = INT_DMA_ERR,
        .intPriority = ~0
    }
};

const UDMACC26XX_Config UDMACC26XX_config[LPSTK_UDMACOUNT] = {
    {
         .object  = &udmaObjects[LPSTK_UDMA0],
         .hwAttrs = &udmaHWAttrs[LPSTK_UDMA0]
    },
};



/*
 *  =============================== Watchdog ===============================
 */
#include <ti/drivers/Watchdog.h>
#include <ti/drivers/watchdog/WatchdogCC26XX.h>

WatchdogCC26XX_Object watchdogCC26XXObjects[LPSTK_WATCHDOGCOUNT];

const WatchdogCC26XX_HWAttrs watchdogCC26XXHWAttrs[LPSTK_WATCHDOGCOUNT] = {
    {
        .baseAddr    = WDT_BASE,
        .reloadValue = 1000 /* Reload value in milliseconds */
    },
};

const Watchdog_Config Watchdog_config[LPSTK_WATCHDOGCOUNT] = {
    {
        .fxnTablePtr = &WatchdogCC26XX_fxnTable,
        .object      = &watchdogCC26XXObjects[LPSTK_WATCHDOG0],
        .hwAttrs     = &watchdogCC26XXHWAttrs[LPSTK_WATCHDOG0]
    },
};

const uint_least8_t Watchdog_count = LPSTK_WATCHDOGCOUNT;

/*
 *  Board-specific initialization function to disable external flash.
 *  This function is defined in the file LPSTK_fxns.c
 */
extern void Board_initHook(void);

/*
 *  ======== LPSTK_initGeneral ========
 */
void LPSTK_initGeneral(void)
{
    Power_init();

    if (PIN_init(BoardGpioInitTable) != PIN_SUCCESS) {
        /* Error with PIN_init */
        while (1);
    }

    /* Perform board-specific initialization */
    Board_initHook();
}

/*
 *  ======== Board_init ========
 */
void Board_init(void)
{
    LPSTK_initGeneral();
}
