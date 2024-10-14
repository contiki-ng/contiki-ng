/*
 * Copyright (c) 2023, ComLab, Jozef Stefan Institute - https://e6.ijs.si/
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
/**
 * \file
 *       Registermap for the AT86RF215
 * 
 *       Note that currently only registers for RF24 and BBC1 are defined.
 * 
 * \author
 *      Grega Morano <grega.morano@ijs.si>
*/

#ifndef AT86RF215_REGISTERMAP_H_
#define AT86RF215_REGISTERMAP_H_

/* Defines for SPI */
#define SPI_CMD_READ        (0x00)
#define SPI_CMD_WRITE       (0x80)

/* Registermap */
enum {
    RG_RF09_IRQS                   = (0x0000),
    RG_RF24_IRQS                   = (0x0001),
    RG_BBC0_IRQS                   = (0x0002),
    RG_BBC1_IRQS                   = (0x0003),
    RG_RF_RST                      = (0x0005),
    RG_RF_CFG                      = (0x0006),
    RG_RF_CLKO                     = (0x0007),
    RG_RF_BMDVC                    = (0x0008),
    RG_RF_XOC                      = (0x0009),
    RG_RF_IQIFC0                   = (0x000A),
    RG_RF_IQIFC1                   = (0x000B),
    RG_RF_IQIFC2                   = (0x000C),
    RG_RF_PN                       = (0x000D),
    RG_RF_VN                       = (0x000E),
    RG_RF09_IRQM                   = (0x0100),
    RG_RF09_AUXS                   = (0x0101),
    RG_RF09_STATE                  = (0x0102),
    RG_RF09_CMD                    = (0x0103),
    RG_RF09_CS                     = (0x0104),
    RG_RF09_CCF0L                  = (0x0105),
    RG_RF09_CCF0H                  = (0x0106),
    RG_RF09_CNL                    = (0x0107),
    RG_RF09_CNM                    = (0x0108),
    RG_RF09_RXBWC                  = (0x0109),
    RG_RF09_RXDFE                  = (0x010A),
    RG_RF09_AGCC                   = (0x010B),
    RG_RF09_AGCS                   = (0x010C),
    RG_RF09_RSSI                   = (0x010D),
    RG_RF09_EDC                    = (0x010E),
    RG_RF09_EDD                    = (0x010F),
    RG_RF09_EDV                    = (0x0110),
    RG_RF09_RNDV                   = (0x0111),
    RG_RF09_TXCUTC                 = (0x0112),
    RG_RF09_TXDFE                  = (0x0113),
    RG_RF09_PAC                    = (0x0114),
    RG_RF09_PADFE                  = (0x0116),
    RG_RF09_PLL                    = (0x0121),
    RG_RF09_PLLCF                  = (0x0122),
    RG_RF09_TXCI                   = (0x0125),
    RG_RF09_TXCQ                   = (0x0126),
    RG_RF09_TXDACI                 = (0x0127),
    RG_RF09_TXDACQ                 = (0x0128),
    RG_RF24_IRQM                   = (0x0200),
    RG_RF24_AUXS                   = (0x0201),
    RG_RF24_STATE                  = (0x0202),
    RG_RF24_CMD                    = (0x0203),
    RG_RF24_CS                     = (0x0204),
    RG_RF24_CCF0L                  = (0x0205),
    RG_RF24_CCF0H                  = (0x0206),
    RG_RF24_CNL                    = (0x0207),
    RG_RF24_CNM                    = (0x0208),
    RG_RF24_RXBWC                  = (0x0209),
    RG_RF24_RXDFE                  = (0x020A),
    RG_RF24_AGCC                   = (0x020B),
    RG_RF24_AGCS                   = (0x020C),
    RG_RF24_RSSI                   = (0x020D),
    RG_RF24_EDC                    = (0x020E),
    RG_RF24_EDD                    = (0x020F),
    RG_RF24_EDV                    = (0x0210),
    RG_RF24_RNDV                   = (0x0211),
    RG_RF24_TXCUTC                 = (0x0212),
    RG_RF24_TXDFE                  = (0x0213),
    RG_RF24_PAC                    = (0x0214),
    RG_RF24_PADFE                  = (0x0216),
    RG_RF24_PLL                    = (0x0221),
    RG_RF24_PLLCF                  = (0x0222),
    RG_RF24_TXCI                   = (0x0225),
    RG_RF24_TXCQ                   = (0x0226),
    RG_RF24_TXDACI                 = (0x0227),
    RG_RF24_TXDACQ                 = (0x0228),
    RG_BBC0_IRQM                   = (0x0300),
    RG_BBC0_PC                     = (0x0301),
    RG_BBC0_PS                     = (0x0302),
    RG_BBC0_RXFLL                  = (0x0304),
    RG_BBC0_RXFLH                  = (0x0305),
    RG_BBC0_TXFLL                  = (0x0306),
    RG_BBC0_TXFLH                  = (0x0307),
    RG_BBC0_FBLL                   = (0x0308),
    RG_BBC0_FBLH                   = (0x0309),
    RG_BBC0_FBLIL                  = (0x030A),
    RG_BBC0_FBLIH                  = (0x030B),
    RG_BBC0_OFDMPHRTX              = (0x030C),
    RG_BBC0_OFDMPHRRX              = (0x030D),
    RG_BBC0_OFDMC                  = (0x030E),
    RG_BBC0_OFDMSW                 = (0x030F),
    RG_BBC0_OQPSKC0                = (0x0310),
    RG_BBC0_OQPSKC1                = (0x0311),
    RG_BBC0_OQPSKC2                = (0x0312),
    RG_BBC0_OQPSKC3                = (0x0313),
    RG_BBC0_OQPSKPHRTX             = (0x0314),
    RG_BBC0_OQPSKPHRRX             = (0x0315),
    RG_BBC0_AFC0                   = (0x0320),
    RG_BBC0_AFC1                   = (0x0321),
    RG_BBC0_AFFTM                  = (0x0322),
    RG_BBC0_AFFVM                  = (0x0323),
    RG_BBC0_AFS                    = (0x0324),
    RG_BBC0_MACEA0                 = (0x0325),
    RG_BBC0_MACEA1                 = (0x0326),
    RG_BBC0_MACEA2                 = (0x0327),
    RG_BBC0_MACEA3                 = (0x0328),
    RG_BBC0_MACEA4                 = (0x0329),
    RG_BBC0_MACEA5                 = (0x032A),
    RG_BBC0_MACEA6                 = (0x032B),
    RG_BBC0_MACEA7                 = (0x032C),
    RG_BBC0_MACPID0F0              = (0x032D),
    RG_BBC0_MACPID1F0              = (0x032E),
    RG_BBC0_MACSHA0F0              = (0x032F),
    RG_BBC0_MACSHA1F0              = (0x0330),
    RG_BBC0_MACPID0F1              = (0x0331),
    RG_BBC0_MACPID1F1              = (0x0332),
    RG_BBC0_MACSHA0F1              = (0x0333),
    RG_BBC0_MACSHA1F1              = (0x0334),
    RG_BBC0_MACPID0F2              = (0x0335),
    RG_BBC0_MACPID1F2              = (0x0336),
    RG_BBC0_MACSHA0F2              = (0x0337),
    RG_BBC0_MACSHA1F2              = (0x0338),
    RG_BBC0_MACPID0F3              = (0x0339),
    RG_BBC0_MACPID1F3              = (0x033A),
    RG_BBC0_MACSHA0F3              = (0x033B),
    RG_BBC0_MACSHA1F3              = (0x033C),
    RG_BBC0_AMCS                   = (0x0340),
    RG_BBC0_AMEDT                  = (0x0341),
    RG_BBC0_AMAACKPD               = (0x0342),
    RG_BBC0_AMAACKTL               = (0x0343),
    RG_BBC0_AMAACKTH               = (0x0344),
    RG_BBC0_FSKC0                  = (0x0360),
    RG_BBC0_FSKC1                  = (0x0361),
    RG_BBC0_FSKC2                  = (0x0362),
    RG_BBC0_FSKC3                  = (0x0363),
    RG_BBC0_FSKC4                  = (0x0364),
    RG_BBC0_FSKPLL                 = (0x0365),
    RG_BBC0_FSKSFD0L               = (0x0366),
    RG_BBC0_FSKSFD0H               = (0x0367),
    RG_BBC0_FSKSFD1L               = (0x0368),
    RG_BBC0_FSKSFD1H               = (0x0369),
    RG_BBC0_FSKPHRTX               = (0x036A),
    RG_BBC0_FSKPHRRX               = (0x036B),
    RG_BBC0_FSKRPC                 = (0x036C),
    RG_BBC0_FSKRPCONT              = (0x036D),
    RG_BBC0_FSKRPCOFFT             = (0x036E),
    RG_BBC0_FSKRRXFLL              = (0x0370),
    RG_BBC0_FSKRRXFLH              = (0x0371),
    RG_BBC0_FSKDM                  = (0x0372),
    RG_BBC0_FSKPE0                 = (0x0373),
    RG_BBC0_FSKPE1                 = (0x0374),
    RG_BBC0_FSKPE2                 = (0x0375),
    RG_BBC0_PMUC                   = (0x0380),
    RG_BBC0_PMUVAL                 = (0x0381),
    RG_BBC0_PMUQF                  = (0x0382),
    RG_BBC0_PMUI                   = (0x0383),
    RG_BBC0_PMUQ                   = (0x0384),
    RG_BBC0_CNTC                   = (0x0390),
    RG_BBC0_CNT0                   = (0x0391),
    RG_BBC0_CNT1                   = (0x0392),
    RG_BBC0_CNT2                   = (0x0393),
    RG_BBC0_CNT3                   = (0x0394),
    RG_BBC1_IRQM                   = (0x0400),
    RG_BBC1_PC                     = (0x0401),
    RG_BBC1_PS                     = (0x0402),
    RG_BBC1_RXFLL                  = (0x0404),
    RG_BBC1_RXFLH                  = (0x0405),
    RG_BBC1_TXFLL                  = (0x0406),
    RG_BBC1_TXFLH                  = (0x0407),
    RG_BBC1_FBLL                   = (0x0408),
    RG_BBC1_FBLH                   = (0x0409),
    RG_BBC1_FBLIL                  = (0x040A),
    RG_BBC1_FBLIH                  = (0x040B),
    RG_BBC1_OFDMPHRTX              = (0x040C),
    RG_BBC1_OFDMPHRRX              = (0x040D),
    RG_BBC1_OFDMC                  = (0x040E),
    RG_BBC1_OFDMSW                 = (0x040F),
    RG_BBC1_OQPSKC0                = (0x0410),
    RG_BBC1_OQPSKC1                = (0x0411),
    RG_BBC1_OQPSKC2                = (0x0412),
    RG_BBC1_OQPSKC3                = (0x0413),
    RG_BBC1_OQPSKPHRTX             = (0x0414),
    RG_BBC1_OQPSKPHRRX             = (0x0415),
    RG_BBC1_AFC0                   = (0x0420),
    RG_BBC1_AFC1                   = (0x0421),
    RG_BBC1_AFFTM                  = (0x0422),
    RG_BBC1_AFFVM                  = (0x0423),
    RG_BBC1_AFS                    = (0x0424),
    RG_BBC1_MACEA0                 = (0x0425),
    RG_BBC1_MACEA1                 = (0x0426),
    RG_BBC1_MACEA2                 = (0x0427),
    RG_BBC1_MACEA3                 = (0x0428),
    RG_BBC1_MACEA4                 = (0x0429),
    RG_BBC1_MACEA5                 = (0x042A),
    RG_BBC1_MACEA6                 = (0x042B),
    RG_BBC1_MACEA7                 = (0x042C),
    RG_BBC1_MACPID0F0              = (0x042D),
    RG_BBC1_MACPID1F0              = (0x042E),
    RG_BBC1_MACSHA0F0              = (0x042F),
    RG_BBC1_MACSHA1F0              = (0x0430),
    RG_BBC1_MACPID0F1              = (0x0431),
    RG_BBC1_MACPID1F1              = (0x0432),
    RG_BBC1_MACSHA0F1              = (0x0433),
    RG_BBC1_MACSHA1F1              = (0x0434),
    RG_BBC1_MACPID0F2              = (0x0435),
    RG_BBC1_MACPID1F2              = (0x0436),
    RG_BBC1_MACSHA0F2              = (0x0437),
    RG_BBC1_MACSHA1F2              = (0x0438),
    RG_BBC1_MACPID0F3              = (0x0439),
    RG_BBC1_MACPID1F3              = (0x043A),
    RG_BBC1_MACSHA0F3              = (0x043B),
    RG_BBC1_MACSHA1F3              = (0x043C),
    RG_BBC1_AMCS                   = (0x0440),
    RG_BBC1_AMEDT                  = (0x0441),
    RG_BBC1_AMAACKPD               = (0x0442),
    RG_BBC1_AMAACKTL               = (0x0443),
    RG_BBC1_AMAACKTH               = (0x0444),
    RG_BBC1_FSKC0                  = (0x0460),
    RG_BBC1_FSKC1                  = (0x0461),
    RG_BBC1_FSKC2                  = (0x0462),
    RG_BBC1_FSKC3                  = (0x0463),
    RG_BBC1_FSKC4                  = (0x0464),
    RG_BBC1_FSKPLL                 = (0x0465),
    RG_BBC1_FSKSFD0L               = (0x0466),
    RG_BBC1_FSKSFD0H               = (0x0467),
    RG_BBC1_FSKSFD1L               = (0x0468),
    RG_BBC1_FSKSFD1H               = (0x0469),
    RG_BBC1_FSKPHRTX               = (0x046A),
    RG_BBC1_FSKPHRRX               = (0x046B),
    RG_BBC1_FSKRPC                 = (0x046C),
    RG_BBC1_FSKRPCONT              = (0x046D),
    RG_BBC1_FSKRPCOFFT             = (0x046E),
    RG_BBC1_FSKRRXFLL              = (0x0470),
    RG_BBC1_FSKRRXFLH              = (0x0471),
    RG_BBC1_FSKDM                  = (0x0472),
    RG_BBC1_FSKPE0                 = (0x0473),
    RG_BBC1_FSKPE1                 = (0x0474),
    RG_BBC1_FSKPE2                 = (0x0475),
    RG_BBC1_PMUC                   = (0x0480),
    RG_BBC1_PMUVAL                 = (0x0481),
    RG_BBC1_PMUQF                  = (0x0482),
    RG_BBC1_PMUI                   = (0x0483),
    RG_BBC1_PMUQ                   = (0x0484),
    RG_BBC1_CNTC                   = (0x0490),
    RG_BBC1_CNT0                   = (0x0491),
    RG_BBC1_CNT1                   = (0x0492),
    RG_BBC1_CNT2                   = (0x0493),
    RG_BBC1_CNT3                   = (0x0494),

