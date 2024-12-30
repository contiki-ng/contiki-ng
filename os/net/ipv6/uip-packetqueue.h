#ifndef UIP_PACKETQUEUE_H
#define UIP_PACKETQUEUE_H

#include "sys/ctimer.h"
#include "net/ipv6/uip.h"
#include <stdint.h>

/*---------------------------------------------------------------------------*/
struct uip_packetqueue_handle;

struct uip_packetqueue_packet {
  uint8_t queue_buf[UIP_BUFSIZE];
  uint16_t queue_buf_len;
  struct ctimer lifetimer;
};

struct uip_packetqueue_handle {
  struct uip_packetqueue_packet *packet;
};

/*---------------------------------------------------------------------------*/
void uip_packetqueue_new(struct uip_packetqueue_handle *handle);
struct uip_packetqueue_packet *uip_packetqueue_alloc(
    struct uip_packetqueue_handle *handle, clock_time_t lifetime);
void uip_packetqueue_free(struct uip_packetqueue_handle *handle);
uint8_t *uip_packetqueue_buf(const struct uip_packetqueue_handle *h);
uint16_t uip_packetqueue_buflen(const struct uip_packetqueue_handle *h);
void uip_packetqueue_set_buflen(struct uip_packetqueue_handle *h, uint16_t len);
/*---------------------------------------------------------------------------*/
#endif /* UIP_PACKETQUEUE_H */
