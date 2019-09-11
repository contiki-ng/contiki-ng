#ifndef RF2XX_REGISTERMAP_H_
#define RF2XX_REGISTERMAP_H_


enum {
	RG_NOOP				= 0x00,
	RG_TRX_STATUS		= 0x01,
	RG_TRX_STATE		= 0x02,
	RG_TRX_CTRL_0		= 0x03,
	RG_TRX_CTRL_1		= 0x04,
	RG_PHY_TX_PWR		= 0x05,
	RG_PHY_RSSI			= 0x06,
	RG_PHY_ED_LEVEL		= 0x07,
	RG_PHY_CC_CCA		= 0x08,
	RG_CCA_THRES		= 0x09,
	RG_RX_CTRL			= 0x0A,
	RG_TRX_CTRL_2		= 0x0C,
	RG_IRQ_MASK			= 0x0E,
	RG_IRQ_STATUS		= 0x0F,
	RG_VREG_CTRL		= 0x10,
	RG_BATMON			= 0x11,
	RG_XOSC_CTRL		= 0x12,
	RG_CC_CTRL_0		= 0x13,
	RG_CC_CTRL_1		= 0x14,
	RG_RX_SYN			= 0x15,
	// 0x16
	RG_XAH_CTRL_1		= 0x17,
	RG_FTN_CTRL			= 0x18,
	// 0x19
	RG_PLL_CF			= 0x1A,
	RG_PLL_DCU			= 0x1B,
	RG_PART_NUM			= 0x1C,
	RG_VERSION_NUM		= 0x1D,
	RG_MAN_ID_0			= 0x1E,
	RG_MAN_ID_1			= 0x1F,
	RG_SHORT_ADDR_0		= 0x20,
	RG_SHORT_ADDR_1		= 0x21,
	RG_PAN_ID_0			= 0x22,
	RG_PAN_ID_1			= 0x23,
	RG_IEEE_ADDR_0		= 0x24,
	RG_IEEE_ADDR_1		= 0x25,
	RG_IEEE_ADDR_2		= 0x26,
	RG_IEEE_ADDR_3		= 0x27,
	RG_IEEE_ADDR_4		= 0x28,
	RG_IEEE_ADDR_5		= 0x29,
	RG_IEEE_ADDR_6		= 0x2A,
	RG_IEEE_ADDR_7		= 0x2B,
	RG_XAH_CTRL_0		= 0x2C,
	RG_CSMA_SEED_0		= 0x2D,
	RG_CSMA_SEED_1		= 0x2E,
	RG_CSMA_BE			= 0x2F,
	//
	RG_TST_CTRL_DIGI	= 0x36,
};


//#define RG_NOOP				(0x00)

//#define RG_TRX_STATUS		(0x01)
#define SR_TRX_STATUS		RG_TRX_STATUS, 0x1F, 0
#define SR_RESERVED_01_5	RG_TRX_STATUS, 0x20, 5
#define SR_CCA_STATUS		RG_TRX_STATUS, 0x40, 6
#define SR_CCA_DONE			RG_TRX_STATUS, 0x80, 7

enum {
	TRX_STATUS_P_ON					= 0x00,
	TRX_STATUS_BUSY_RX				= 0x01,
	TRX_STATUS_BUSY_TX				= 0x02,
	TRX_STATUS_RX_ON				= 0x06,
	TRX_STATUS_TRX_OFF				= 0x08,
	TRX_STATUS_PLL_ON				= 0x09, // TX_ON
	TRX_STATUS_TX_ON				= 0x09, // !typo (for consistency)
	TRX_STATUS_SLEEP				= 0x0F,
	TRX_STATUS_PREP_DEEP_SLEEP		= 0x10, // RF233-only state
	TRX_STATUS_BUSY_RX_AACK			= 0x11,
	TRX_STATUS_BUSY_TX_ARET			= 0x12,
	TRX_STATUS_RX_AACK_ON			= 0x16,
	TRX_STATUS_TX_ARET_ON			= 0x19,
	TRX_STATUS_RX_ON_NOCLK			= 0x1C, // *_NOCLK states removed in RF233
	TRX_STATUS_RX_AACK_ON_NOCLK		= 0x1D, // *_NOCLK states removed in RF233
	TRX_STATUS_BUSY_RX_AACK_NOCLK	= 0x1E, // *_NOCLK states removed in RF233
	TRX_STATUS_STATE_TRANSITION		= 0x1F,
};


