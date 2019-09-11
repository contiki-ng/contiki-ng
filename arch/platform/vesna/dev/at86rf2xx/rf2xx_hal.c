#include <stdio.h>
#include <string.h>

#include "sys/log.h"
#include "net/packetbuf.h"
#include "net/netstack.h"
#include "sys/energest.h"
#include "sys/rtimer.h"

#include "sys/critical.h"

#include "stm32f10x_rcc.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "vsnspi_new.h"

#include "rf2xx_registermap.h"
#include "rf2xx_hal.h"
#include "rf2xx.h"
#include "rf2xx_arch.h"

#if AT86RF2XX_BOARD_SNR
#include "vsnsetup.h"
#endif

#define LOG_MODULE  "rf2xxHAL"
#define LOG_LEVEL   LOG_LEVEL_RF2XX


// Register access
#define CMD_REG_MASK	((uint8_t)0x3F)
#define CMD_REG_ACCESS	((uint8_t)0x80)

// Frame Buffer access
#define CMD_FB_MASK		((uint8_t)0x1F)
#define CMD_FB_ACCESS	((uint8_t)0x20)

// SRAM acess
#define CMD_SR_MASK		((uint8_t)0x1F)
#define CMD_SR_ACCESS	((uint8_t)0x00)

// R/W operation bit
#define CMD_READ		((uint8_t)0x00)
#define CMD_WRITE		((uint8_t)0x40)


#define DEFAULT_IRQ_MASK    (IRQ2_RX_START | IRQ3_TRX_END | IRQ4_CCA_ED_DONE | IRQ5_AMI)


// EXTI (interrupt) struct (from STM) and constant (immutable) pointer to it.
static EXTI_InitTypeDef EXTI_rf2xxStructure = {
    .EXTI_Line = EXTI_IRQ_LINE,
    .EXTI_Mode = EXTI_Mode_Interrupt,
    .EXTI_Trigger = EXTI_Trigger_Rising,
    .EXTI_LineCmd = ENABLE,
};
EXTI_InitTypeDef * const rf2xxEXTI = &EXTI_rf2xxStructure;


// SPI struct (from VESNA drivers) and constant (immutable) pointer to it.
static vsnSPI_CommonStructure SPI_rf2xxStructure;
vsnSPI_CommonStructure * const rf2xxSPI = &SPI_rf2xxStructure;


// These settings are passed as pointer and they have to exist for the runtime
// and does not change. That is why struct is `static`.
static SPI_InitTypeDef rf2xxSpiConfig = {
    .SPI_Direction = SPI_Direction_2Lines_FullDuplex,
    .SPI_Mode = SPI_Mode_Master,
    .SPI_DataSize = SPI_DataSize_8b,
    .SPI_CPOL = SPI_CPOL_Low,
    .SPI_CPHA = SPI_CPHA_1Edge,
    .SPI_NSS = SPI_NSS_Soft,
    .SPI_FirstBit = SPI_FirstBit_MSB,
    .SPI_CRCPolynomial = 7,
};

static void
spiErrorCallback(void *cbDevStruct)
{
	vsnSPI_CommonStructure *spi = cbDevStruct;
	vsnSPI_chipSelect(spi, SPI_CS_HIGH);
	LOG_ERR("SPI error callback triggered\n");
}

// SPI struct (from VESNA drivers) for CC1101 radio
static vsnSPI_CommonStructure CC1101_SPI_Structure;
vsnSPI_CommonStructure * const CC1101_SPI = &CC1101_SPI_Structure;

static void
CC1101_spiErrorCallback(void *cbDevStruct)
{
	vsnSPI_CommonStructure *spi = cbDevStruct;
	vsnSPI_chipSelect(spi, SPI_CS_HIGH);
	LOG_ERR("SPI error callback triggered for CC radio\n");
}




