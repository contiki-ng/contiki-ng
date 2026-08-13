/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "sys/int-master.h"
#include <stdint.h>

#define MSTATUS_MIE_BIT (1U << 3)

void
int_master_enable(void)
{
  __asm__ volatile ("csrs mstatus, %0" :: "r"(MSTATUS_MIE_BIT));
}

int_master_status_t
int_master_read_and_disable(void)
{
  uint32_t prev;
  __asm__ volatile ("csrrc %0, mstatus, %1" : "=r"(prev) : "r"(MSTATUS_MIE_BIT));
  return prev & MSTATUS_MIE_BIT;
}

void
int_master_status_set(int_master_status_t s)
{
  if(s & MSTATUS_MIE_BIT) {
    int_master_enable();
  }
}

bool
int_master_is_enabled(void)
{
  uint32_t s;
  __asm__ volatile ("csrr %0, mstatus" : "=r"(s));
  return (s & MSTATUS_MIE_BIT) != 0;
}