//#define RG_TRX_STATE		(0x02)
#define SR_TRX_CMD			RG_TRX_STATE, 0x1F, 0
#define SR_TRAC_STATUS		RG_TRX_STATE, 0xE0, 5

enum {
	TRX_CMD_NOP				= 0x00,
	TRX_CMD_TX_START		= 0x02,
	TRX_CMD_FORCE_TRX_OFF	= 0x03,
	TRX_CMD_FORCE_PLL_ON	= 0x04,
	TRX_CMD_RX_ON			= 0x06,
	TRX_CMD_TRX_OFF			= 0x08,
	TRX_CMD_PLL_ON			= 0x09,
	TRX_CMD_TX_ON			= 0x09, // !typo (for consistency)
	TRX_CMD_PREP_DEEP_SLEEP = 0x10, // RF233-only feature
	TRX_CMD_RX_AACK_ON		= 0x16,
	TRX_CMD_TX_ARET_ON		= 0x19,
};

enum {
										// used by:
	TRAC_SUCCESS				= 0,	// AACK & ARET
	TRAC_SUCCESS_DATA_PENDING	= 1,	// ARET
	TRAC_SUCCESS_WAIT_FOR_ACK	= 2,	// AACK
	TRAC_CHANNEL_ACCESS_FAILURE	= 3,	// ARET
	TRAC_NO_ACK					= 5,	// ARET
	TRAC_INVALID				= 7,	// AACK & ARET
};


//#define RG_TRX_CTRL_0		(0x03)
#define SR_CLKM_CTRL		RG_TRX_CTRL_0, 0x07, 0
#define SR_CLKM_SHA_SEL		RG_TRX_CTRL_0, 0x08, 3
#define SR_PAD_IO_CLKM		RG_TRX_CTRL_0, 0x30, 4 // Removed in RF233
#define SR_PAD_IO			RG_TRX_CTRL_0, 0xC0, 6 // Removed in RF233


enum {
	CLKM_2mA			= 0,
	CLKM_4mA			= 1,
	CLKM_6mA			= 2,
	CLKM_8mA			= 3,
};


enum {
	CLKM_CTRL__DISABLED		= 0,
	CLKM_CTRL__1MHz			= 1,
	CLKM_CTRL__2MHz			= 2,
	CLKM_CTRL__4MHz			= 3,
	CLKM_CTRL__8MHz			= 4,
	CLKM_CTRL__16MHz		= 5,
	CLKM_CTRL__250kHz		= 6,
	CLKM_CTRL__62_5kHz		= 7,
};



//#define	RG_TRX_CTRL_1		(0x04)
#define SR_IRQ_POLARITY		RG_TRX_CTRL_1, 0x01, 0
#define SR_IRQ_MASK_MODE	RG_TRX_CTRL_1, 0x02, 1

enum {
	IRQ_POLARITY__HIGH_ACTIVE = 0,
	IRQ_POLARITY__LOW_ACTIVE = 1,
};

#define SR_SPI_CMD_MODE		RG_TRX_CTRL_1, 0x0C, 2
#define SR_RX_BL_CTRL		RG_TRX_CTRL_1, 0x10, 4
#define SR_TX_AUTO_CRC_ON	RG_TRX_CTRL_1, 0x20, 5
#define	SR_IRQ2_EXT_EN		RG_TRX_CTRL_1, 0x40, 6
#define SR_PA_EXT_EN		RG_TRX_CTRL_1, 0x80, 7

enum {
	SPI_CMD_MODE__NONE			= 0,
	SPI_CMD_MODE__TRX_STATUS	= 1,
	SPI_CMD_MODE__PHY_RSSI		= 2,
	SPI_CMD_MODE__IRQ_STATUS	= 3,
};


// This register differs quite a bit between radios
//#define RG_PHY_TX_PWR		(0x05)