void
rf2xx_initHW(void)
{
	// If AT86RF2xx radio is on SNR board
	#if AT86RF2XX_BOARD_SNR
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	#endif

	// If AT86RF2xx radio is on ISTMV v1.0 board
	#if AT86RF2XX_BOARD_ISMTV_V1_0
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

        // Quirk: Disable JTAG since it share pin with RST (fixed in ISMTV v1.1 hardware)
        LOG_INFO("Disable JTAG in 5 seconds\n");
        vsnTime_delayS(5);
		GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	#endif

	// If AT86RF2xx radio is on ISTMV v1.1 board
	#if AT86RF2XX_BOARD_ISMTV_V1_1
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	#endif

	// Initializes only GPIOs of SPI port
	vsnSPI_initHW(SPI_PORT);

    // Configure CC1101 clock --> for rTimer trigger 
    #if (AT86RF2XX_BOARD_ISMTV_V1_0 || AT86RF2XX_BOARD_ISMTV_V1_1)
        configure_cc1101();
    #endif

    // Complete initialization of SPI for rf2xx
    vsnSPI_initCommonStructure(
        rf2xxSPI,
        SPI_PORT,
        &rf2xxSpiConfig,
        CSN_PIN,
        CSN_PORT,
        RF2XX_SPI_SPEED
    );

	// register error callback
	vsnSPI_Init(rf2xxSPI, spiErrorCallback);

    // GPIO init and common settings
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;

	// IRQ (input)
	GPIO_InitStructure.GPIO_Pin = IRQ_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(IRQ_PORT, &GPIO_InitStructure);

	// Important. I forgot this line and ISR was triggered by UART1 output signal.
	GPIO_EXTILineConfig(EXTI_IRQ_PORT, EXTI_IRQ_PIN);

	// SLP_TR (output)
	GPIO_InitStructure.GPIO_Pin = SLP_TR_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(SLP_TR_PORT, &GPIO_InitStructure);

	// RSTn (output)
	GPIO_InitStructure.GPIO_Pin = RSTN_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(RSTN_PORT, &GPIO_InitStructure);

	setRST(); // hold radio in reset state
	clearCS(); // clear chip select (default)
	clearSLPTR(); // prevent going to sleep
	clearEXTI(); // clear interrupt flag
}