    RG_BBC0_FBRXS                  = (0x2000),
    RG_BBC0_FBRXE                  = (0x27FE),
    RG_BBC0_FBTXS                  = (0x2800),
    RG_BBC0_FBTXE                  = (0x2FFE),
    RG_BBC1_FBRXS                  = (0x3000),
    RG_BBC1_FBRXE                  = (0x37FE),
    RG_BBC1_FBTXS                  = (0x3800),
    RG_BBC1_FBTXE                  = (0x3FFE),
};

/*---------------------------------------------------------*/

/*---------------------------------------------------------*/
//#define RG_RF_RST                             (0x0005)

//#define RG_RF_IQIFC1                          (0x000B)
#define SR_SKEWDRV              RG_RF_IQIFC1, 0x03, 0
#define SR_CHPM                 RG_RF_IQIFC1, 0x70, 4
#define SR_FAILSF               RG_RF_IQIFC1, 0x80, 7


/*---------------------------------------------------------*/
/* State machine */
/*---------------------------------------------------------*/
//#define RG_RF24_STATE                         (0x0202)
#define SR_RF24_STATE           RG_RF24_STATE, 0x07, 0
enum {
    RF_STATE_TRXOFF       = 0x2,
    RF_STATE_TXPREP       = 0x3,
    RF_STATE_TX           = 0x4,
    RF_STATE_RX           = 0x5,
    RF_STATE_TRANSITION   = 0x6,
    RF_STATE_RESET        = 0x7
};

