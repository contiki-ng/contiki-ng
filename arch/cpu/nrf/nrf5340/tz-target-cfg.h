/*
 * Copyright (c) 2017-2019 Arm Limited
 * Copyright (c) 2020 Nordic Semiconductor ASA
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

#ifndef __TZ_TARGET_CFG_H__
#define __TZ_TARGET_CFG_H__

/**
 * \file tz-target-cfg.h
 * \brief nRF5340 target configuration header
 *
 * This file contains the platform specific functions to configure
 * the Cortex-M33 core, memory permissions and security attribution
 * on the nRF5340 platform.
 *
 * Memory permissions and security attribution are configured via
 * the System Protection Unit (SPU) which is the nRF specific Implementation
 * Defined Attribution Unit (IDAU).
 */

/**
 * \brief A convenient struct to include all required Non-Secure state configuration.
 */
typedef struct tz_nonsecure_setup_conf {
  uint32_t msp_ns;
  uint32_t psp_ns;
  uint32_t vtor_ns;
  struct {
    uint32_t npriv : 1;
    uint32_t spsel : 1;
    uint32_t reserved : 30;
  } control_ns;
} tz_nonsecure_setup_conf_t;

/**
 * \brief Configure nonsecure vtor offset
 */
void configure_nonsecure_vtor_offset(uint32_t vtor_ns);

/**
 * \brief Store the addresses of memory regions
 */
struct memory_region_limits {
  uint32_t non_secure_code_start;
  uint32_t non_secure_partition_base;
  uint32_t non_secure_partition_limit;
  uint32_t veneer_base;
  uint32_t veneer_limit;
#ifdef BL2
  uint32_t secondary_partition_base;
  uint32_t secondary_partition_limit;
#endif /* BL2 */
};

/**
 * \brief Holds the data necessary to do isolation for a specific peripheral.
 */
struct platform_data_t {
  uint32_t periph_start;
  uint32_t periph_limit;
};

/**
 * \brief Configures peripheral permissions via the System Protection Unit.
 *
 * The function does the following:
 * - grants Non-Secure access to nRF peripherals that are not Secure-only
 * - grants Non-Secure access to DDPI channels
 * - grants Non-Secure access to GPIO pins
 */
void spu_periph_init_cfg(void);

/**
 * \brief Setup nonsecure state
 */
void tz_nonsecure_state_setup(const tz_nonsecure_setup_conf_t *p_ns_conf);

/**
 * \brief Restrict access to peripheral to secure
 */
void spu_periph_configure_to_secure(uint32_t periph_num);

/**
 * \brief Allow non-secure access to peripheral
 */
void spu_periph_configure_to_non_secure(uint32_t periph_num);

/**
 * \brief Configures the NRF_UARTE0 non-secure
 */
void spu_periph_config_uarte(void);

/**
 * \brief Configures SAU and IDAU.
 */
void sau_and_idau_cfg(void);

/**
 * \brief Configure rom, ram and peripherials non-secure
 */
void non_secure_configuration(void);

/**
 * \brief Enables the fault handlers and sets priorities.
 */
void enable_fault_handlers(void);

/**
 * \brief Configures the system reset request properties
 */
void system_reset_cfg(void);

/**
 * \brief Configures all external interrupts to target the
 *        NS state, apart for the ones associated to secure
 *        peripherals (plus SPU)
 */
void nvic_interrupt_target_state_cfg(void);

/**
 * \brief This function enable the interrupts associated
 *        to the secure peripherals (plus the isolation boundary violation
 *        interrupts)
 */
void nvic_interrupt_enable(void);

/**
 * \brief Report and clear any SPU violation captured by the previous
 *        boot's SPU_IRQHandler. Should be called early in secure
 *        initialization to surface the cause of an unexpected reset.
 */
void spu_report_violation(void);

#endif /* __TZ_TARGET_CFG_H__ */