void
rf2xx_reset(void)
{
    uint8_t dummy __attribute__((unused));
    uint8_t partNum;

    LOG_DBG("%s\n", __func__);

	setRST(); // hold radio in reset state


    disableEXTI();

	clearCS(); // clear chip select (default)
	clearSLPTR(); // prevent going to sleep
	clearEXTI(); // clear interrupt flag
    clearRST(); // release radio from RESET state

	// Print for what pin layout was compiled.
	LOG_INFO("Compiled for " AT86RF2XX_BOARD_STRING "\n");

    // Radio indentification procedure
    do {
        LOG_DBG("Detecting AT86RF2xx radio\n");

        // Match JEDEC manufacturer ID
        if (regRead(RG_MAN_ID_0) == RF2XX_MAN_ID_0 && regRead(RG_MAN_ID_1) == RF2XX_MAN_ID_1) {
            LOG_DBG("JEDEC ID matches Atmel\n");

            // Match known radio (and sanitize radio type)
            partNum = regRead(RG_PART_NUM);
            switch (partNum) {
                case RF2XX_AT86RF233:
                case RF2XX_AT86RF231:
                case RF2XX_AT86RF230:
                //case RF2XX_AT86RF212:
                    rf2xxChip = partNum;
                default:
                    break;
            }
        }
    } while (RF2XX_UNDEFINED == rf2xxChip);

    // Radio indentified. Now, put it into TRX_OFF state.
    do {
        regWrite(RG_TRX_STATE, TRX_CMD_FORCE_TRX_OFF);
    } while (TRX_STATUS_TRX_OFF != bitRead(SR_TRX_STATUS));

	// CLKM clock change visible (0 = immediately, 1 = needs to go through SLEEP cycle)
	bitWrite(SR_CLKM_SHA_SEL, 0);

#if AT86RF2XX_BOARD_SNR
	// Enable CLKM (as output) so it can be used as external clock source for VESNA
	bitWrite(SR_CLKM_CTRL, CLKM_CTRL__8MHz);
#else
    // Disable CLKM since it's not connected anywhere (ISMTV)
    bitWrite(SR_CLKM_CTRL, CLKM_CTRL__DISABLED);
#endif

	// Enable/disable Tx autogenerating CRC16/CCITT
	bitWrite(SR_TX_AUTO_CRC_ON, RF2XX_CHECKSUM);

	// Enable RX_SAFE mode to protect buffer while reading it
	bitWrite(SR_RX_SAFE_MODE, 1);

	// Set same value for RF231 (default=0) and RF233 (default=1)
	bitWrite(SR_IRQ_MASK_MODE, 1);

	// Number of CSMA retries (part of IEEE 802.15.4)
	// Possible values [0 - 5], 6 is reserved, 7 will send immediately (no CCA)
	bitWrite(SR_MAX_CSMA_RETRIES, RF2XX_CCA ? RF2XX_CSMA_RETRIES : 7);

	// Number of maximum TX_ARET frame retries
	// Possible values [0 - 15]
	bitWrite(SR_MAX_FRAME_RETRIES, RF2XX_FRAME_RETRIES);

	// Highest allowed backoff exponent
	regWrite(RG_CSMA_BE, 0x80);

    // Randomize backoff timing
	// Upper two RSSI reg bits are random
	regWrite(RG_CSMA_SEED_0, regRead(RG_PHY_RSSI));

	// First returned byte will be IRQ_STATUS;
	bitWrite(SR_SPI_CMD_MODE, SPI_CMD_MODE__IRQ_STATUS);

	// Configure Promiscuous mode (AACK-mode only)
	bitWrite(SR_AACK_PROM_MODE, RF2XX_PROMISCOUS_MODE);
	bitWrite(SR_AACK_UPLD_RES_FT, RF2XX_PROMISCOUS_MODE);
	bitWrite(SR_AACK_FLTR_RES_FT, RF2XX_PROMISCOUS_MODE);

	// Enable only specific IRQs
	regWrite(RG_IRQ_MASK, DEFAULT_IRQ_MASK);

	// Read IRQ register to clear it
	dummy = regRead(RG_IRQ_STATUS);

	// Clear any interrupt pending
	clearEXTI();
    enableEXTI();
}


inline static vsnSPI_ErrorStatus
REGREAD(uint8_t addr, uint8_t *value)
{
    vsnSPI_ErrorStatus status;
    int_master_status_t intStatus;

    // Clear chip-select if it was not cleared
    status = clearCS();
    if (status != VSN_SPI_SUCCESS) goto error; // goto is considered bad practice, however it is OK for small cases

    intStatus = critical_enter();

    // start SPI communication
    status = setCS();
    if (status != VSN_SPI_SUCCESS) goto error;

    status = vsnSPI_pullByteTXRX(rf2xxSPI, ((addr & CMD_REG_MASK) | CMD_REG_ACCESS | CMD_READ), value);
    if (status != VSN_SPI_SUCCESS) goto error;

    status = vsnSPI_pullByteTXRX(rf2xxSPI, 0x00, value);
    if (status != VSN_SPI_SUCCESS) goto error;

error:
    clearCS();
    critical_exit(intStatus);
    if (status != VSN_SPI_SUCCESS) LOG_WARN("register read error (0x%02x)\n", addr);
    return status;
}


inline static vsnSPI_ErrorStatus
REGWRITE(uint8_t addr, const uint8_t value)
{
    vsnSPI_ErrorStatus status;
    int_master_status_t intStatus;
    uint8_t dummy __attribute__((unused));

    status = clearCS();
    if (status != VSN_SPI_SUCCESS) goto error; // goto is considered bad practice, however it is OK for small cases

    intStatus = critical_enter();

    // start SPI communication
    status = setCS();
    if (status != VSN_SPI_SUCCESS) goto error;

    status = vsnSPI_pullByteTXRX(rf2xxSPI, ((addr & CMD_REG_MASK) | CMD_REG_ACCESS | CMD_WRITE), &dummy);
    if (status != VSN_SPI_SUCCESS) goto error;

    status |= vsnSPI_pullByteTXRX(rf2xxSPI, value, &dummy);
    if (status != VSN_SPI_SUCCESS) goto error;

error:
    clearCS();
    critical_exit(intStatus);
    if (status != VSN_SPI_SUCCESS) LOG_WARN("register write error (0x%02x)\n", addr);
    return status;
}


