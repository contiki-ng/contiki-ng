#ifndef NULLRADIO_H
#define NULLRADIO_H

#include "dev/radio.h"

radio_async_result_t nullradio_async_enter(void);
radio_async_result_t nullradio_async_prepare(
    uint8_t *payload,
    uint_fast16_t payload_len);
radio_async_result_t nullradio_async_reprepare(
    uint_fast16_t offset,
    uint8_t *patch,
    uint_fast16_t patch_len);
radio_async_result_t nullradio_async_transmit(
    bool shall_enter_rx_after_tx);
radio_async_result_t nullradio_async_on(void);
radio_async_result_t nullradio_async_off(void);
void nullradio_async_set_shr_callback(
    radio_shr_callback_t cb);
void nullradio_async_set_fifop_callback(
    radio_fifop_callback_t cb,
    uint_fast16_t threshold);
void nullradio_async_set_txdone_callback(radio_txdone_callback_t cb);
uint_fast16_t nullradio_async_read_phy_header(void);
radio_async_result_t nullradio_async_read_payload(
    uint8_t *buf,
    uint_fast16_t bytes);
uint_fast16_t nullradio_async_read_payload_bytes(void);
radio_async_result_t nullradio_async_prepare_sequence(
    uint8_t *sequence,
    uint_fast16_t sequence_len);
radio_async_result_t nullradio_async_append_to_sequence(
    uint8_t *appendix,
    uint_fast16_t appendix_len);
radio_async_result_t nullradio_async_transmit_sequence(void);
radio_async_result_t nullradio_async_finish_sequence(void);

extern const struct radio_driver nullradio_driver;

#endif /* NULLRADIO_H */