// (AT86RF21x-only) configure PA_BOOST, GC_PA, TX_PWR at the same time
#define SR_TX_PWR_RF21x_ALL	RG_PHY_TX_PWR, 0xFF, 0

// (AT86RF21x-only) individual Tx parameters
#define SR_TX_PWR_RF21x		RG_PHY_TX_PWR, 0x1F, 0
#define SR_GC_PA			RG_PHY_TX_PWR, 0x60, 5
#define SR_PA_BOOST			RG_PHY_TX_PWR, 0x80, 7 // 5dB gain for price of side lobes

// (AT86RF23x-only) configure Tx power
#define SR_TX_PWR			RG_PHY_TX_PWR, 0x0F, 0
#define SR_PA_LT			RG_PHY_TX_PWR, 0x30, 4 // Removed in RF233
#define SR_PA_BUF_LT		RG_PHY_TX_PWR, 0xC0, 6 // Removed in RF233

enum {
	GC_PA__2_9dB = 0,	// -2.9dB
	GC_PA__1_3dB = 1,	// -1.3dB
	GC_PA__0_9dB = 2,	// -0.9dB
	GC_PA__0dB = 3,		// 0dB
};


//#define RG_PHY_RSSI			(0x06)
#define	SR_RSSI				RG_PHY_RSSI, 0x1F, 0
#define SR_RND_VALUE		RG_PHY_RSSI, 0x30, 5
#define SR_RX_CRC_VALID		RG_PHY_RSSI, 0x80, 7


//#define RG_PHY_ED_LEVEL		(0x07)
#define	SR_ED_LEVEL			RG_PHY_ED_LEVEL, 0xFF, 0


//#define RG_PHY_CC_CCA		(0x08)
#define SR_CHANNEL			RG_PHY_CC_CCA, 0x1F, 0
#define SR_CCA_MODE			RG_PHY_CC_CCA, 0x60, 5
#define SR_CCA_REQUEST		RG_PHY_CC_CCA, 0x80, 7

enum {
	CCA_MODE__CARRIER_SENSE_OR_ENERGY_THRESHOLD = 0,
	CCA_MODE__ENERGY_THRESHOLD = 1,
	CCA_MODE__CARRIER_SENSE = 2,
	CCA_MODE__CARRIER_SENSE_AND_ENERGY_THRESHOLD = 3,
};




//#define RG_CCA_THRES		(0x09)
#define SR_CCA_ED_THRES		RG_CCA_THRES, 0x0F, 0
#define SR_serverved_09_1	RG_CCA_THRES, 0xF0, 4


//#define RG_RX_CTRL			(0x0A)


//#define RG_TRX_CTRL_2		(0x0C)
#define SR_OQPSK_DATA_RATE	RG_TRX_CTRL_2, 0x03, 0
#define SR_reserverd_0C		RG_TRX_CTRL_2, 0x7C, 2
#define SR_RX_SAFE_MODE		RG_TRX_CTRL_2, 0x80, 7

enum {
	OQPSK_DATA_RATE_250 	= 0, // IEEE 802.15.4 compliant
	OQPSK_DATA_RATE_500		= 1,
	OQPSK_DATA_RATE_1000	= 2,
	OQPSK_DATA_RATE_2000	= 3,
};



//#define RG_IRQ_MASK			(0x0E)
#define SR_IRQ_MASK			RG_IRQ_MASK, 0xFF, 0

enum {
	IRQ0_PLL_LOCK		= 1 << 0,
	IRQ1_PLL_UNLOCK		= 1 << 1,
	IRQ2_RX_START		= 1 << 2,
	IRQ3_TRX_END		= 1 << 3,
	IRQ4_CCA_ED_DONE	= 1 << 4,
	IRQ4_AWAKE_END		= 1 << 4, // !typo; IRQ4 flag is for both CCA_ED_DONE and AWAKE_END
	IRQ5_AMI			= 1 << 5,
	IRQ6_TRX_UR			= 1 << 6,
	IRQ7_BAT_LOW		= 1 << 7,
};