//#define RG_RF24_CMD                           (0x0203)
#define SR_RF24_CMD             RG_RF24_CMD, 0x07, 0
enum {
    RF_CMD_NOP          = 0x0,
    RF_CMD_SLEEP        = 0x1,
    RF_CMD_TRXOFF       = 0x2,
    RF_CMD_TXPREP       = 0x3,
    RF_CMD_TX           = 0x4,
    RF_CMD_RX           = 0x5,
    RF_CMD_RESET        = 0x7
};


/*---------------------------------------------------------*/
/* IRQ registers */
/*---------------------------------------------------------*/
//#define RG_RF_CFG                             (0x0006)
#define SR_DRV                  RG_RF_CFG, 0x03, 0
#define SR_IRQP                 RG_RF_CFG, 0x04, 2
#define SR_IRQMM                RG_RF_CFG, 0x08, 3

//#define RG_RF24_IRQM                           (0x0200)
#define SR_RF24_IRQ_MASK        RG_RF24_IRQM, 0x3F, 0
enum {
    IRQ1_WAKEUP              = 1 << 0,
    IRQ2_TRXRDY              = 1 << 1,
    IRQ3_EDC                 = 1 << 2,
    IRQ4_BATLOW              = 1 << 3,
    IRQ5_TRXERR              = 1 << 4,
    IRQ6_IQIFSF              = 1 << 5,
};
//#define RG_RF24_IRQS                           (0x0001)
#define SR_RF24_IRQ_STATUS      RG_RF24_IRQS, 0x3F, 0