inline static vsnSPI_ErrorStatus
FIFOREAD(rxFrame_t *frame)
{
    vsnSPI_ErrorStatus status;
    int_master_status_t intStatus;
    rf2xx_irq_t irq;
    uint8_t dummy __attribute__((unused));

    status = clearCS();
    if (status != VSN_SPI_SUCCESS) goto error;

    // critical section
    intStatus = critical_enter();

    status = setCS();
    if (status != VSN_SPI_SUCCESS) goto error;

    status = vsnSPI_pullByteTXRX(rf2xxSPI, (CMD_FB_ACCESS | CMD_READ), &irq.value);
    if (status != VSN_SPI_SUCCESS) goto error;

    if (irq.IRQ6_TRX_UR) goto error;

    status = vsnSPI_pullByteTXRX(rf2xxSPI, 0x00, &frame->len);
    if (status != VSN_SPI_SUCCESS) goto error;

    if (frame->len < 3 || frame->len > RF2XX_MAX_FRAME_SIZE) {
        goto error;
    }

    frame->len -= RF2XX_CRC_SIZE;
    frame->crc = (uint16_t *)(frame->content + frame->len);

    for (uint8_t i = 0; i < frame->len + RF2XX_CRC_SIZE; i++) {
        status = vsnSPI_pullByteTXRX(rf2xxSPI, 0x00, frame->content + i);
        if (status != VSN_SPI_SUCCESS) goto error;
    }

    status = vsnSPI_pullByteTXRX(rf2xxSPI, 0x00, &frame->lqi);
    if (status != VSN_SPI_SUCCESS) goto error;

    // AT86RF233 adds another byte, which is RSSI value
    if (rf2xxChip == RF2XX_AT86RF233) {
        status = vsnSPI_pullByteTXRX(rf2xxSPI, 0x00, (uint8_t *)&frame->rssi);
        if (status != VSN_SPI_SUCCESS) goto error;
    }
    
    status = clearCS();
    if (status != VSN_SPI_SUCCESS) goto error;

    if (rf2xxChip != RF2XX_AT86RF233) {
        status = REGREAD(RG_PHY_ED_LEVEL, (uint8_t *)&frame->rssi);
        if (status != VSN_SPI_SUCCESS) goto error;
    }

    critical_exit(intStatus);

    frame->rssi += -91; // RSSI_BASE_VAL

    return status;

error:
    clearCS();
    frame->len = 0;
    critical_exit(intStatus);
    LOG_WARN("Frame read error\n");
    return status;
}


inline static vsnSPI_ErrorStatus
FIFOWRITE(txFrame_t *frame)
{
    vsnSPI_ErrorStatus status;
    int_master_status_t intStatus;
    uint8_t dummy __attribute__((unused));

    status = clearCS();
    if (status != VSN_SPI_SUCCESS) goto error;

    intStatus = critical_enter();

    status = setCS();
    if (status != VSN_SPI_SUCCESS) goto error;

    status = vsnSPI_pullByteTXRX(rf2xxSPI, (CMD_FB_ACCESS | CMD_WRITE), &dummy);
    if (status != VSN_SPI_SUCCESS) goto error;

    status = vsnSPI_pullByteTXRX(rf2xxSPI, frame->len + RF2XX_CRC_SIZE, &dummy);
    if (status != VSN_SPI_SUCCESS) goto error;

    for (uint8_t i = 0; i < frame->len + RF2XX_CRC_SIZE; i++) {
        status = vsnSPI_pullByteTXRX(rf2xxSPI, frame->content[i], &dummy);
        if (status != VSN_SPI_SUCCESS) goto error;
    }

error:
    clearCS();
    critical_exit(intStatus);
    if (status != VSN_SPI_SUCCESS) LOG_WARN("Frame write error\n");
    return status;
}

