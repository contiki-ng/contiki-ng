/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "contiki.h"
#include "net/linkaddr.h"
#include "net/mac/mac.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"
#include "shell.h"
#include "shell-commands.h"
#include "nrf54l15-radio-debug.h"

#include "sys/log.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define RADIO_TEST_MAGIC          0x54a315acUL
#define RADIO_TEST_DEFAULT_LEN    50
#define RADIO_TEST_DEFAULT_MS     1000
#define RADIO_TEST_DEFAULT_MAX_TX 1
#define RADIO_TEST_MAX_LEN        96

struct radio_test_hdr {
  uint32_t magic;
  uint32_t seq;
};

struct radio_test_state {
  bool running;
  bool verbose;
  bool target_set;
  bool run_limited;
  bool tx_busy;
  bool timer_dirty;
  bool tx_event_pending;
  bool rx_event_pending;
  uint8_t max_transmissions;
  uint16_t payload_len;
  uint16_t pending_tx;
  uint32_t interval_ms;
  uint32_t run_remaining;
  uint32_t next_seq;
  uint32_t tx_started;
  uint32_t tx_ok;
  uint32_t tx_noack;
  uint32_t tx_collision;
  uint32_t tx_err;
  uint32_t tx_no_target;
  uint32_t rx_ok;
  uint32_t rx_bad_len;
  uint32_t rx_bad_magic;
  uint32_t last_tx_seq;
  uint32_t last_rx_seq;
  int last_tx_status;
  int last_tx_attempts;
  int16_t last_rx_rssi;
  uint16_t last_rx_lqi;
  linkaddr_t target;
  linkaddr_t last_rx_src;
};

static struct radio_test_state state;
static uint8_t tx_payload[RADIO_TEST_MAX_LEN];

PROCESS(radio_test_process, "nRF radio test");
AUTOSTART_PROCESSES(&radio_test_process);

static const char *status_to_str(int status);
static bool parse_u32(const char *str, uint32_t *value);
static bool parse_i32(const char *str, int32_t *value);
static bool parse_lladdr(const char *str, linkaddr_t *addr);
static void print_status(shell_output_func output);
static void print_status_brief(shell_output_func output);
static void queue_tx_request(uint16_t count);
static void update_timer(struct etimer *periodic_timer, bool *timer_active);
static void try_send_packet(void);

static PT_THREAD(cmd_radio_test(struct pt *pt, shell_output_func output, char *args));

static const struct shell_command_t radio_test_commands[] = {
  {
    "radio-test",
    cmd_radio_test,
    "'> radio-test': status | status-brief | target <mac> | clear-target | start | run <count> | stop | once | interval <ms> | len <bytes> | txmax <n> | channel [n] | power [dbm] | verbose 0/1 | reset"
  },
  { NULL, NULL, NULL }
};

