/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * \file
 *      RF switch initialization for the Seeed XIAO nRF54L15.
 */

#include "contiki.h"
#include BOARD_DEF_PATH

#include "nrf_gpio.h"
#include <string.h>
#if BUILD_WITH_SHELL
#include "shell.h"
#include "shell-commands.h"
#include "sys/etimer.h"
#endif

static bool rf_sw_external;
static bool rf_sw_power_on = true;

static void rf_switch_apply(void);

#if BUILD_WITH_SHELL
static void rf_switch_print_status(shell_output_func output);
static PT_THREAD(cmd_rf_switch(struct pt *pt, shell_output_func output, char *args));
PROCESS(xiao_nrf54l15_rf_switch_shell_process, "XIAO RF switch shell");

static const struct shell_command_t rf_switch_commands[] = {
  {
    "rf-sw",
    cmd_rf_switch,
    "'> rf-sw': status | ceramic | external | power [0|1|off|on]"
  },
  { NULL, NULL, NULL }
};

static struct shell_command_set_t rf_switch_shell_set = {
  .next = NULL,
  .commands = rf_switch_commands,
};
#endif
/*---------------------------------------------------------------------------*/
static void
rf_switch_apply(void)
{
  uint32_t rf_sw_sel =
    NRF_GPIO_PIN_MAP(XIAO_NRF54L15_RF_SW_SEL_PORT, XIAO_NRF54L15_RF_SW_SEL_PIN);
  uint32_t rf_sw_pwr =
    NRF_GPIO_PIN_MAP(XIAO_NRF54L15_RF_SW_PWR_PORT, XIAO_NRF54L15_RF_SW_PWR_PIN);

  nrf_gpio_cfg_output(rf_sw_sel);
  nrf_gpio_cfg_output(rf_sw_pwr);

  if(rf_sw_external) {
    nrf_gpio_pin_set(rf_sw_sel);
  } else {
    nrf_gpio_pin_clear(rf_sw_sel);
  }

  if(rf_sw_power_on) {
    nrf_gpio_pin_set(rf_sw_pwr);
  } else {
    nrf_gpio_pin_clear(rf_sw_pwr);
  }
}
/*---------------------------------------------------------------------------*/
void
platform_init_board(void)
{
  /* The XIAO routes the 2.4 GHz path through an external RF switch.
   * Power the switch and default to the onboard ceramic antenna (RF1). */
  rf_sw_external = false;
  rf_sw_power_on = true;
  rf_switch_apply();
}
/*---------------------------------------------------------------------------*/
void
platform_init_board_stage_two(void)
{
#if BUILD_WITH_SHELL
  process_start(&xiao_nrf54l15_rf_switch_shell_process, NULL);
#endif
}
#if BUILD_WITH_SHELL
/*---------------------------------------------------------------------------*/
static void
rf_switch_print_status(shell_output_func output)
{
  const char *path = rf_sw_external ? "external" : "ceramic";
  const char *power = rf_sw_power_on ? "on" : "off";
  uint32_t rf_sw_sel =
    NRF_GPIO_PIN_MAP(XIAO_NRF54L15_RF_SW_SEL_PORT, XIAO_NRF54L15_RF_SW_SEL_PIN);
  uint32_t rf_sw_pwr =
    NRF_GPIO_PIN_MAP(XIAO_NRF54L15_RF_SW_PWR_PORT, XIAO_NRF54L15_RF_SW_PWR_PIN);

  SHELL_OUTPUT(output,
               "RF switch: path=%s power=%s raw(sel=%u pwr=%u)\n",
               path, power,
               (unsigned)nrf_gpio_pin_read(rf_sw_sel),
               (unsigned)nrf_gpio_pin_read(rf_sw_pwr));
}
/*---------------------------------------------------------------------------*/
static PT_THREAD(cmd_rf_switch(struct pt *pt, shell_output_func output, char *args))
{
  char *next_args;

  PT_BEGIN(pt);

  SHELL_ARGS_INIT(args, next_args);
  SHELL_ARGS_NEXT(args, next_args);

  if(args == NULL || !strcmp(args, "status")) {
    rf_switch_print_status(output);
    PT_EXIT(pt);
  }

  if(!strcmp(args, "ceramic")) {
    rf_sw_external = false;
    rf_sw_power_on = true;
    rf_switch_apply();
    rf_switch_print_status(output);
    PT_EXIT(pt);
  }

  if(!strcmp(args, "external")) {
    rf_sw_external = true;
    rf_sw_power_on = true;
    rf_switch_apply();
    rf_switch_print_status(output);
    PT_EXIT(pt);
  }

  if(!strcmp(args, "power")) {
    SHELL_ARGS_NEXT(args, next_args);
    if(args == NULL) {
      rf_switch_print_status(output);
      PT_EXIT(pt);
    }

    if(!strcmp(args, "1") || !strcmp(args, "on")) {
      rf_sw_power_on = true;
      rf_switch_apply();
      rf_switch_print_status(output);
      PT_EXIT(pt);
    }

    if(!strcmp(args, "0") || !strcmp(args, "off")) {
      rf_sw_power_on = false;
      rf_switch_apply();
      rf_switch_print_status(output);
      PT_EXIT(pt);
    }

    SHELL_OUTPUT(output, "Usage: rf-sw power [0|1|off|on]\n");
    PT_EXIT(pt);
  }

  SHELL_OUTPUT(output, "Usage: rf-sw status|ceramic|external|power [0|1|off|on]\n");
  PT_END(pt);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(xiao_nrf54l15_rf_switch_shell_process, ev, data)
{
  static struct etimer register_timer;

  PROCESS_BEGIN();

  /* Register after serial_shell_init() has had a chance to call shell_init(). */
  etimer_set(&register_timer, 1);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&register_timer));
  shell_command_set_register(&rf_switch_shell_set);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
#endif
