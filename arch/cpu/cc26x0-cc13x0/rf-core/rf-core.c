/*
 * Copyright (c) 2015, Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup rf-core
 * @{
 *
 * \file
 * Implementation of the CC13xx/CC26xx RF core driver
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/watchdog.h"
#include "sys/process.h"
#include "sys/energest.h"
#include "sys/cc.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "rf-core/rf-core.h"
#include "rf-core/rf-switch.h"
#include "ti-lib.h"
/*---------------------------------------------------------------------------*/
/* RF core and RF HAL API */
#include "hw_rfc_dbell.h"
#include "hw_rfc_pwr.h"
/*---------------------------------------------------------------------------*/
/* RF Core Mailbox API */
#include "driverlib/rf_mailbox.h"
#include "driverlib/rf_common_cmd.h"
#include "driverlib/rf_data_entry.h"
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
#ifdef RF_CORE_CONF_DEBUG_CRC
#define RF_CORE_DEBUG_CRC RF_CORE_CONF_DEBUG_CRC
#else
#define RF_CORE_DEBUG_CRC DEBUG
#endif
/*---------------------------------------------------------------------------*/
/* RF interrupts */
#define RX_FRAME_IRQ IRQ_RX_ENTRY_DONE
#define ERROR_IRQ    (IRQ_INTERNAL_ERROR | IRQ_RX_BUF_FULL)
#define RX_NOK_IRQ   IRQ_RX_NOK

/* Those IRQs are enabled all the time */
#if RF_CORE_DEBUG_CRC
#define ENABLED_IRQS (RX_FRAME_IRQ | ERROR_IRQ | RX_NOK_IRQ)
#else
#define ENABLED_IRQS (RX_FRAME_IRQ | ERROR_IRQ)
#endif

#define ENABLED_IRQS_POLL_MODE (ENABLED_IRQS & ~(RX_FRAME_IRQ | ERROR_IRQ))

#define cc26xx_rf_cpe0_isr RFCCPE0IntHandler
#define cc26xx_rf_cpe1_isr RFCCPE1IntHandler
/*---------------------------------------------------------------------------*/
typedef ChipType_t chip_type_t;
/*---------------------------------------------------------------------------*/
/* Remember the last Radio Op issued to the radio */
static rfc_radioOp_t *last_radio_op = NULL;
/*---------------------------------------------------------------------------*/
/* A struct holding pointers to the primary mode's abort() and restore() */
static const rf_core_primary_mode_t *primary_mode = NULL;
/*---------------------------------------------------------------------------*/
/* RAT has 32-bit register, overflows once 18 minutes */
#define RAT_RANGE  4294967296ull
/* approximate value */
#define RAT_OVERFLOW_PERIOD_SECONDS (60 * 18)

/* how often to check for the overflow, as a minimum */
#define RAT_OVERFLOW_TIMER_INTERVAL (CLOCK_SECOND * RAT_OVERFLOW_PERIOD_SECONDS / 3)

/* Radio timer (RAT) offset as compared to the rtimer counter (RTC) */
static int32_t rat_offset;
static bool rat_offset_known;

/* Value during the last read of the RAT register */
static uint32_t rat_last_value;

/* For RAT overflow handling */
static struct ctimer rat_overflow_timer;
static volatile uint32_t rat_overflow_counter;
static rtimer_clock_t rat_last_overflow;