uint8_t
regRead(uint8_t addr)
{
    uint8_t value;
    while (VSN_SPI_SUCCESS != REGREAD(addr, &value));
    return value;
}


void
regWrite(uint8_t addr, const uint8_t value)
{
    while (VSN_SPI_SUCCESS != REGWRITE(addr, value));
}

uint8_t
bitRead(uint8_t addr, uint8_t mask, uint8_t offset)
{
    uint8_t value;

    while(VSN_SPI_SUCCESS != REGREAD(addr, &value));
    value = (value & mask) >> offset;
    return value;
}

void
bitWrite(uint8_t addr, uint8_t mask, uint8_t offset, uint8_t value)
{
    uint8_t tmp;

    while (VSN_SPI_SUCCESS != REGREAD(addr, &tmp));
    tmp = (tmp & ~mask) | ((value << offset) & mask);
    while (VSN_SPI_SUCCESS != REGWRITE(addr, tmp));
}


int
frameRead(rxFrame_t *frame)
{
    vsnSPI_ErrorStatus status = FIFOREAD(frame);
    return status;
}


int
frameWrite(txFrame_t *frame)
{
    vsnSPI_ErrorStatus status = FIFOWRITE(frame);
    return status;
}


/// Access I/O functions
vsnSPI_ErrorStatus
clearCS(void)
{
    return vsnSPI_chipSelect(rf2xxSPI, SPI_CS_HIGH);
}

vsnSPI_ErrorStatus
setCS(void)
{
    return vsnSPI_chipSelect(rf2xxSPI, SPI_CS_LOW);
}

uint8_t
getCS(void)
{
    return !GPIO_ReadInputDataBit(CSN_PORT, CSN_PIN);
}


void
clearRST(void)
{
    GPIO_SetBits(RSTN_PORT, RSTN_PIN);
}

void
setRST(void)
{
    GPIO_ResetBits(RSTN_PORT, RSTN_PIN);
}

uint8_t
getRST(void)
{
    return !GPIO_ReadInputDataBit(RSTN_PORT, RSTN_PIN);
}



void
clearSLPTR(void)
{
    GPIO_ResetBits(SLP_TR_PORT, SLP_TR_PIN);
}

void
setSLPTR(void)
{
    GPIO_SetBits(SLP_TR_PORT, SLP_TR_PIN);
}

uint8_t
getSLPTR(void)
{
    return GPIO_ReadInputDataBit(SLP_TR_PORT, SLP_TR_PIN);
}


void
clearEXTI(void)
{
    EXTI_ClearFlag(EXTI_IRQ_LINE);
}

void
disableEXTI(void)
{
    rf2xxEXTI->EXTI_LineCmd = DISABLE;
    EXTI_Init(rf2xxEXTI);
}

void
enableEXTI(void)
{
    rf2xxEXTI->EXTI_LineCmd = ENABLE;
    EXTI_Init(rf2xxEXTI);
}


// Set clock output on pin GDO0 of radio CC1101 to be 13.5 MHz
// For other possible freq see radio datasheet
void
configure_cc1101(void)
{
    // These settings are passed as pointer and they have to exist for the runtime
    // and does not change. That is why struct is `static`.
	static SPI_InitTypeDef spiConfig = {
		//.SPI_BaudRatePrescaler is overwritten later
		.SPI_Direction = SPI_Direction_2Lines_FullDuplex,
		.SPI_Mode = SPI_Mode_Master,
		.SPI_DataSize = SPI_DataSize_8b,
		.SPI_CPOL = SPI_CPOL_Low,
		.SPI_CPHA = SPI_CPHA_1Edge,
		.SPI_NSS = SPI_NSS_Soft,
		.SPI_FirstBit = SPI_FirstBit_MSB,
		.SPI_CRCPolynomial = 7,
	};

    vsnSPI_initCommonStructure(
        CC1101_SPI,
        SPI_PORT,
        &spiConfig,
        CC1101_CSN_PIN,
        CC1101_CSN_PORT,
        RF2XX_SPI_SPEED     //speed can be the same as rf2xx
    );

    // Init SPI for CC radio
    vsnSPI_Init(CC1101_SPI, CC1101_spiErrorCallback);

    // Set GDO0 output pin to desire freq (0x32 = 13.5MHz)
        CC1101_set_CS();

        // Radio must be in IDLE state
        if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6) == 0){
            CC1101_regWrite(0x02, 0x32);
        }
        else{
            // Frist reset the radio and then set the freq
            CC1101_reset();
            CC1101_regWrite(0x02, 0x32);
        }
        CC1101_clear_CS();

        LOG_INFO("GDO0 output freq set to 13.5 MHz\n");

    // Give SPI control back to rf2xx
    vsnSPI_deInit(CC1101_SPI);
}

