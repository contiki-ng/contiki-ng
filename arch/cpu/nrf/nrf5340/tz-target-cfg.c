/*
 * Copyright (c) 2018-2020 Arm Limited. All rights reserved.
 * Copyright (c) 2020 Nordic Semiconductor ASA.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* This file has been modified for use in the Contiki-NG operating system. */

#include "contiki.h"

#include "tz-target-cfg.h"
#include "region_defs.h"
#include "trustzone/tz-api.h"

#include <spu.h>
#include <nrfx.h>
#include <hal/nrf_gpio.h>

#include "nrf5340_application_bitfields.h"

/******************************************************************************/
#if NRF_GPIO_HAS_SEL
#define gpio_pin_select nrf_gpio_pin_control_select
#define GPIO_PIN_SEL_PERIPHERAL NRF_GPIO_PIN_SEL_PERIPHERAL
#else
#define gpio_pin_select nrf_gpio_pin_mcu_select
#define GPIO_PIN_SEL_PERIPHERAL NRF_GPIO_PIN_MCUSEL_PERIPHERAL
#endif
/******************************************************************************/
#include "sys/log.h"
#define LOG_MODULE "TZSecureWorld"
#define LOG_LEVEL LOG_LEVEL_DBG
/******************************************************************************/

#define PIN_XL1 0
#define PIN_XL2 1

/* To write into AIRCR register, 0x5FA value must be write to the VECTKEY field,
 * otherwise the processor ignores the write.
 */
#define SCB_AIRCR_WRITE_MASK ((0x5FAUL << SCB_AIRCR_VECTKEY_Pos))
/******************************************************************************/
void
enable_fault_handlers(void)
{
  /* Explicitly set secure fault priority to the highest */
  NVIC_SetPriority(SecureFault_IRQn, 0);

  /* Enables BUS, MEM, USG and Secure faults */
  SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk | SCB_SHCSR_SECUREFAULTENA_Msk;
}
/******************************************************************************/
void
system_reset_cfg(void)
{
  uint32_t reg_value = SCB->AIRCR;

  /* Clear SCB_AIRCR_VECTKEY value */
  reg_value &= ~(uint32_t)(SCB_AIRCR_VECTKEY_Msk);

  /* Enable system reset request only to the secure world */
  reg_value |= (uint32_t)(SCB_AIRCR_WRITE_MASK | SCB_AIRCR_SYSRESETREQS_Msk);

  SCB->AIRCR = reg_value;
}
/******************************************************************************/
/*----------------- NVIC interrupt target state to NS configuration ----------*/
void
nvic_interrupt_target_state_cfg(void)
{
  /*
   * Target all interrupts to NS by default; unimplemented interrupts
   * will be Write-Ignored. Peripherals that must remain secure are
   * cleared explicitly below.
   */
  for(uint8_t i = 0; i < sizeof(NVIC->ITNS) / sizeof(NVIC->ITNS[0]); i++) {
    NVIC->ITNS[i] = 0xffffffff;
  }

  /* Keep secure-world peripherals targeting secure state. */
  NVIC_ClearTargetState(NRFX_IRQ_NUMBER_GET(NRF_SPU));

  /* IPC IRQ must target secure state since IPC is a secure peripheral. */
  NVIC_ClearTargetState(NRFX_IRQ_NUMBER_GET(NRF_IPC));

  /* RTC1 and TIMER1 are used by the secure world's clock/rtimer. */
  NVIC_ClearTargetState(NRFX_IRQ_NUMBER_GET(NRF_RTC1));
  NVIC_ClearTargetState(NRFX_IRQ_NUMBER_GET(NRF_TIMER1));

#ifdef SECURE_UART0
  /* UARTE0 is a secure peripheral, so its IRQ has to target S state */
  NVIC_ClearTargetState(NRFX_IRQ_NUMBER_GET(NRF_UARTE0));
#endif

#ifdef SECURE_UART1
  /* UARTE1 is a secure peripheral, so its IRQ has to target S state */
  NVIC_ClearTargetState(NRFX_IRQ_NUMBER_GET(NRF_UARTE1));
#endif

  /* TIMER1 is kept secure for the secure-world rtimer. */
  NVIC_ClearTargetState(NRFX_IRQ_NUMBER_GET(NRF_TIMER1));

  /* RTC1 is kept secure for the secure-world clock. */
  NVIC_ClearTargetState(NRFX_IRQ_NUMBER_GET(NRF_RTC1));
}
/******************************************************************************/
/*----------------- NVIC interrupt enabling for S peripherals ----------------*/
void
nvic_interrupt_enable(void)
{
  /* SPU interrupt enabling */
  spu_enable_interrupts();

  NVIC_ClearPendingIRQ(NRFX_IRQ_NUMBER_GET(NRF_SPU));
  NVIC_EnableIRQ(NRFX_IRQ_NUMBER_GET(NRF_SPU));
}
/******************************************************************************/
/*----------------- TrustZone API platform hooks -----------------------------*/
/*
 * Borrow EGU0_IRQn as a software-pended doorbell to wake the normal
 * world from a secure ISR. The EGU peripheral itself is left unused;
 * we only need an NS-targeted NVIC slot. EGU0 is configured non-secure
 * and the ITNS bit is set by nvic_interrupt_target_state_cfg above.
 */