//#define RG_IRQ_STATUS		(0x0F)
#define SR_IRQ_STATUS		RG_IRQ_STATUS, 0xFF, 0

#define SR_IRQ0_PLL_LOCK		RG_IRQ_STATUS, IRQ0_PLL_LOCK, 0
#define SR_IRQ1_PLL_UNLOCK		RG_IRQ_STATUS, IRQ1_PLL_UNLOCK, 1
#define SR_IRQ2_RX_START		RG_IRQ_STATUS, IRQ2_RX_START, 2
#define SR_IRQ3_TRX_END			RG_IRQ_STATUS, IRQ3_TRX_END, 3
#define SR_IRQ4_CCA_ED_DONE		RG_IRQ_STATUS, IRQ4_CCA_ED_DONE, 4
#define SR_IRQ4_AWAKE_END		RG_IRQ_STATUS, IRQ4_AWAKE_END, 4
#define SR_IRQ5_AMI				RG_IRQ_STATUS, IRQ5_AMI, 5
#define SR_IRQ6_TRX_UR			RG_IRQ_STATUS, IRQ6_TRX_UR, 6
#define SR_IRQ7_BAT_LOW			RG_IRQ_STATUS, IRQ7_BAT_LOW, 7



//#define RG_VREG_CTRL			(0x10)
#define SR_RESERVED_10_0		RG_VREG_CTRL, 0x03, 0
#define SR_DVDD_OK				RG_VREG_CTRL, 0x04, 2
#define SR_DVREG_EXT			RG_VREG_CTRL, 0x08, 3
#define SR_RESERVED_10_4		RG_VREG_CTRL, 0x30, 4
#define SR_AVDD_OK				RG_VREG_CTRL, 0x40, 6
#define	SR_AVREG_EXT			RG_VREG_CTRL, 0x80, 7


//#define RG_BATMON				(0x11)
#define SR_BATMON_VTH			RG_BATMON, 0x0F, 0
#define SR_BATMON_HR			RG_BATMON, 0x10, 4
#define SR_BATMON_OK			RG_BATMON, 0x20, 5
#define SR_RESERVED_11_6		RG_BATMON, 0xC0, 6

//#define RG_XOSC_CTRL			(0x12)
#define SR_XTAL_TRIM			RG_XOSC_CTRL, 0x0F, 0
#define SR_XTAL_MODE			RG_XOSC_CTRL, 0xF0, 4

enum {
	XTAL_MODE__INTERNAL_OSC		= 0xF,
	XTAL_MODE__EXTERNAL_OSC		= 0x4,
};


//#define RG_CC_CTRL_0			(0x13) // RF212-only register
#define SR_CC_NUMBER			RG_CC_CTRL_0, 0xFF, 0


//#define RG_CC_CTRL_1			(0x14) // RF212-only register
#define SR_CC_BAND				RG_CC_CTRL_1, 0x07, 0
#define SR_RESERVED_14_3		RG_CC_CTRL_1, 0xF8, 3


//#define RG_RX_SYN				(0x15)
#define SR_RX_PDT_LEVEL			RG_RX_SYN, 0x0F, 0
#define SR_RESERVED_15_4		RG_RX_SYN, 0x70, 4
#define SR_RX_PDT_DIS			RG_RX_SYN, 0x80, 7




//#define RG_XAH_CTRL_1			(0x17)
#define SR_AACK_PROM_MODE		RG_XAH_CTRL_1, 0x02, 1
#define SR_AACK_ACK_TIME		RG_XAH_CTRL_1, 0x04, 2
#define SR_AACK_UPLD_RES_FT		RG_XAH_CTRL_1, 0x10, 4
#define SR_AACK_FLTR_RES_FT		RG_XAH_CTRL_1, 0x20, 5


//#define RG_FTN_CTRL				(0x18)
#define SR_RESERVED_18_0		RG_FTN_CTRL, 0x7F, 0
#define	SR_FTN_START			RG_FTN_CTRL, 0x80, 7



//#define RG_PLL_CF				(0x1A)
#define SR_RESERVED_1A_0		RG_PLL_CF, 0x7F, 0
#define SR_PLL_CF_START			RG_PLL_CF, 0x80, 7

