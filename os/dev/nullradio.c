#include "dev/nullradio.h"


/*---------------------------------------------------------------------------*/
static int
init(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
  return RADIO_TX_OK;
}
/*---------------------------------------------------------------------------*/
static int
send(const void *payload, unsigned short payload_len)
{
  prepare(payload, payload_len);
  return transmit(payload_len);
}
/*---------------------------------------------------------------------------*/
static int
radio_read(void *buf, unsigned short buf_len)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
void
nullradio_enter_async_mode(void)
{
}
/*---------------------------------------------------------------------------*/
void
nullradio_async_prepare(uint8_t *length_then_payload)
{
}
/*---------------------------------------------------------------------------*/
void
nullradio_async_transmit(int shall_enter_rx_after_tx)
{
}
/*---------------------------------------------------------------------------*/
void
nullradio_async_on(void)
{
}
/*---------------------------------------------------------------------------*/
void
nullradio_async_off(void)
{
}
/*---------------------------------------------------------------------------*/
void
nullradio_async_set_shr_callback(radio_shr_callback_t cb)
{
}
/*---------------------------------------------------------------------------*/
void
nullradio_async_set_fifop_callback(radio_fifop_callback_t cb, uint8_t threshold)
{
}
/*---------------------------------------------------------------------------*/
void
nullradio_async_set_txdone_callback(radio_txdone_callback_t cb)
{
}
/*---------------------------------------------------------------------------*/
uint8_t
nullradio_async_read_phy_header(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
uint8_t
nullradio_async_read_phy_header_to_packetbuf(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
int
nullradio_async_read_payload(uint8_t *buf, uint8_t bytes)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
int
nullradio_async_read_payload_to_packetbuf(uint8_t bytes)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
uint8_t
nullradio_async_remaining_payload_bytes(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
void
nullradio_async_prepare_sequence(uint8_t *sequence, uint8_t sequence_len)
{
}
/*---------------------------------------------------------------------------*/
void
nullradio_async_append_to_sequence(uint8_t *appendix, uint8_t appendix_len)
{
}
/*---------------------------------------------------------------------------*/
void
nullradio_async_transmit_sequence(void)
{
}
/*---------------------------------------------------------------------------*/
void
nullradio_async_finish_sequence(void)
{
}
/*---------------------------------------------------------------------------*/
const struct radio_driver nullradio_driver =
  {
    init,
    prepare,
    transmit,
    send,
    radio_read,
    channel_clear,
    receiving_packet,
    pending_packet,
    on,
    off,
    get_value,
    set_value,
    get_object,
    set_object,
    nullradio_enter_async_mode,
    nullradio_async_prepare,
    nullradio_async_transmit,
    nullradio_async_on,
    nullradio_async_off,
    nullradio_async_set_shr_callback,
    nullradio_async_set_fifop_callback,
    nullradio_async_set_txdone_callback,
    nullradio_async_read_phy_header,
    nullradio_async_read_phy_header_to_packetbuf,
    nullradio_async_read_payload,
    nullradio_async_read_payload_to_packetbuf,
    nullradio_async_remaining_payload_bytes,
    nullradio_async_prepare_sequence,
    nullradio_async_append_to_sequence,
    nullradio_async_transmit_sequence,
    nullradio_async_finish_sequence
  };
/*---------------------------------------------------------------------------*/