void
tz_arch_signal_ns(void)
{
  TZ_NVIC_SetPendingIRQ_NS(EGU0_IRQn);
}
/******************************************************************************/
/*----------------- SPU violation diagnostics --------------------------------*/
#define SPU_VIOLATION_MAGIC 0x5BADACCEUL
struct spu_violation_info {
  uint32_t magic;
  uint32_t flashaccerr;
  uint32_t ramaccerr;
  uint32_t periphaccerr;
};
__attribute__((section(".noinit"))) static volatile struct spu_violation_info
  spu_violation_info;

void
spu_report_violation(void)
{
  if(spu_violation_info.magic != SPU_VIOLATION_MAGIC) {
    return;
  }
  spu_violation_info.magic = 0;

  LOG_WARN("Reboot caused by SPU violation:%s%s%s\n",
           spu_violation_info.flashaccerr ? " FLASHACCERR" : "",
           spu_violation_info.ramaccerr ? " RAMACCERR" : "",
           spu_violation_info.periphaccerr ? " PERIPHACCERR" : "");
}
/******************************************************************************/
/*----------------- SPU interrupt handler ------------------------------------*/
void
SPU_IRQHandler(void)
{
  /*
   * Stash the violation type in .noinit for spu_report_violation() to
   * print on the next boot. No log call here: the UARTE TX path would
   * block waiting for an ENDTX ISR that cannot run while this handler
   * is active.
   */
  spu_violation_info.flashaccerr = NRF_SPU->EVENTS_FLASHACCERR;
  spu_violation_info.ramaccerr = NRF_SPU->EVENTS_RAMACCERR;
  spu_violation_info.periphaccerr = NRF_SPU->EVENTS_PERIPHACCERR;
  spu_violation_info.magic = SPU_VIOLATION_MAGIC;

  spu_clear_events();
  NVIC_SystemReset();
}
/******************************************************************************/
/*------------------- SAU/IDAU configuration functions -----------------------*/
void
sau_and_idau_cfg(void)
{
  /* IDAU (SPU) is always enabled. SAU is non-existent.
   * Allow SPU to have precedence over (non-existing) ARMv8-M SAU.
   */
  TZ_SAU_Disable();
  SAU->CTRL |= SAU_CTRL_ALLNS_Msk;
}
/******************************************************************************/
void
spu_periph_init_cfg(void)
{
  /* Peripheral configuration */

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_FPU));
  spu_peripheral_config_non_secure((uint32_t)NRF_FPU, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_REGULATORS));
  spu_peripheral_config_non_secure((uint32_t)NRF_REGULATORS, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_CLOCK));
  spu_peripheral_config_non_secure((uint32_t)NRF_CLOCK, true); /* Necessary */

#ifndef SECURE_UART0
  /* If UART0 is a secure peripheral, we need to leave Serial-Box 0 as Secure */
  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_SPIM0));
  spu_peripheral_config_non_secure((uint32_t)NRF_SPIM0, false);
#endif

#ifndef SECURE_UART1
  /* If UART1 is a secure peripheral, we need to leave Serial-Box 1 as Secure */
  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_SPIM1));
  spu_peripheral_config_non_secure((uint32_t)NRF_SPIM1, false);