//#define RG_PLL_DCU				(0x1B)
#define SR_RESERVED_1B_0		RG_PLL_DCU, 0x7F, 0
#define	SR_PLL_DCU_START		RG_PLL_DCU, 0x80, 7


//#define RG_PART_NUM				(0x1C)
#define	SR_PART_NUM				RG_PART_NUM, 0xFF, 0


//#define RG_VERSION_NUM			(0x1D)
#define SR_VERSION_NUM			RG_VERSION_NUM, 0xFF, 0


//#define RG_MAN_ID_0				(0x1E)
#define SR_MAN_ID_0				RG_MAN_ID_0, 0xFF, 0

//#define RG_MAN_ID_1				(0x1F)
#define SR_MAN_ID_1				RG_MAN_ID_1, 0xFF, 0



//#define RG_SHORT_ADDR_0			(0x20)
#define SR_SHORT_ADDR_0			RG_SHORT_ADDR_0, 0xFF, 0

//#define RG_SHORT_ADDR_1			(0x21)
#define SR_SHORT_ADDR_1			RG_SHORT_ADDR_1, 0xFF, 0


//#define RG_PAN_ID_0				(0x22)
#define	SR_PAN_ID_0				RG_PAN_ID_0, 0xFF, 0

//#define RG_PAN_ID_1				(0x23)
#define	SR_PAN_ID_1				RG_PAN_ID_1, 0xFF, 0


//#define	RG_IEEE_ADDR_0			(0x24)
#define	SR_IEEE_ADDR_0			RG_IEEE_ADDR_0, 0xFF, 0

//#define RG_IEEE_ADDR_1			(0x25)
#define	SR_IEEE_ADDR_1			RG_IEEE_ADDR_1, 0xFF, 0

//#define RG_IEEE_ADDR_2			(0x26)
#define SR_IEEE_ADDR_2			RG_IEEE_ADDR_2, 0xFF, 0

//#define RG_IEEE_ADDR_3			(0x27)
#define SR_IEEE_ADDR_3			RG_IEEE_ADDR_3, 0xFF, 0

//#define RG_IEEE_ADDR_4			(0x28)
#define SR_IEEE_ADDR_4			RG_IEEE_ADDR_4, 0xFF, 0

//#define RG_IEEE_ADDR_5			(0x29)
#define SR_IEEE_ADDR_5			RG_IEEE_ADDR_5, 0xFF, 0

//#define RG_IEEE_ADDR_6			(0x2A)
#define SR_IEEE_ADDR_6			RG_IEEE_ADDR_6, 0xFF, 0

//#define RG_IEEE_ADDR_7			(0x2B)
#define SR_IEEE_ADDR_7			RG_IEEE_ADDR_7, 0xFF, 0


//#define RG_XAH_CTRL_0			(0x2C)
#define SR_SLOTTED_OPERATION	RG_XAH_CTRL_0, 0x01, 0
#define	SR_MAX_CSMA_RETRIES		RG_XAH_CTRL_0, 0x0E, 1
#define	SR_MAX_FRAME_RETRIES	RG_XAH_CTRL_0, 0xF0, 4


//#define RG_CSMA_SEED_0			(0x2D)
#define	SR_CSMA_SEED_0			RG_CSMA_SEED_0, 0xFF, 0

//#define RG_CSMA_SEED_1			(0x2E)
#define SR_CSMA_SEED_1			RG_CSMA_SEED_1, 0x07, 0
#define	SR_AACK_I_AM_COORD		RG_CSMA_SEED_1, 0x08, 3
#define SR_AACK_DIS_ACK			RG_CSMA_SEED_1, 0x10, 4
#define SR_AACK_SET_PD			RG_CSMA_SEED_1, 0x20, 5
#define	SR_AACK_FVN_MODE		RG_CSMA_SEED_1, 0xC0, 6


//#define RG_CSMA_BE				(0x2F)
#define SR_MAX_BE				RG_CSMA_BE, 0xF0, 4
#define SR_MIN_BE				RG_CSMA_BE, 0x0F, 0



//#define RG_TST_CTRL_DIGI		(0x36)


#endif