static struct shell_command_set_t radio_test_shell_set = {
  .next = NULL,
  .commands = radio_test_commands,
};
/*---------------------------------------------------------------------------*/
static void
reset_stats(void)
{
  state.pending_tx = 0;
  state.next_seq = 0;
  state.tx_started = 0;
  state.tx_ok = 0;
  state.tx_noack = 0;
  state.tx_collision = 0;
  state.tx_err = 0;
  state.tx_no_target = 0;
  state.rx_ok = 0;
  state.rx_bad_len = 0;
  state.rx_bad_magic = 0;
  state.last_tx_seq = 0;
  state.last_rx_seq = 0;
  state.last_tx_status = -1;
  state.last_tx_attempts = 0;
  state.last_rx_rssi = 0;
  state.last_rx_lqi = 0;
  memset(&state.last_rx_src, 0, sizeof(state.last_rx_src));
}
/*---------------------------------------------------------------------------*/
static void
init_state(void)
{
  memset(&state, 0, sizeof(state));
  state.payload_len = RADIO_TEST_DEFAULT_LEN;
  state.interval_ms = RADIO_TEST_DEFAULT_MS;
  state.max_transmissions = RADIO_TEST_DEFAULT_MAX_TX;
  state.run_limited = false;
  state.run_remaining = 0;
  state.last_tx_status = -1;
}
/*---------------------------------------------------------------------------*/
static const char *
status_to_str(int status)
{
  switch(status) {
  case MAC_TX_OK:
    return "OK";
  case MAC_TX_NOACK:
    return "NOACK";
  case MAC_TX_COLLISION:
    return "COLLISION";
  case MAC_TX_DEFERRED:
    return "DEFERRED";
  case MAC_TX_ERR:
    return "ERR";
  case MAC_TX_ERR_FATAL:
    return "ERR_FATAL";
  case MAC_TX_QUEUE_FULL:
    return "QUEUE_FULL";
  default:
    return "UNKNOWN";
  }
}
/*---------------------------------------------------------------------------*/
static int
hex_nibble(char c)
{
  if(c >= '0' && c <= '9') {
    return c - '0';
  }
  c = (char)tolower((unsigned char)c);
  if(c >= 'a' && c <= 'f') {
    return 10 + (c - 'a');
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
static bool
parse_u32(const char *str, uint32_t *value)
{
  char *end = NULL;
  unsigned long parsed;

  if(str == NULL || *str == '\0') {
    return false;
  }

  parsed = strtoul(str, &end, 0);
  if(*end != '\0') {
    return false;
  }

  *value = (uint32_t)parsed;
  return true;
}
/*---------------------------------------------------------------------------*/
static bool
parse_i32(const char *str, int32_t *value)
{
  char *end = NULL;
  long parsed;

  if(str == NULL || *str == '\0') {
    return false;
  }

  parsed = strtol(str, &end, 0);
  if(*end != '\0') {
    return false;
  }

  *value = (int32_t)parsed;
  return true;
}
/*---------------------------------------------------------------------------*/
static bool
parse_lladdr(const char *str, linkaddr_t *addr)
{
  uint8_t parsed[LINKADDR_SIZE];
  int high = -1;
  int index = 0;

  memset(parsed, 0, sizeof(parsed));

  while(*str != '\0') {
    int value = hex_nibble(*str);

    if(value >= 0) {
      if(high < 0) {
        high = value;
      } else {
        if(index >= LINKADDR_SIZE) {
          return false;
        }
        parsed[index++] = (high << 4) | value;
        high = -1;
      }
    } else if(*str != '.' && *str != ':' && *str != '-' && *str != ' ') {
      return false;
    }

    str++;
  }

  if(high >= 0 || index != LINKADDR_SIZE) {
    return false;
  }

  memcpy(addr->u8, parsed, sizeof(parsed));
  return true;
}
/*---------------------------------------------------------------------------*/
static void
print_status(shell_output_func output)
{
  radio_value_t tx_power = 0;
  radio_value_t tx_power_max = 0;
  radio_value_t tx_power_min = 0;
  radio_value_t channel = 0;

  SHELL_OUTPUT(output, "Local MAC: ");
  shell_output_lladdr(output, &linkaddr_node_addr);
  SHELL_OUTPUT(output, "\n");

  SHELL_OUTPUT(output, "Target: ");
  if(state.target_set) {
    shell_output_lladdr(output, &state.target);
  } else {
    output("(unset)");
  }
  SHELL_OUTPUT(output, "\n");

  if(NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &tx_power) == RADIO_RESULT_OK &&
     NETSTACK_RADIO.get_value(RADIO_CONST_TXPOWER_MIN, &tx_power_min) == RADIO_RESULT_OK &&
     NETSTACK_RADIO.get_value(RADIO_CONST_TXPOWER_MAX, &tx_power_max) == RADIO_RESULT_OK) {
    SHELL_OUTPUT(output, "TX power: %d dBm (range %d..%d)\n",
                 tx_power, tx_power_min, tx_power_max);
  }

  if(NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &channel) == RADIO_RESULT_OK) {
    SHELL_OUTPUT(output, "Channel: %d\n", channel);
  }

  SHELL_OUTPUT(output,
               "Running=%u verbose=%u limited=%u remaining=%" PRIu32
               " tx-busy=%u pending=%u interval-ms=%" PRIu32
               " len=%u txmax=%u\n",
               state.running, state.verbose, state.run_limited,
               state.run_remaining, state.tx_busy, state.pending_tx,
               state.interval_ms, state.payload_len, state.max_transmissions);

  SHELL_OUTPUT(output,
               "TX started/ok/noack/coll/err/no-target: %" PRIu32 "/%" PRIu32
               "/%" PRIu32 "/%" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
               state.tx_started, state.tx_ok, state.tx_noack, state.tx_collision,
               state.tx_err, state.tx_no_target);

  if(state.last_tx_status >= 0) {
    SHELL_OUTPUT(output,
                 "Last TX: seq=%" PRIu32 " status=%s attempts=%d\n",
                 state.last_tx_seq, status_to_str(state.last_tx_status),
                 state.last_tx_attempts);
  }

  SHELL_OUTPUT(output,
               "RX ok/bad-len/bad-magic: %" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
               state.rx_ok, state.rx_bad_len, state.rx_bad_magic);

  if(state.rx_ok > 0) {
    SHELL_OUTPUT(output, "Last RX: seq=%" PRIu32 " from ", state.last_rx_seq);
    shell_output_lladdr(output, &state.last_rx_src);
    SHELL_OUTPUT(output, " RSSI=%d LQI=%u\n",
                 state.last_rx_rssi, state.last_rx_lqi);
  }

  SHELL_OUTPUT(output,
               "Driver rx/ack-req/arm-ok/arm-fail/tx-start/tx-done: %" PRIu32
               "/%" PRIu32 "/%" PRIu32 "/%" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
               nrf54l15_radio_debug.rx_frame_ok,
               nrf54l15_radio_debug.ack_requested,
               nrf54l15_radio_debug.ack_tx_arm_ok,
               nrf54l15_radio_debug.ack_tx_arm_fail,
               nrf54l15_radio_debug.ack_tx_started,
               nrf54l15_radio_debug.ack_tx_done);

  SHELL_OUTPUT(output,
               "Driver ack-rx-start/valid/invalid/timeout/delay-short: %" PRIu32
               "/%" PRIu32 "/%" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
               nrf54l15_radio_debug.ack_rx_started,
               nrf54l15_radio_debug.ack_rx_valid,
               nrf54l15_radio_debug.ack_rx_invalid,
               nrf54l15_radio_debug.ack_timeout,
               nrf54l15_radio_debug.ack_delay_too_short);

  SHELL_OUTPUT(output,
               "Driver ack-path irq/core/txreq cc: %" PRIu32
               "/%" PRIu32 "/%" PRIu32 "\n",
               nrf54l15_radio_debug.last_ack_irq_entry_cc,
               nrf54l15_radio_debug.last_ack_core_entry_cc,
               nrf54l15_radio_debug.last_ack_before_txreq_cc);

  SHELL_OUTPUT(output,
               "Driver last-ack delay/ramp/now/fem/armed: %" PRIu32
               "/%" PRIu32 "/%" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
               nrf54l15_radio_debug.last_ack_delay_us,
               nrf54l15_radio_debug.last_ack_ramp_up_cc,
               nrf54l15_radio_debug.last_ack_now_cc,
               nrf54l15_radio_debug.last_ack_fem_cc,
               nrf54l15_radio_debug.last_ack_arm_result);
}
/*---------------------------------------------------------------------------*/
static void
print_status_brief(shell_output_func output)
{
  radio_value_t tx_power = 0;
  radio_value_t channel = 0;

  output("RTSTAT");

  output(" local=");
  shell_output_lladdr(output, &linkaddr_node_addr);

  output(" target=");
  if(state.target_set) {
    shell_output_lladdr(output, &state.target);
  } else {
    output("unset");
  }

  SHELL_OUTPUT(output,
               " target_set=%u running=%u verbose=%u run_limited=%u"
               " run_remaining=%" PRIu32 " tx_busy=%u pending=%u"
               " interval_ms=%" PRIu32 " len=%u txmax=%u",
               state.target_set, state.running, state.verbose,
               state.run_limited, state.run_remaining, state.tx_busy,
               state.pending_tx, state.interval_ms, state.payload_len,
               state.max_transmissions);

  if(NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &tx_power) == RADIO_RESULT_OK) {
    SHELL_OUTPUT(output, " power=%d", tx_power);
  }

  if(NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &channel) == RADIO_RESULT_OK) {
    SHELL_OUTPUT(output, " channel=%d", channel);
  }

  SHELL_OUTPUT(output,
               " tx_started=%" PRIu32 " tx_ok=%" PRIu32 " tx_noack=%" PRIu32
               " tx_collision=%" PRIu32 " tx_err=%" PRIu32
               " tx_no_target=%" PRIu32,
               state.tx_started, state.tx_ok, state.tx_noack,
               state.tx_collision, state.tx_err, state.tx_no_target);

  SHELL_OUTPUT(output, " last_tx_seq=%" PRIu32, state.last_tx_seq);
  if(state.last_tx_status >= 0) {
    SHELL_OUTPUT(output, " last_tx_status=%s last_tx_attempts=%d",
                 status_to_str(state.last_tx_status),
                 state.last_tx_attempts);
  } else {
    SHELL_OUTPUT(output, " last_tx_status=unset last_tx_attempts=0");
  }

  SHELL_OUTPUT(output,
               " rx_ok=%" PRIu32 " rx_bad_len=%" PRIu32
               " rx_bad_magic=%" PRIu32,
               state.rx_ok, state.rx_bad_len, state.rx_bad_magic);

  SHELL_OUTPUT(output, " last_rx_seq=%" PRIu32, state.last_rx_seq);
  if(state.rx_ok > 0) {
    output(" last_rx_src=");
    shell_output_lladdr(output, &state.last_rx_src);
    SHELL_OUTPUT(output, " last_rx_rssi=%d last_rx_lqi=%u",
                 state.last_rx_rssi, state.last_rx_lqi);
  } else {
    output(" last_rx_src=unset last_rx_rssi=0 last_rx_lqi=0");
  }

  SHELL_OUTPUT(output,
               " drv_rx_ok=%" PRIu32
               " drv_ack_req=%" PRIu32
               " drv_ack_arm_ok=%" PRIu32
               " drv_ack_arm_fail=%" PRIu32
               " drv_ack_tx_started=%" PRIu32
               " drv_ack_tx_done=%" PRIu32,
               nrf54l15_radio_debug.rx_frame_ok,
               nrf54l15_radio_debug.ack_requested,
               nrf54l15_radio_debug.ack_tx_arm_ok,
               nrf54l15_radio_debug.ack_tx_arm_fail,
               nrf54l15_radio_debug.ack_tx_started,
               nrf54l15_radio_debug.ack_tx_done);

  SHELL_OUTPUT(output,
               " drv_ack_rx_started=%" PRIu32
               " drv_ack_rx_valid=%" PRIu32
               " drv_ack_rx_invalid=%" PRIu32
               " drv_ack_timeout=%" PRIu32
               " drv_ack_delay_short=%" PRIu32,
               nrf54l15_radio_debug.ack_rx_started,
               nrf54l15_radio_debug.ack_rx_valid,
               nrf54l15_radio_debug.ack_rx_invalid,
               nrf54l15_radio_debug.ack_timeout,
               nrf54l15_radio_debug.ack_delay_too_short);

  SHELL_OUTPUT(output,
               " drv_ack_irq_cc=%" PRIu32
               " drv_ack_core_cc=%" PRIu32
               " drv_ack_txreq_cc=%" PRIu32,
               nrf54l15_radio_debug.last_ack_irq_entry_cc,
               nrf54l15_radio_debug.last_ack_core_entry_cc,
               nrf54l15_radio_debug.last_ack_before_txreq_cc);

  SHELL_OUTPUT(output,
               " drv_ack_last_delay_us=%" PRIu32
               " drv_ack_last_ramp_cc=%" PRIu32
               " drv_ack_last_now_cc=%" PRIu32
               " drv_ack_last_fem_cc=%" PRIu32
               " drv_ack_last_armed=%" PRIu32,
               nrf54l15_radio_debug.last_ack_delay_us,
               nrf54l15_radio_debug.last_ack_ramp_up_cc,
               nrf54l15_radio_debug.last_ack_now_cc,
               nrf54l15_radio_debug.last_ack_fem_cc,
               nrf54l15_radio_debug.last_ack_arm_result);

  output("\n");
}
/*---------------------------------------------------------------------------*/
static void
queue_tx_request(uint16_t count)
{
  if(count == 0) {
    return;
  }

  if((uint32_t)state.pending_tx + count > UINT16_MAX) {
    state.pending_tx = UINT16_MAX;
  } else {
    state.pending_tx += count;
  }

  process_poll(&radio_test_process);
}
/*---------------------------------------------------------------------------*/
static void
tx_done(void *ptr, int status, int transmissions)
{
  (void)ptr;

  state.tx_busy = false;
  state.last_tx_status = status;
  state.last_tx_attempts = transmissions;
  state.tx_event_pending = true;

  switch(status) {
  case MAC_TX_OK:
    state.tx_ok++;
    break;
  case MAC_TX_NOACK:
    state.tx_noack++;
    break;
  case MAC_TX_COLLISION:
    state.tx_collision++;
    break;
  default:
    state.tx_err++;
    break;
  }

  process_poll(&radio_test_process);
}
/*---------------------------------------------------------------------------*/
static void
input_callback(const void *data, uint16_t len, const linkaddr_t *src,
               const linkaddr_t *dest)
{
  const struct radio_test_hdr *hdr;

  (void)dest;

  if(len < sizeof(struct radio_test_hdr)) {
    state.rx_bad_len++;
    return;
  }

  hdr = (const struct radio_test_hdr *)data;
  if(hdr->magic != RADIO_TEST_MAGIC) {
    state.rx_bad_magic++;
    if(state.verbose) {
      LOG_INFO("RX bad magic 0x%08" PRIx32 " len=%u from ",
               hdr->magic, len);
      LOG_INFO_LLADDR(src);
      LOG_INFO_(" mac_seq=%u\n", packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO));
    }
    return;
  }

  state.rx_ok++;
  state.last_rx_seq = hdr->seq;
  state.last_rx_rssi = (int16_t)packetbuf_attr(PACKETBUF_ATTR_RSSI);
  state.last_rx_lqi = packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);
  linkaddr_copy(&state.last_rx_src, src);
  state.rx_event_pending = true;
  process_poll(&radio_test_process);
}
/*---------------------------------------------------------------------------*/
static void
update_timer(struct etimer *periodic_timer, bool *timer_active)
{
  if(state.running) {
    if(!*timer_active || state.timer_dirty) {
      clock_time_t ticks = (clock_time_t)(((uint64_t)state.interval_ms * CLOCK_SECOND) / 1000);

      if(ticks == 0) {
        ticks = 1;
      }

      etimer_set(periodic_timer, ticks);
      *timer_active = true;
    }
  } else if(*timer_active) {
    etimer_stop(periodic_timer);
    *timer_active = false;
  }

  state.timer_dirty = false;
}
/*---------------------------------------------------------------------------*/
static void
try_send_packet(void)
{
  struct radio_test_hdr *hdr;
  uint16_t i;

  if(state.tx_busy || state.pending_tx == 0) {
    return;
  }

  if(!state.target_set) {
    state.tx_no_target += state.pending_tx;
    state.pending_tx = 0;
    return;
  }

  hdr = (struct radio_test_hdr *)tx_payload;
  hdr->magic = RADIO_TEST_MAGIC;
  hdr->seq = state.next_seq++;

  for(i = sizeof(*hdr); i < state.payload_len; i++) {
    tx_payload[i] = (uint8_t)(hdr->seq + i);
  }

  packetbuf_clear();
  packetbuf_copyfrom(tx_payload, state.payload_len);
  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &state.target);
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_ACK, 1);
  packetbuf_set_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS,
                     state.max_transmissions);

  state.tx_started++;
  state.last_tx_seq = hdr->seq;
  state.pending_tx--;
  state.tx_busy = true;

  NETSTACK_MAC.send(tx_done, NULL);
}
/*---------------------------------------------------------------------------*/
static PT_THREAD(cmd_radio_test(struct pt *pt, shell_output_func output, char *args))
{
  char *next_args;
  int32_t svalue;
  uint32_t value;
  linkaddr_t addr;

  PT_BEGIN(pt);

  SHELL_ARGS_INIT(args, next_args);
  SHELL_ARGS_NEXT(args, next_args);

  if(args == NULL || !strcmp(args, "status")) {
    print_status(output);
    PT_EXIT(pt);
  }

  if(!strcmp(args, "status-brief")) {
    print_status_brief(output);
    PT_EXIT(pt);
  }

  if(!strcmp(args, "target")) {
    SHELL_ARGS_NEXT(args, next_args);
    if(args == NULL || !parse_lladdr(args, &addr)) {
      SHELL_OUTPUT(output, "Usage: radio-test target <16-hex-byte MAC>\n");
      PT_EXIT(pt);
    }
    linkaddr_copy(&state.target, &addr);
    state.target_set = true;
    SHELL_OUTPUT(output, "Target set to ");
    shell_output_lladdr(output, &state.target);
    SHELL_OUTPUT(output, "\n");
    PT_EXIT(pt);
  }

  if(!strcmp(args, "clear-target")) {
    memset(&state.target, 0, sizeof(state.target));
    state.target_set = false;
    SHELL_OUTPUT(output, "Target cleared\n");
    PT_EXIT(pt);
  }

  if(!strcmp(args, "start")) {
    state.running = true;
    state.run_limited = false;
    state.run_remaining = 0;
    state.timer_dirty = true;
    queue_tx_request(1);
    SHELL_OUTPUT(output, "Periodic transmit enabled\n");
    PT_EXIT(pt);
  }

  if(!strcmp(args, "run")) {
    SHELL_ARGS_NEXT(args, next_args);
    if(args == NULL || !parse_u32(args, &value) || value == 0) {
      SHELL_OUTPUT(output, "Usage: radio-test run <count>\n");
      PT_EXIT(pt);
    }

    state.running = true;
    state.run_limited = true;
    state.run_remaining = value - 1;
    state.timer_dirty = true;
    queue_tx_request(1);

    if(state.run_remaining == 0) {
      state.running = false;
      process_poll(&radio_test_process);
    }

    SHELL_OUTPUT(output, "Queued %" PRIu32 " timed transmits\n", value);
    PT_EXIT(pt);
  }

  if(!strcmp(args, "stop")) {
    state.running = false;
    state.run_limited = false;
    state.run_remaining = 0;
    state.timer_dirty = true;
    process_poll(&radio_test_process);
    SHELL_OUTPUT(output, "Periodic transmit disabled\n");
    PT_EXIT(pt);
  }

  if(!strcmp(args, "once")) {
    queue_tx_request(1);
    SHELL_OUTPUT(output, "Queued one transmit\n");
    PT_EXIT(pt);
  }

  if(!strcmp(args, "interval")) {
    SHELL_ARGS_NEXT(args, next_args);
    if(args == NULL || !parse_u32(args, &value) || value == 0) {
      SHELL_OUTPUT(output, "Usage: radio-test interval <ms>\n");
      PT_EXIT(pt);
    }
    state.interval_ms = value;
    state.timer_dirty = true;
    process_poll(&radio_test_process);
    SHELL_OUTPUT(output, "Interval set to %" PRIu32 " ms\n", state.interval_ms);
    PT_EXIT(pt);
  }

  if(!strcmp(args, "len")) {
    SHELL_ARGS_NEXT(args, next_args);
    if(args == NULL || !parse_u32(args, &value) ||
       value < sizeof(struct radio_test_hdr) ||
       value > RADIO_TEST_MAX_LEN) {
      SHELL_OUTPUT(output, "Usage: radio-test len <%u..%u>\n",
                   (unsigned)sizeof(struct radio_test_hdr), RADIO_TEST_MAX_LEN);
      PT_EXIT(pt);
    }
    state.payload_len = value;
    SHELL_OUTPUT(output, "Payload length set to %u bytes\n", state.payload_len);
    PT_EXIT(pt);
  }

  if(!strcmp(args, "txmax")) {
    SHELL_ARGS_NEXT(args, next_args);
    if(args == NULL || !parse_u32(args, &value) || value == 0 || value > UINT8_MAX) {
      SHELL_OUTPUT(output, "Usage: radio-test txmax <1..255>\n");
      PT_EXIT(pt);
    }
    state.max_transmissions = value;
    SHELL_OUTPUT(output, "Max MAC transmissions set to %u\n",
                 state.max_transmissions);
    PT_EXIT(pt);
  }

  if(!strcmp(args, "channel")) {
    radio_value_t current_channel = 0;
    radio_value_t min_channel = 0;
    radio_value_t max_channel = 0;
    radio_result_t result;

    result = NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &current_channel);
    if(result != RADIO_RESULT_OK) {
      SHELL_OUTPUT(output, "Channel control not supported\n");
      PT_EXIT(pt);
    }

    (void)NETSTACK_RADIO.get_value(RADIO_CONST_CHANNEL_MIN, &min_channel);
    (void)NETSTACK_RADIO.get_value(RADIO_CONST_CHANNEL_MAX, &max_channel);

    SHELL_ARGS_NEXT(args, next_args);
    if(args == NULL) {
      SHELL_OUTPUT(output, "Channel: %d (range %d..%d)\n",
                   current_channel, min_channel, max_channel);
      PT_EXIT(pt);
    }

    if(!parse_u32(args, &value)) {
      SHELL_OUTPUT(output, "Usage: radio-test channel [n]\n");
      PT_EXIT(pt);
    }

    result = NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, (radio_value_t)value);
    if(result != RADIO_RESULT_OK) {
      SHELL_OUTPUT(output, "Failed to set channel to %" PRIu32 " (err=%d)\n",
                   value, result);
      PT_EXIT(pt);
    }

    result = NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &current_channel);
    if(result == RADIO_RESULT_OK) {
      SHELL_OUTPUT(output, "Channel set to %d\n", current_channel);
    } else {
      SHELL_OUTPUT(output, "Channel set request sent\n");
    }
    PT_EXIT(pt);
  }

  if(!strcmp(args, "verbose")) {
    SHELL_ARGS_NEXT(args, next_args);
    if(args == NULL || !parse_u32(args, &value) || value > 1) {
      SHELL_OUTPUT(output, "Usage: radio-test verbose <0|1>\n");
      PT_EXIT(pt);
    }
    state.verbose = value;
    SHELL_OUTPUT(output, "Verbose set to %u\n", state.verbose);
    PT_EXIT(pt);
  }

  if(!strcmp(args, "power")) {
    radio_value_t current_power = 0;
    radio_value_t min_power = 0;
    radio_value_t max_power = 0;
    radio_result_t result;

    result = NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &current_power);
    if(result != RADIO_RESULT_OK) {
      SHELL_OUTPUT(output, "TX power control not supported\n");
      PT_EXIT(pt);
    }

    (void)NETSTACK_RADIO.get_value(RADIO_CONST_TXPOWER_MIN, &min_power);
    (void)NETSTACK_RADIO.get_value(RADIO_CONST_TXPOWER_MAX, &max_power);

    SHELL_ARGS_NEXT(args, next_args);
    if(args == NULL) {
      SHELL_OUTPUT(output, "TX power: %d dBm (range %d..%d)\n",
                   current_power, min_power, max_power);
      PT_EXIT(pt);
    }

    if(!parse_i32(args, &svalue)) {
      SHELL_OUTPUT(output, "Usage: radio-test power [dbm]\n");
      PT_EXIT(pt);
    }

    result = NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, (radio_value_t)svalue);
    if(result != RADIO_RESULT_OK) {
      SHELL_OUTPUT(output, "Failed to set TX power to %ld dBm (err=%d)\n",
                   (long)svalue, result);
      PT_EXIT(pt);
    }

    result = NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &current_power);
    if(result == RADIO_RESULT_OK) {
      SHELL_OUTPUT(output, "TX power set to %d dBm\n", current_power);
    } else {
      SHELL_OUTPUT(output, "TX power set request sent\n");
    }
    PT_EXIT(pt);
  }

  if(!strcmp(args, "reset")) {
    reset_stats();
    nrf54l15_radio_debug_reset();
    SHELL_OUTPUT(output, "Statistics reset\n");
    PT_EXIT(pt);
  }

  SHELL_OUTPUT(output,
               "Usage: radio-test [status|status-brief|target|clear-target|"
               "start|run|stop|once|interval|len|txmax|channel|power|"
               "verbose|reset]\n");

  PT_END(pt);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(radio_test_process, ev, data)
{
  static struct etimer periodic_timer;
  static bool timer_active;

  PROCESS_BEGIN();

  init_state();
  nrf54l15_radio_debug_reset();
  nullnet_set_input_callback(input_callback);
  shell_command_set_register(&radio_test_shell_set);

  LOG_INFO("MAC radio test ready\n");
  LOG_INFO("Use shell command: radio-test\n");

  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == PROCESS_EVENT_TIMER && data == &periodic_timer) {
      timer_active = false;
      if(state.running) {
        queue_tx_request(1);
        if(state.run_limited && state.run_remaining > 0) {
          state.run_remaining--;
        }
        if(state.run_limited && state.run_remaining == 0) {
          state.running = false;
        }
        state.timer_dirty = true;
      }
    }

    if(ev == PROCESS_EVENT_POLL || ev == PROCESS_EVENT_TIMER) {
      update_timer(&periodic_timer, &timer_active);

      if(state.tx_event_pending) {
        if(state.verbose) {
          LOG_INFO("TX seq=%" PRIu32 " status=%s attempts=%d\n",
                   state.last_tx_seq, status_to_str(state.last_tx_status),
                   state.last_tx_attempts);
        }
        state.tx_event_pending = false;
      }

      if(state.rx_event_pending) {
        if(state.verbose) {
          LOG_INFO("RX seq=%" PRIu32 " from ", state.last_rx_seq);
          LOG_INFO_LLADDR(&state.last_rx_src);
          LOG_INFO_(" RSSI=%d LQI=%u\n",
                    state.last_rx_rssi, state.last_rx_lqi);
        }
        state.rx_event_pending = false;
      }

      try_send_packet();
    }
  }

  shell_command_set_deregister(&radio_test_shell_set);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