#endif

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_SPIM4));
  spu_peripheral_config_non_secure((uint32_t)NRF_SPIM4, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_SPIM2));
  spu_peripheral_config_non_secure((uint32_t)NRF_SPIM2, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_SPIM3));
  spu_peripheral_config_non_secure((uint32_t)NRF_SPIM3, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_SAADC));
  spu_peripheral_config_non_secure((uint32_t)NRF_SAADC, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_TIMER0));
  spu_peripheral_config_non_secure((uint32_t)NRF_TIMER0, false);

  /* TIMER1 is kept secure: used by the secure world for rtimer. */

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_TIMER2));
  spu_peripheral_config_non_secure((uint32_t)NRF_TIMER2, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_RTC0));
  spu_peripheral_config_non_secure((uint32_t)NRF_RTC0, false);

  /* RTC1 is kept secure: used by the secure world for the clock. */

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_DPPIC));
  spu_peripheral_config_non_secure((uint32_t)NRF_DPPIC, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_WDT0));
  spu_peripheral_config_non_secure((uint32_t)NRF_WDT0, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_WDT1));
  spu_peripheral_config_non_secure((uint32_t)NRF_WDT1, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_COMP));
  spu_peripheral_config_non_secure((uint32_t)NRF_COMP, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_EGU0));
  spu_peripheral_config_non_secure((uint32_t)NRF_EGU0, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_EGU1));
  spu_peripheral_config_non_secure((uint32_t)NRF_EGU1, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_EGU2));
  spu_peripheral_config_non_secure((uint32_t)NRF_EGU2, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_EGU3));
  spu_peripheral_config_non_secure((uint32_t)NRF_EGU3, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_EGU4));
  spu_peripheral_config_non_secure((uint32_t)NRF_EGU4, false);
#ifndef PSA_API_TEST_IPC
  /* EGU5 is used as a secure peripheral in PSA FF tests */

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_EGU5));
  spu_peripheral_config_non_secure((uint32_t)NRF_EGU5, false);
#endif

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_PWM0));
  spu_peripheral_config_non_secure((uint32_t)NRF_PWM0, false); /* Necessary */

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_PWM1));
  spu_peripheral_config_non_secure((uint32_t)NRF_PWM1, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_PWM2));
  spu_peripheral_config_non_secure((uint32_t)NRF_PWM2, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_PWM3));
  spu_peripheral_config_non_secure((uint32_t)NRF_PWM3, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_PDM0));
  spu_peripheral_config_non_secure((uint32_t)NRF_PDM0, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_I2S0));
  spu_peripheral_config_non_secure((uint32_t)NRF_I2S0, false);

  /* IPC remains secure so the radio driver runs in the secure world. */

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_QSPI));
  spu_peripheral_config_non_secure((uint32_t)NRF_QSPI, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_NFCT));
  spu_peripheral_config_non_secure((uint32_t)NRF_NFCT, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_GPIOTE1_NS));
  spu_peripheral_config_non_secure((uint32_t)NRF_GPIOTE1_NS, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_MUTEX));
  spu_peripheral_config_non_secure((uint32_t)NRF_MUTEX, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_QDEC0));
  spu_peripheral_config_non_secure((uint32_t)NRF_QDEC0, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_QDEC1));
  spu_peripheral_config_non_secure((uint32_t)NRF_QDEC1, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_USBD));
  spu_peripheral_config_non_secure((uint32_t)NRF_USBD, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_USBREGULATOR));
  spu_peripheral_config_non_secure((uint32_t)NRF_USBREGULATOR, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_NVMC));
  spu_peripheral_config_non_secure((uint32_t)NRF_NVMC, false); /* Necessary */

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_P0));
  spu_peripheral_config_non_secure((uint32_t)NRF_P0, false); /* Necessary */

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_P1));
  spu_peripheral_config_non_secure((uint32_t)NRF_P1, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_VMC));
  spu_peripheral_config_non_secure((uint32_t)NRF_VMC, false);

#ifndef SECURE_UART1
  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_UARTE1));
  spu_peripheral_config_non_secure((uint32_t)NRF_UARTE1, false);