void
CC1101_regWrite(uint8_t addr, uint8_t value)
{
    vsnSPI_ErrorStatus CC_status;
    uint8_t dummy __attribute__((unused));
    uint8_t state;

    //CC1101_clear_CS();
    CC1101_set_CS();

        int count = 0;			
			while((GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)) && (count < 200000))	
					count++;										
			if(count >= 200000){ 							
				LOG_ERR("CC1101_regWrite: MISO is not low! \n");											
			}	

    CC_status = vsnSPI_pullByteTXRX(CC1101_SPI, addr , &state);
        if (VSN_SPI_SUCCESS != CC_status) LOG_WARN("ERR while sending\n");
        LOG_DBG("regWrite address state is 0x%02x  \n",state);

    CC_status = vsnSPI_pullByteTXRX(CC1101_SPI, value, &state);
        if (VSN_SPI_SUCCESS != CC_status) LOG_WARN("ERR while receiving\n");
        LOG_DBG("regWrite data state is 0x%02x  \n",state);

    CC1101_clear_CS();        
}

void
CC1101_reset(void)
{
    uint8_t dummy __attribute__((unused));

    //SCLK = 1 and MOSI = 0
	GPIO_SetBits(GPIOA, GPIO_Pin_5);
	GPIO_ResetBits(GPIOA, GPIO_Pin_7);

    //Strobe CS low/high
	GPIO_SetBits(GPIOB, GPIO_Pin_9);
	vsnTime_delayUS(10);
	GPIO_ResetBits(GPIOB, GPIO_Pin_9);
	vsnTime_delayUS(10); 

    //Hold CS low for at least 40us
	GPIO_SetBits(GPIOB, GPIO_Pin_9);
	vsnTime_delayUS(100); 

    //Pull CS low and wait for MISO to go low
	GPIO_ResetBits(GPIOB, GPIO_Pin_9);

	int count = 0;
			while((GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)) && (count < 200000))	
					count++;										
			if(count >= 200000){ 							
				LOG_ERR("CC1101_reset: MISO is not low!\n");												
			}		

	vsnTime_delayUS(300);

    CC1101_set_CS();
       
        count = 0;			
			while((GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)) && (count < 200000))	
					count++;										
			if(count >= 200000){ 							
				LOG_ERR("CC1101_reset: MISO is not low! \n");											
			}

    //Isue SRES (0x30) strobe on MOSI line
    vsnSPI_pullByteTXRX(CC1101_SPI, 0x30 , &dummy);

    vsnTime_delayUS(50);
    
    CC1101_clear_CS();
	
    //When MISO goes low again, reset is complete
        count = 0;    
			while((GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)) && (count < 500000))	
					count++;										
			if(count >= 500000){ 							
				LOG_ERR("CC1101_reset: MISO is not low!\n");												
			}	

	vsnTime_delayUS(300);

    LOG_INFO("CC1101 Radio reset complete!\n");
}

void
CC1101_clear_CS(void)
{
    vsnSPI_chipSelect(CC1101_SPI, SPI_CS_HIGH);
}

void
CC1101_set_CS(void)
{
    vsnSPI_chipSelect(CC1101_SPI, SPI_CS_LOW);
}