//#define RG_BBC1_IRQM                           (0x0400)
#define SR_BBC1_IRQ_MASK        RG_BBC1_IRQM, 0xFF, 0
enum {
    IRQ1_RXFS                = 1 << 0,
    IRQ2_RXFE                = 1 << 1,
    IRQ3_RXAM                = 1 << 2,
    IRQ4_RXEM                = 1 << 3,
    IRQ5_TXFE                = 1 << 4,
    IRQ6_AGCH                = 1 << 5,
    IRQ7_AGCR                = 1 << 6,
    IRQ8_FBLI                = 1 << 7
};

//#dfine RG_BBC1_IRQS                           (0x0003)
#define SR_BBC1_IRQ_STATUS      RG_BBC1_IRQS, 0xFF, 0


/*---------------------------------------------------------*/
/* Output power config */
/*---------------------------------------------------------*/
//#define RG_RF24_PAC                           (0x0214)
#define SR_RF24_TXPWR           RG_RF24_PAC, 0x1F, 0
#define SR_RF24_PACUR           RG_RF24_PAC, 0x60, 5

//#define RG_RF24_RSSI
#define SR_RF24_RSSI            RG_RF24_RSSI, 0xFF, 0

/*---------------------------------------------------------*/
/* Frequency registers */
/*---------------------------------------------------------*/
//#define RG_RF24_CS
#define SR_RF24_CS              RG_RF24_CS, 0xFF, 0