static void rat_overflow_check_timer_cb(void *);
/*---------------------------------------------------------------------------*/
volatile int8_t rf_core_last_rssi = RF_CORE_CMD_CCA_REQ_RSSI_UNKNOWN;
volatile uint8_t rf_core_last_corr_lqi = 0;
volatile uint32_t rf_core_last_packet_timestamp = 0;
/*---------------------------------------------------------------------------*/
/* Are we currently in poll mode? */
uint8_t rf_core_poll_mode = 0;
/*---------------------------------------------------------------------------*/
/* Status of the last command sent */
volatile uint32_t last_cmd_status;
/*---------------------------------------------------------------------------*/
PROCESS(rf_core_process, "CC13xx / CC26xx RF driver");
/*---------------------------------------------------------------------------*/
#define RF_CORE_CLOCKS_MASK (RFC_PWR_PWMCLKEN_RFC_M | RFC_PWR_PWMCLKEN_CPE_M \
                             | RFC_PWR_PWMCLKEN_CPERAM_M | RFC_PWR_PWMCLKEN_FSCA_M \
                             | RFC_PWR_PWMCLKEN_PHA_M | RFC_PWR_PWMCLKEN_RAT_M \
                             | RFC_PWR_PWMCLKEN_RFERAM_M | RFC_PWR_PWMCLKEN_RFE_M \
                             | RFC_PWR_PWMCLKEN_MDMRAM_M | RFC_PWR_PWMCLKEN_MDM_M)
