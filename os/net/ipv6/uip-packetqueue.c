#include "net/ipv6/uip-packetqueue.h"
#include "lib/memb.h"
#include <stdio.h>

#define MAX_NUM_QUEUED_PACKETS 2
MEMB(packets_memb, struct uip_packetqueue_packet, MAX_NUM_QUEUED_PACKETS);

/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE  "Packet-Q"
#define LOG_LEVEL   LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
static void
packet_timedout(void *ptr)
{
  struct uip_packetqueue_handle *h = ptr;

  LOG_INFO("Timed out %p\n", h);
  memb_free(&packets_memb, h->packet);
  h->packet = NULL;
}
/*---------------------------------------------------------------------------*/
void
uip_packetqueue_new(struct uip_packetqueue_handle *handle)
{
  LOG_DBG("New %p\n", handle);
  handle->packet = NULL;
}
/*---------------------------------------------------------------------------*/
struct uip_packetqueue_packet *
uip_packetqueue_alloc(struct uip_packetqueue_handle *handle,
                      clock_time_t lifetime)
{
  LOG_DBG("Alloc %p\n", handle);
  if(handle->packet != NULL) {
    LOG_DBG("Alloced\n");
    return NULL;
  }
  handle->packet = memb_alloc(&packets_memb);
  if(handle->packet != NULL) {
    ctimer_set(&handle->packet->lifetimer, lifetime,
               packet_timedout, handle);
  } else {
    LOG_ERR("Alloc failed\n");
  }
  return handle->packet;
}
/*---------------------------------------------------------------------------*/
void
uip_packetqueue_free(struct uip_packetqueue_handle *handle)
{
  LOG_DBG("Free %p\n", handle);
  if(handle->packet != NULL) {
    ctimer_stop(&handle->packet->lifetimer);
    memb_free(&packets_memb, handle->packet);
    handle->packet = NULL;
  }
}
/*---------------------------------------------------------------------------*/
uint8_t *
uip_packetqueue_buf(const struct uip_packetqueue_handle *h)
{
  return h->packet != NULL ? h->packet->queue_buf: NULL;
}
/*---------------------------------------------------------------------------*/
uint16_t
uip_packetqueue_buflen(const struct uip_packetqueue_handle *h)
{
  return h->packet != NULL ? h->packet->queue_buf_len: 0;
}
/*---------------------------------------------------------------------------*/
void
uip_packetqueue_set_buflen(struct uip_packetqueue_handle *h, uint16_t len)
{
  if(h->packet != NULL) {
    h->packet->queue_buf_len = len;
  }
}
/*---------------------------------------------------------------------------*/
