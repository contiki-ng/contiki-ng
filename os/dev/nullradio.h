#ifndef NULLRADIO_H
#define NULLRADIO_H

#include "dev/radio.h"

void nullradio_async_enter(void);
void nullradio_async_prepare(uint8_t *length_then_payload);
void nullradio_async_reprepare(uint8_t offset, uint8_t *patch, uint8_t patch_len);
void nullradio_async_transmit(bool shall_enter_rx_after_tx);
void nullradio_async_on(void);
void nullradio_async_off(void);
void nullradio_async_set_shr_callback(radio_shr_callback_t cb);
void nullradio_async_set_fifop_callback(radio_fifop_callback_t cb, uint8_t threshold);
void nullradio_async_set_txdone_callback(radio_txdone_callback_t cb);
uint8_t nullradio_async_read_phy_header(void);
bool nullradio_async_read_payload(uint8_t *buf, uint8_t bytes);
uint8_t nullradio_async_read_payload_bytes(void);
void nullradio_async_prepare_sequence(uint8_t *sequence, uint8_t sequence_len);
void nullradio_async_append_to_sequence(uint8_t *appendix, uint8_t appendix_len);
void nullradio_async_transmit_sequence(void);
void nullradio_async_finish_sequence(void);

extern const struct radio_driver nullradio_driver;

#endif /* NULLRADIO_H */