//#define RG_RF24_CCF0L
#define SR_RF24_CCF0L           RG_RF24_CCF0L, 0xFF, 0

//#define RG_RF24_CCF0H
#define SR_RF24_CCF0H           RG_RF24_CCF0H, 0xFF, 0

//#define RG_RF24_CNL
#define SR_RF24_CNL             RG_RF24_CNL, 0xFF, 0

//#define RG_RF24_CNM
#define SR_RF24_CHN             RG_RF24_CNM, 0x01, 0
#define SR_RF24_CM              RG_RF24_CNM, 0xC0, 6

enum {
    CHANNEL_MODE_IEEE_COMPLIANT     = 0x0,
    //
    CHANNEL_MODE_09_FINE_RESOLUTION = 0x2,
    CHANNEL_MODE_24_FINE_RESOLUTION = 0x3,
};


/*---------------------------------------------------------*/
/* PHY control (PC) */
/*---------------------------------------------------------*/
//#define RG_BBC1_PC                            (0x0401)
#define SR_BBC1_CTX             RG_BBC1_PC, 0x80, 7
#define SR_BBC1_FCSFE           RG_BBC1_PC, 0x40, 6
#define SR_BBC1_FCSOK           RG_BBC1_PC, 0x20, 5
#define SR_BBC1_TXAFCS          RG_BBC1_PC, 0x10, 4
#define SR_BBC1_FCST            RG_BBC1_PC, 0x08, 3
#define SR_BBC1_BBEN            RG_BBC1_PC, 0x04, 2
#define SR_BBC1_PT              RG_BBC1_PC, 0x03, 0

