#ifndef RF2XX_H_
#define RF2XX_H_

#include "contiki-net.h"

// String for debug purposes
#ifndef AT86RF2XX_BOARD_STRING
#define AT86RF2XX_BOARD_STRING "Unknown"
#endif


// If driver is built for Contiki's 6Tisch implementation
#if MAC_CONF_WITH_TSCH
#define RF2XX_CONF_AACK  (0)
#define RF2XX_CONF_ARET  (0)
#define RF2XX_CONF_CCA   (0)
#define RF2XX_CONF_POLLING_MODE (1)
#endif


// Default log level
#ifndef LOG_CONF_LEVEL_RF2XX
#define LOG_LEVEL_RF2XX     (LOG_LEVEL_WARN)
#else
#define LOG_LEVEL_RF2XX     (LOG_CONF_LEVEL_RF2XX)
#endif

// Collect statistics on radio operations
#ifndef RF2XX_CONF_STATS
#define RF2XX_STATS     (0)
#else
#define RF2XX_STATS     (RF2XX_CONF_STATS)
#endif

// Enable radio's auto acknowledge capabilities (extended mode)
#ifndef RF2XX_CONF_AACK
#define RF2XX_AACK   (1)
#else
#define RF2XX_AACK   (RF2XX_CONF_AACK)
#endif

// Enable radio's auto retransmission capabilities (extended mode)
#ifndef RF2XX_CONF_ARET
#define RF2XX_ARET  (1)
#else
#define RF2XX_ARET  (RF2XX_CONF_ARET)
#endif

// Enable radio's automatic carrier sense
#ifndef RF2XX_CONF_CCA
#define RF2XX_CCA   (1)
#else
#define RF2XX_CCA   (RF2XX_CONF_CCA)
#endif

// Number of CSMA retries 0-5, 6 = reserved, 7 = immediately without CSMA/CA
#ifndef RF2XX_CONF_CSMA_RETRIES
#define RF2XX_CSMA_RETRIES  (5)
#else
#if RF2XX_CONF_CSMA_RETRIES < 0 || RF2XX_CONF_CSMA_RETRIES > 5
#error "Invalid RF2XX_CONF_CSMA_RETRIES"
#endif
#define RF2XX_CSMA_RETRIES  (RF2XX_CONF_CSMA_RETRIES)
#endif

// Number of frame retries, if no ACK, 0-15 (TX_ARET-only)
#ifndef RF2XX_CONF_FRAME_RETRIES
#define RF2XX_FRAME_RETRIES     (15)
#else
#define RF2XX_FRAME_RETRIES     (RF2XX_CONF_FRAME_RETRIES)
#endif

// Will CRC16-CITT be offloaded to the radio? 
#ifndef RF2XX_CONF_CHECKSUM
#define RF2XX_CHECKSUM  (1)
#else
#define RF2XX_CHECKSUM  (RF2XX_CONF_CHECKSUM)
#endif

// Skip radio's address filter (AACK only)
#ifndef RF2XX_CONF_PROMISCOUS_MODE
#define RF2XX_PROMISCOUS_MODE   (0)
#else
#define RF2XX_PROMISCOUS_MODE   (RF2XX_CONF_PROMISCOUS_MODE)
#endif

#ifndef RF2XX_CONF_POLLING_MODE
#define RF2XX_POLLING_MODE   (0)
#else
#define RF2XX_POLLING_MODE   (RF2XX_CONF_POLLING_MODE)
#endif


// AT86RF2xx driver for Contiki(-NG)
extern const struct radio_driver rf2xx_driver;

// Radio driver API
int rf2xx_init(void);

int rf2xx_prepare(const void *payload, unsigned short payload_len);
int rf2xx_transmit(unsigned short payload_len);
int rf2xx_send(const void *payload, unsigned short payload_len);
int rf2xx_read(void *buf, unsigned short buf_len);
int rf2xx_cca(void);
int rf2xx_receiving_packet(void);
int rf2xx_pending_packet(void);
int rf2xx_on(void);
int rf2xx_off(void);

// Interrupt routine function
void rf2xx_isr(void);


enum {
	rxDetected,
	rxAddrMatch,
	rxSuccess,
	rxToStack,

	txCount,
	txSuccess,
	txCollision,
	txNoAck,

	RF2XX_STATS_COUNT
};


#if RF2XX_STATS
extern volatile uint32_t rf2xxStats[RF2XX_STATS_COUNT];
#define RF2XX_STATS_GET(event)		rf2xxStats[event]
#define RF2XX_STATS_ADD(event)		rf2xxStats[event]++
#define RF2XX_STATS_RESET()    		memset(rf2xxStats, 0, sizeof(rf2xxStats[0]) * RF2XX_STATS_COUNT)
#endif

#define RF2XX_STATS_GET(event)		(0)
#define RF2XX_STATS_ADD(event)
#define RF2XX_STATS_RESET()

#endif