#endif /* SECURE_UART1 */

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_UARTE2));
  spu_peripheral_config_non_secure((uint32_t)NRF_UARTE2, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_TWIM2));
  spu_peripheral_config_non_secure((uint32_t)NRF_TWIM2, false);

  /* IPC_S remains secure (see IPC comment above). */

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_VMC_S));
  spu_peripheral_config_non_secure((uint32_t)NRF_VMC_S, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_FPU_S));
  spu_peripheral_config_non_secure((uint32_t)NRF_FPU_S, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_EGU1_S));
  spu_peripheral_config_non_secure((uint32_t)NRF_EGU1_S, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_EGU2_S));
  spu_peripheral_config_non_secure((uint32_t)NRF_EGU2_S, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_DPPIC_S));
  spu_peripheral_config_non_secure((uint32_t)NRF_DPPIC_S, false);

  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_REGULATORS_S));
  spu_peripheral_config_non_secure((uint32_t)NRF_REGULATORS_S, false);

  /* DPPI channel configuration */
  spu_dppi_config_non_secure(false);

  /* GPIO pin configuration (P0 and P1 ports) */
  spu_gpio_config_non_secure(0, true);   /* P0.00 to P0.31 */
  spu_gpio_config_non_secure(1, true);   /* P1.00 to P1.15 */

  /*
   * Configure properly the XL1 and XL2 pins so that the low-frequency
   * crystal oscillator (LFXO) can be used. This configuration can be
   * done only from secure code, as otherwise those register fields
   * are not accessible. That's why it is placed here.
   */
  gpio_pin_select(PIN_XL1, GPIO_PIN_SEL_PERIPHERAL);
  gpio_pin_select(PIN_XL2, GPIO_PIN_SEL_PERIPHERAL);

  /*
   * Enable the instruction and data cache (this can be done only from secure
   * code; that's why it is placed here).
   */
  NRF_CACHE->ENABLE = CACHE_ENABLE_ENABLE_Enabled;
}
/******************************************************************************/
void
spu_periph_configure_to_secure(uint32_t periph_num)
{
  spu_peripheral_config_secure(periph_num, true);
}
/******************************************************************************/
void
spu_periph_configure_to_non_secure(uint32_t periph_num)
{
  spu_peripheral_config_non_secure(periph_num, true);
}
/******************************************************************************/
void
spu_periph_config_uarte(void)
{
#ifndef SECURE_UART0
  NVIC_DisableIRQ(NRFX_IRQ_NUMBER_GET(NRF_UARTE0));
  spu_peripheral_config_non_secure((uint32_t)NRF_UARTE0, false);
#endif /* SECURE_UART0 */
}
/******************************************************************************/
void
non_secure_configuration(void)
{
  spu_regions_reset_all_secure();
  /* Hard coded linker script addresses. */
  spu_regions_flash_config_non_secure((uint32_t)NS_CODE_START,
				      (uint32_t)NS_ROM_LIMIT_ADDR);
  spu_regions_sram_config_non_secure((uint32_t)NS_DATA_START,
				     (uint32_t)NS_DATA_LIMIT);
  spu_periph_init_cfg();
}
/******************************************************************************/
void
configure_nonsecure_vtor_offset(uint32_t vtor_ns)
{
  SCB_NS->VTOR = vtor_ns;
}
/******************************************************************************/
void
configure_nonsecure_msp(uint32_t msp_ns)
{
  __TZ_set_MSP_NS(msp_ns);
}
/******************************************************************************/
static void
configure_nonsecure_psp(uint32_t psp_ns)
{
  __TZ_set_PSP_NS(psp_ns);
}
/******************************************************************************/
static void
configure_nonsecure_control(uint32_t spsel_ns, uint32_t npriv_ns)
{
  uint32_t control_ns = __TZ_get_CONTROL_NS();

  /* Only nPRIV and SPSEL bits are banked between security states. */
  control_ns &= ~(CONTROL_SPSEL_Msk | CONTROL_nPRIV_Msk);

  if(spsel_ns) {
    control_ns |= CONTROL_SPSEL_Msk;
  }
  if(npriv_ns) {
    control_ns |= CONTROL_nPRIV_Msk;
  }

  __TZ_set_CONTROL_NS(control_ns);
}
/******************************************************************************/
void
tz_nonsecure_state_setup(const tz_nonsecure_setup_conf_t *p_ns_conf)
{
  configure_nonsecure_vtor_offset(p_ns_conf->vtor_ns);
  configure_nonsecure_msp(p_ns_conf->msp_ns);
  configure_nonsecure_psp(p_ns_conf->psp_ns);

  /* Select which stack pointer to use (MSP or PSP) and the privilege
     level for thread mode. */
  configure_nonsecure_control(p_ns_conf->control_ns.spsel,
                              p_ns_conf->control_ns.npriv);
}
/******************************************************************************/
