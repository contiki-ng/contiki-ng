/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * IRQ Abstraction Layer for nrf_802154 on nRF54L15.
 * Stores ISR function pointers for RADIO and EGU10 and dispatches from
 * the NVIC IRQ handlers.
 */

#include "platform/nrf_802154_irq.h"
#include "nrf_802154_config.h"
#include "nrf_802154_irq_handlers.h"
#include "nrf.h"

#include <nrfx.h>
#include <stddef.h>

/* Only two ISRs are used by the 802.15.4 driver: RADIO and EGU10 (SWI). */
static nrf_802154_isr_t radio_isr;
static nrf_802154_isr_t egu10_isr;
/*---------------------------------------------------------------------------*/
void
nrf_802154_irq_init(uint32_t irqn, int32_t prio, nrf_802154_isr_t isr)
{
  if(prio < 0) {
    /* Negative priorities are reserved by the driver for zero-latency IRQs.
     * Bare-metal Contiki only exposes the NVIC priority value, so clamp to
     * the highest programmable priority. */
    prio = 0;
  }

  if(irqn == RADIO_0_IRQn || irqn == RADIO_1_IRQn
#ifdef RADIO_IRQn
     || irqn == RADIO_IRQn
#endif
     ) {
    radio_isr = isr;
  } else if(irqn == EGU10_IRQn) {
    egu10_isr = isr;
  }
  NVIC_SetPriority((IRQn_Type)irqn, (uint32_t)prio);
  NVIC_ClearPendingIRQ((IRQn_Type)irqn);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_irq_enable(uint32_t irqn)
{
  NVIC_EnableIRQ((IRQn_Type)irqn);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_irq_disable(uint32_t irqn)
{
  NVIC_DisableIRQ((IRQn_Type)irqn);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_irq_set_pending(uint32_t irqn)
{
  NVIC_SetPendingIRQ((IRQn_Type)irqn);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_irq_clear_pending(uint32_t irqn)
{
  NVIC_ClearPendingIRQ((IRQn_Type)irqn);
}
/*---------------------------------------------------------------------------*/
bool
nrf_802154_irq_is_enabled(uint32_t irqn)
{
  return NVIC_GetEnableIRQ((IRQn_Type)irqn) != 0;
}
/*---------------------------------------------------------------------------*/
uint32_t
nrf_802154_irq_priority_get(uint32_t irqn)
{
  return NVIC_GetPriority((IRQn_Type)irqn);
}
/*
 * RADIO IRQ handler -- dispatch to the ISR registered by the 802.15.4 driver.
 */
volatile uint32_t radio_irq_count;
/*---------------------------------------------------------------------------*/
void
RADIO_0_IRQHandler(void)
{
  radio_irq_count++;
#if NRF_802154_INTERNAL_RADIO_IRQ_HANDLING
  if(radio_isr != NULL) {
    radio_isr();
  }
#else
  nrf_802154_radio_irq_handler();
#endif
}
/*---------------------------------------------------------------------------*/
void
RADIO_1_IRQHandler(void)
{
  radio_irq_count++;
#if NRF_802154_INTERNAL_RADIO_IRQ_HANDLING
  if(radio_isr != NULL) {
    radio_isr();
  }
#else
  nrf_802154_radio_irq_handler();
#endif
}
/*
 * EGU10 IRQ handler -- used by the SWI notification/request layer.
 */
volatile uint32_t egu10_irq_count;
/*---------------------------------------------------------------------------*/
void
EGU10_IRQHandler(void)
{
  egu10_irq_count++;
#if NRF_802154_INTERNAL_SWI_IRQ_HANDLING
  if(egu10_isr != NULL) {
    egu10_isr();
  }
#else
  nrf_802154_swi_irq_handler();
#endif
}
/*---------------------------------------------------------------------------*/