enum {
    PHY_TYPE_BB_PHYOFF      = 0x0,
    PHY_TYPE_BB_MRFSK       = 0x1,
    PHY_TYPE_BB_MROFDM      = 0x2,
    PHY_TYPE_BB_MROQPSK     = 0x3
};


/*---------------------------------------------------------*/
/* Auto modes (AACK, CCATX and TX2RX) */
/*---------------------------------------------------------*/
//#define RG_BBC1_AMCS                          (0x0)
#define SR_BBC1_AACKFT          RG_BBC1_AMCS, 0x80, 7
#define SR_BBC1_AACKFA          RG_BBC1_AMCS, 0x40, 6
#define SR_BBC1_AACKDR          RG_BBC1_AMCS, 0x20, 5
#define SR_BBC1_AACKS           RG_BBC1_AMCS, 0x10, 4
#define SR_BBC1_AACK            RG_BBC1_AMCS, 0x80, 3
#define SR_BBC1_CCAED           RG_BBC1_AMCS, 0x04, 2
#define SR_BBC1_CCATX           RG_BBC1_AMCS, 0x02, 1
#define SR_BBC1_TX2RX           RG_BBC1_AMCS, 0x01, 0


/*---------------------------------------------------------*/
/* Phase Measurement Unit */
/*---------------------------------------------------------*/
//#define RG_BBC1_PMUC                           (0x0480)
#define SR_BBC1_PMU_EN              RG_BBC1_PMUC, 0x01, 0
#define SR_BBC1_PMU_AVG             RG_BBC1_PMUC, 0x02, 1
#define SR_BBC1_PMU_SYNC            RG_BBC1_PMUC, 0x1C, 2
#define SR_BBC1_PMU_FED             RG_BBC1_PMUC, 0x20, 5
#define SR_BBC1_PMU_IQSEL           RG_BBC1_PMUC, 0x40, 6
#define SR_BBC1_PMU_CCFTS           RG_BBC1_PMUC, 0x80, 7

#endif /* AT86RF215_REGISTERMAP_H_ */
