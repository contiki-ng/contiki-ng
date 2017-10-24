#ifndef DTLS_SUPPORT_CONF_H_
#define DTLS_SUPPORT_CONF_H_

#undef HAVE_VPRINTF

#include "coap-endpoint.h"

typedef coap_endpoint_t session_t;

#include "sys/ctimer.h"
#include <stdint.h>

typedef struct {
  struct ctimer retransmit_timer;
} dtls_support_context_state_t;

#define DTLS_SUPPORT_CONF_CONTEXT_STATE dtls_support_context_state_t

#define DTLS_TICKS_PER_SECOND CLOCK_SECOND

typedef clock_time_t dtls_tick_t;

#define WITH_CONTIKI 1

#define HAVE_ASSERT_H 1

#endif /* DTLS_SUPPORT_CONF_H_ */