/*---------------------------------------------------------------------------*/
#define RF_CMD0	0x0607
/*---------------------------------------------------------------------------*/
uint8_t
rf_core_is_accessible()
{
  if(ti_lib_prcm_rf_ready()) {
    return RF_CORE_ACCESSIBLE;
  }
  return RF_CORE_NOT_ACCESSIBLE;
}
/*---------------------------------------------------------------------------*/
uint_fast8_t
rf_core_send_cmd(uint32_t cmd, uint32_t *status)
{
  uint32_t timeout_count = 0;
  bool interrupts_disabled;
  bool is_radio_op = false;

  /* reset the status variables to invalid values */
  last_cmd_status = (uint32_t)-1;
  *status = last_cmd_status;

  /*
   * If cmd is 4-byte aligned, then it's either a radio OP or an immediate
   * command. Clear the status field if it's a radio OP
   */
  if((cmd & 0x03) == 0) {
    uint32_t cmd_type;
    cmd_type = ((rfc_command_t *)cmd)->commandNo & RF_CORE_COMMAND_TYPE_MASK;
    if(cmd_type == RF_CORE_COMMAND_TYPE_IEEE_FG_RADIO_OP ||
       cmd_type == RF_CORE_COMMAND_TYPE_RADIO_OP) {
      is_radio_op = true;
      ((rfc_radioOp_t *)cmd)->status = RF_CORE_RADIO_OP_STATUS_IDLE;
    }
  }

  /*
   * Make sure ContikiMAC doesn't turn us off from within an interrupt while
   * we are accessing RF Core registers
   */
  interrupts_disabled = ti_lib_int_master_disable();

  if(!rf_core_is_accessible()) {
    PRINTF("rf_core_send_cmd: RF was off\n");
    if(!interrupts_disabled) {
      ti_lib_int_master_enable();
    }
    return RF_CORE_CMD_ERROR;
  }

  if(is_radio_op) {
    uint16_t command_no = ((rfc_radioOp_t *)cmd)->commandNo;
    if((command_no & RF_CORE_COMMAND_PROTOCOL_MASK) != RF_CORE_COMMAND_PROTOCOL_COMMON &&
       (command_no & RF_CORE_COMMAND_TYPE_MASK) == RF_CORE_COMMAND_TYPE_RADIO_OP) {
      last_radio_op = (rfc_radioOp_t *)cmd;
    }
  }

  HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDR) = cmd;
  do {
    last_cmd_status = HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDSTA);
    if(++timeout_count > 50000) {
      PRINTF("rf_core_send_cmd: 0x%08lx Timeout\n", cmd);
      if(!interrupts_disabled) {
        ti_lib_int_master_enable();
      }
      *status = last_cmd_status;
      return RF_CORE_CMD_ERROR;
    }
  } while((last_cmd_status & RF_CORE_CMDSTA_RESULT_MASK) == RF_CORE_CMDSTA_PENDING);

  if(!interrupts_disabled) {
    ti_lib_int_master_enable();
  }

  /*
   * If we reach here the command is no longer pending. It is either completed
   * successfully or with error
   */
  *status = last_cmd_status;
  return (last_cmd_status & RF_CORE_CMDSTA_RESULT_MASK) == RF_CORE_CMDSTA_DONE;
}
/*---------------------------------------------------------------------------*/
uint_fast8_t
rf_core_wait_cmd_done(void *cmd)
{
  volatile rfc_radioOp_t *command = (rfc_radioOp_t *)cmd;
  uint32_t timeout_cnt = 0;

  /*
   * 0xn4nn=DONE, 0x0400=DONE_OK while all other "DONE" values means done
   * but with some kind of error (ref. "Common radio operation status codes")
   */
  do {
    if(++timeout_cnt > 500000) {
      return RF_CORE_CMD_ERROR;
    }
  } while((command->status & RF_CORE_RADIO_OP_MASKED_STATUS)
          != RF_CORE_RADIO_OP_MASKED_STATUS_DONE);

  last_cmd_status = command->status;
  return (command->status & RF_CORE_RADIO_OP_MASKED_STATUS)
         == RF_CORE_RADIO_OP_STATUS_DONE_OK;
}
/*---------------------------------------------------------------------------*/
uint32_t
rf_core_cmd_status(void)
{
  return last_cmd_status;
}
/*---------------------------------------------------------------------------*/
static int
fs_powerdown(void)
{
  rfc_CMD_FS_POWERDOWN_t cmd;
  uint32_t cmd_status;

  rf_core_init_radio_op((rfc_radioOp_t *)&cmd, sizeof(cmd), CMD_FS_POWERDOWN);

  if(rf_core_send_cmd((uint32_t)&cmd, &cmd_status) != RF_CORE_CMD_OK) {
    PRINTF("fs_powerdown: CMDSTA=0x%08lx\n", cmd_status);
    return RF_CORE_CMD_ERROR;
  }

  if(rf_core_wait_cmd_done(&cmd) != RF_CORE_CMD_OK) {
    PRINTF("fs_powerdown: CMDSTA=0x%08lx, status=0x%04x\n",
           cmd_status, cmd.status);
    return RF_CORE_CMD_ERROR;
  }

  return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
int
rf_core_power_up()
{
  uint32_t cmd_status;
  bool interrupts_disabled = ti_lib_int_master_disable();

  ti_lib_int_pend_clear(INT_RFC_CPE_0);
  ti_lib_int_pend_clear(INT_RFC_CPE_1);
  ti_lib_int_disable(INT_RFC_CPE_0);
  ti_lib_int_disable(INT_RFC_CPE_1);

  /* Enable RF Core power domain */
  ti_lib_prcm_power_domain_on(PRCM_DOMAIN_RFCORE);
  while(ti_lib_prcm_power_domain_status(PRCM_DOMAIN_RFCORE)
        != PRCM_DOMAIN_POWER_ON);

  ti_lib_prcm_domain_enable(PRCM_DOMAIN_RFCORE);
  ti_lib_prcm_load_set();
  while(!ti_lib_prcm_load_get());

  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x0;
  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = 0x0;
  ti_lib_int_enable(INT_RFC_CPE_0);
  ti_lib_int_enable(INT_RFC_CPE_1);

  if(!interrupts_disabled) {
    ti_lib_int_master_enable();
  }

  rf_switch_power_up();

  /* Let CPE boot */
  HWREG(RFC_PWR_NONBUF_BASE + RFC_PWR_O_PWMCLKEN) = RF_CORE_CLOCKS_MASK;

  /* Turn on additional clocks on boot */
  HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG) = 0;
  HWREG(RFC_DBELL_BASE+RFC_DBELL_O_CMDR) =
    CMDR_DIR_CMD_2BYTE(RF_CMD0,
                       RFC_PWR_PWMCLKEN_MDMRAM | RFC_PWR_PWMCLKEN_RFERAM);

  /* Send ping (to verify RFCore is ready and alive) */
  if(rf_core_send_cmd(CMDR_DIR_CMD(CMD_PING), &cmd_status) != RF_CORE_CMD_OK) {
    PRINTF("rf_core_power_up: CMD_PING fail, CMDSTA=0x%08lx\n", cmd_status);
    return RF_CORE_CMD_ERROR;
  }

  return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
uint8_t
rf_core_start_rat(void)
{
  uint32_t cmd_status;
  rfc_CMD_SYNC_START_RAT_t cmd_start;

  /* Start radio timer (RAT) */
  rf_core_init_radio_op((rfc_radioOp_t *)&cmd_start, sizeof(cmd_start), CMD_SYNC_START_RAT);

  /* copy the value and send back */
  cmd_start.rat0 = rat_offset;

  if(rf_core_send_cmd((uint32_t)&cmd_start, &cmd_status) != RF_CORE_CMD_OK) {
    PRINTF("rf_core_get_rat_rtc_offset: SYNC_START_RAT fail, CMDSTA=0x%08lx\n",
           cmd_status);
    return RF_CORE_CMD_ERROR;
  }

  /* Wait until done (?) */
  if(rf_core_wait_cmd_done(&cmd_start) != RF_CORE_CMD_OK) {
    PRINTF("rf_core_cmd_ok: SYNC_START_RAT wait, CMDSTA=0x%08lx, status=0x%04x\n",
           cmd_status, cmd_start.status);
    return RF_CORE_CMD_ERROR;
  }

  return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
uint8_t
rf_core_stop_rat(void)
{
  rfc_CMD_SYNC_STOP_RAT_t cmd_stop;
  uint32_t cmd_status;

  rf_core_init_radio_op((rfc_radioOp_t *)&cmd_stop, sizeof(cmd_stop), CMD_SYNC_STOP_RAT);

  int ret = rf_core_send_cmd((uint32_t)&cmd_stop, &cmd_status);
  if(ret != RF_CORE_CMD_OK) {
    PRINTF("rf_core_get_rat_rtc_offset: SYNC_STOP_RAT fail, ret %d CMDSTA=0x%08lx\n",
           ret, cmd_status);
    return ret;
  }

  /* Wait until done */
  ret = rf_core_wait_cmd_done(&cmd_stop);
  if(ret != RF_CORE_CMD_OK) {
    PRINTF("rf_core_cmd_ok: SYNC_STOP_RAT wait, CMDSTA=0x%08lx, status=0x%04x\n",
        cmd_status, cmd_stop.status);
    return ret;
  }

  if(!rat_offset_known) {
    /* save the offset, but only if this is the first time */
    rat_offset_known = true;
    rat_offset = cmd_stop.rat0;
  }

  return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
void
rf_core_power_down()
{
  bool interrupts_disabled = ti_lib_int_master_disable();
  ti_lib_int_disable(INT_RFC_CPE_0);
  ti_lib_int_disable(INT_RFC_CPE_1);

  if(rf_core_is_accessible()) {
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x0;
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = 0x0;

    /* need to send FS_POWERDOWN or analog components will use power */
    fs_powerdown();
  }

  rf_core_stop_rat();

  /* Shut down the RFCORE clock domain in the MCU VD */
  ti_lib_prcm_domain_disable(PRCM_DOMAIN_RFCORE);
  ti_lib_prcm_load_set();
  while(!ti_lib_prcm_load_get());

  /* Turn off RFCORE PD */
  ti_lib_prcm_power_domain_off(PRCM_DOMAIN_RFCORE);
  while(ti_lib_prcm_power_domain_status(PRCM_DOMAIN_RFCORE)
        != PRCM_DOMAIN_POWER_OFF);

  rf_switch_power_down();

  ti_lib_int_pend_clear(INT_RFC_CPE_0);
  ti_lib_int_pend_clear(INT_RFC_CPE_1);
  ti_lib_int_enable(INT_RFC_CPE_0);
  ti_lib_int_enable(INT_RFC_CPE_1);
  if(!interrupts_disabled) {
    ti_lib_int_master_enable();
  }
}
/*---------------------------------------------------------------------------*/
uint8_t
rf_core_set_modesel()
{
  uint8_t rv = RF_CORE_CMD_ERROR;
  chip_type_t chip_type = ti_lib_chipinfo_get_chip_type();

  if(chip_type == CHIP_TYPE_CC2650) {
    HWREG(PRCM_BASE + PRCM_O_RFCMODESEL) = PRCM_RFCMODESEL_CURR_MODE5;
    rv = RF_CORE_CMD_OK;
  } else if(chip_type == CHIP_TYPE_CC2630) {
    HWREG(PRCM_BASE + PRCM_O_RFCMODESEL) = PRCM_RFCMODESEL_CURR_MODE2;
    rv = RF_CORE_CMD_OK;
  } else if(chip_type == CHIP_TYPE_CC1310) {
    HWREG(PRCM_BASE + PRCM_O_RFCMODESEL) = PRCM_RFCMODESEL_CURR_MODE3;
    rv = RF_CORE_CMD_OK;
  } else if(chip_type == CHIP_TYPE_CC1350) {
    HWREG(PRCM_BASE + PRCM_O_RFCMODESEL) = PRCM_RFCMODESEL_CURR_MODE5;
    rv = RF_CORE_CMD_OK;
#if CPU_FAMILY_CC26X0R2
  } else if(chip_type == CHIP_TYPE_CC2640R2) {
    HWREG(PRCM_BASE + PRCM_O_RFCMODESEL) = PRCM_RFCMODESEL_CURR_MODE1;
    rv = RF_CORE_CMD_OK;
#endif
  }

  return rv;
}
/*---------------------------------------------------------------------------*/
uint8_t
rf_core_boot()
{
  if(rf_core_power_up() != RF_CORE_CMD_OK) {
    PRINTF("rf_core_boot: rf_core_power_up() failed\n");

    rf_core_power_down();

    return RF_CORE_CMD_ERROR;
  }

  if(rf_core_start_rat() != RF_CORE_CMD_OK) {
    PRINTF("rf_core_boot: rf_core_start_rat() failed\n");

    rf_core_power_down();

    return RF_CORE_CMD_ERROR;
  }

  return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
uint8_t
rf_core_restart_rat(void)
{
  if(rf_core_stop_rat() != RF_CORE_CMD_OK) {
    PRINTF("rf_core_restart_rat: rf_core_stop_rat() failed\n");
    /* Don't bail out here, still try to start it */
  }

  if(rf_core_start_rat() != RF_CORE_CMD_OK) {
    PRINTF("rf_core_restart_rat: rf_core_start_rat() failed\n");

    rf_core_power_down();

    return RF_CORE_CMD_ERROR;
  }

  return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
void
rf_core_setup_interrupts(void)
{
  bool interrupts_disabled;
  const uint32_t enabled_irqs = rf_core_poll_mode ? ENABLED_IRQS_POLL_MODE : ENABLED_IRQS;

  /* We are already turned on by the caller, so this should not happen */
  if(!rf_core_is_accessible()) {
    PRINTF("setup_interrupts: No access\n");
    return;
  }

  /* Disable interrupts */
  interrupts_disabled = ti_lib_int_master_disable();

  /* Set all interrupt channels to CPE0 channel, error to CPE1 */
  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEISL) = ERROR_IRQ;

  /* Acknowledge configured interrupts */
  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = enabled_irqs;

  /* Clear interrupt flags, active low clear(?) */
  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x0;

  ti_lib_int_pend_clear(INT_RFC_CPE_0);
  ti_lib_int_pend_clear(INT_RFC_CPE_1);
  ti_lib_int_enable(INT_RFC_CPE_0);
  ti_lib_int_enable(INT_RFC_CPE_1);

  if(!interrupts_disabled) {
    ti_lib_int_master_enable();
  }
}
/*---------------------------------------------------------------------------*/
void
rf_core_cmd_done_en(bool fg)
{
  uint32_t irq = 0;
  const uint32_t enabled_irqs = rf_core_poll_mode ? ENABLED_IRQS_POLL_MODE : ENABLED_IRQS;

  if(!rf_core_poll_mode) {
    irq = fg ? IRQ_LAST_FG_COMMAND_DONE : IRQ_LAST_COMMAND_DONE;
  }

  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = enabled_irqs;
  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = enabled_irqs | irq;
}
/*---------------------------------------------------------------------------*/
void
rf_core_cmd_done_dis(void)
{
  const uint32_t enabled_irqs = rf_core_poll_mode ? ENABLED_IRQS_POLL_MODE : ENABLED_IRQS;
  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = enabled_irqs;
}
/*---------------------------------------------------------------------------*/
rfc_radioOp_t *
rf_core_get_last_radio_op()
{
  return last_radio_op;
}
/*---------------------------------------------------------------------------*/
void
rf_core_init_radio_op(rfc_radioOp_t *op, uint16_t len, uint16_t command)
{
  memset(op, 0, len);

  op->commandNo = command;
  op->condition.rule = COND_NEVER;
}
/*---------------------------------------------------------------------------*/
void
rf_core_primary_mode_register(const rf_core_primary_mode_t *mode)
{
  primary_mode = mode;
}
/*---------------------------------------------------------------------------*/
void
rf_core_primary_mode_abort()
{
  if(primary_mode) {
    if(primary_mode->abort) {
      primary_mode->abort();
    }
  }
}
/*---------------------------------------------------------------------------*/
uint8_t
rf_core_primary_mode_restore()
{
  if(primary_mode) {
    if(primary_mode->restore) {
      return primary_mode->restore();
    }
  }

  return RF_CORE_CMD_ERROR;
}
/*---------------------------------------------------------------------------*/
uint8_t
rf_core_rat_init(void)
{
  rat_last_value = HWREG(RFC_RAT_BASE + RATCNT);

  ctimer_set(&rat_overflow_timer, RAT_OVERFLOW_TIMER_INTERVAL,
             rat_overflow_check_timer_cb, NULL);

  return 1;
}
/*---------------------------------------------------------------------------*/
uint8_t
rf_core_check_rat_overflow(void)
{
  uint32_t rat_current_value;
  uint8_t interrupts_disabled;

  /* Bail out if the RF is not on */
  if(primary_mode == NULL || !primary_mode->is_on()) {
    return 0;
  }

  interrupts_disabled = ti_lib_int_master_disable();

  rat_current_value = HWREG(RFC_RAT_BASE + RATCNT);
  if(rat_current_value + RAT_RANGE / 4 < rat_last_value) {
    /* Overflow detected */
    rat_last_overflow = RTIMER_NOW();
    rat_overflow_counter++;
  }
  rat_last_value = rat_current_value;

  if(!interrupts_disabled) {
    ti_lib_int_master_enable();
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
rat_overflow_check_timer_cb(void *unused)
{
  uint8_t success = 0;
  uint8_t was_off = 0;

  if(primary_mode != NULL) {

    if(!primary_mode->is_on()) {
      was_off = 1;
      if(NETSTACK_RADIO.on() != RF_CORE_CMD_OK) {
        PRINTF("overflow: on() failed\n");
        ctimer_set(&rat_overflow_timer, CLOCK_SECOND,
                   rat_overflow_check_timer_cb, NULL);
        return;
      }
    }

    success = rf_core_check_rat_overflow();

    if(was_off) {
      NETSTACK_RADIO.off();
    }
  }

  if(success) {
    /* Retry after half of the interval */
    ctimer_set(&rat_overflow_timer, RAT_OVERFLOW_TIMER_INTERVAL,
               rat_overflow_check_timer_cb, NULL);
  } else {
    /* Retry sooner */
    ctimer_set(&rat_overflow_timer, CLOCK_SECOND,
               rat_overflow_check_timer_cb, NULL);
  }
}
/*---------------------------------------------------------------------------*/
uint32_t
rf_core_convert_rat_to_rtimer(uint32_t rat_timestamp)
{
  uint64_t rat_timestamp64;
  uint32_t adjusted_overflow_counter;
  uint8_t was_off = 0;

  if(primary_mode == NULL) {
    PRINTF("rf_core_convert_rat_to_rtimer: not initialized\n");
    return 0;
  }

  if(!primary_mode->is_on()) {
    was_off = 1;
    NETSTACK_RADIO.on();
  }

  rf_core_check_rat_overflow();

  if(was_off) {
    NETSTACK_RADIO.off();
  }

  adjusted_overflow_counter = rat_overflow_counter;

  /* if the timestamp is large and the last oveflow was recently,
     assume that the timestamp refers to the time before the overflow */
  if(rat_timestamp > (uint32_t)(RAT_RANGE * 3 / 4)) {
    if(RTIMER_CLOCK_LT(RTIMER_NOW(),
                       rat_last_overflow + RAT_OVERFLOW_PERIOD_SECONDS * RTIMER_SECOND / 4)) {
      adjusted_overflow_counter--;
    }
  }

  /* add the overflowed time to the timestamp */
  rat_timestamp64 = rat_timestamp + RAT_RANGE * adjusted_overflow_counter;
  /* correct timestamp so that it refers to the end of the SFD */
  rat_timestamp64 += primary_mode->sfd_timestamp_offset;

  return RADIO_TO_RTIMER(rat_timestamp64 - rat_offset);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rf_core_process, ev, data)
{
  int len;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    do {
      watchdog_periodic();
      packetbuf_clear();
      len = NETSTACK_RADIO.read(packetbuf_dataptr(), PACKETBUF_SIZE);

      if(len > 0) {
        packetbuf_set_datalen(len);

        NETSTACK_MAC.input();
      }
    } while(len > 0);
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void
rx_nok_isr(void)
{
}
/*---------------------------------------------------------------------------*/
void
cc26xx_rf_cpe1_isr(void)
{
  PRINTF("RF Error\n");

  if(!rf_core_is_accessible()) {
    if(rf_core_power_up() != RF_CORE_CMD_OK) {
      return;
    }
  }

  if(HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_RX_BUF_FULL) {
    PRINTF("\nRF: BUF_FULL\n\n");
    /* make sure read_frame() will be called to make space in RX buffer */
    process_poll(&rf_core_process);
    /* Clear the IRQ_RX_BUF_FULL interrupt flag by writing zero to bit */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = ~(IRQ_RX_BUF_FULL);
  }

  /* Clear INTERNAL_ERROR interrupt flag */
  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x7FFFFFFF;
}
/*---------------------------------------------------------------------------*/
void
cc26xx_rf_cpe0_isr(void)
{
  if(!rf_core_is_accessible()) {
    PRINTF("RF ISR called but RF not ready... PANIC!!\n");
    if(rf_core_power_up() != RF_CORE_CMD_OK) {
      PRINTF("rf_core_power_up() failed\n");
      return;
    }
  }

  ti_lib_int_master_disable();

  if(HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & RX_FRAME_IRQ) {
    /* Clear the RX_ENTRY_DONE interrupt flag */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0xFF7FFFFF;
    process_poll(&rf_core_process);
  }

  if(RF_CORE_DEBUG_CRC) {
    if(HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & RX_NOK_IRQ) {
      /* Clear the RX_NOK interrupt flag */
      HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0xFFFDFFFF;
      rx_nok_isr();
    }
  }

  if(HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) &
     (IRQ_LAST_FG_COMMAND_DONE | IRQ_LAST_COMMAND_DONE)) {
    /* Clear the two TX-related interrupt flags */
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0xFFFFFFF5;
  }

  ti_lib_int_master_enable();
}
/*---------------------------------------------------------------------------*/
/** @} */
