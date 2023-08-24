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
 * \brief TFM error codes.
 */
enum tfm_plat_err_t {
  TFM_PLAT_ERR_SUCCESS = 0,
  TFM_PLAT_ERR_SYSTEM_ERR = 0x3A5C,
  TFM_PLAT_ERR_MAX_VALUE = 0x55A3,
  TFM_PLAT_ERR_INVALID_INPUT = 0xA3C5,
  TFM_PLAT_ERR_UNSUPPORTED = 0xC35A,
  /* Following entry is only to ensure the error code of int size */
  /* TFM_PLAT_ERR_FORCE_INT_SIZE = INT_MAX */
};

#define TFM_DRIVER_STDIO    Driver_USART1
#define NS_DRIVER_STDIO     Driver_USART0

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
 * \brief Configures memory permissions via the System Protection Unit.
 *
 * \return Returns values as specified by the \ref tfm_plat_err_t
 */
enum tfm_plat_err_t spu_init_cfg(void);

/**
 * \brief Configures peripheral permissions via the System Protection Unit.
 *
 * The function does the following:
 * - grants Non-Secure access to nRF peripherals that are not Secure-only
 * - grants Non-Secure access to DDPI channels
 * - grants Non-Secure access to GPIO pins
 *
 * \return Returns values as specified by the \ref tfm_plat_err_t
 */
enum tfm_plat_err_t spu_periph_init_cfg(void);

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
 * \brief Clears SPU interrupt.
 */
void spu_clear_irq(void);

/**
 * \brief Configures SAU and IDAU.
 */
void sau_and_idau_cfg(void);

/**
 * \brief Configure rom, ram and peripherials non-secure
 */
void non_secure_configuration(void);

/**
 * \brief Get non-secure vector table.
 */
uint32_t tfm_spm_hal_get_ns_VTOR(void);

/**
 * \brief Get non-secure MSP location.
 */
uint32_t tfm_spm_hal_get_ns_MSP(void);

/**
 * \brief Get entry point location.
 */
uint32_t tfm_spm_hal_get_ns_entry_point(void);

/**
 * \brief Enables the fault handlers and sets priorities.
 *
 * \return Returns values as specified by the \ref tfm_plat_err_t
 */
enum tfm_plat_err_t enable_fault_handlers(void);

/**
 * \brief Configures the system reset request properties
 *
 * \return Returns values as specified by the \ref tfm_plat_err_t
 */
enum tfm_plat_err_t system_reset_cfg(void);

/**
 * \brief Configures the system debug properties.
 *
 * \return Returns values as specified by the \ref tfm_plat_err_t
 */
enum tfm_plat_err_t init_debug(void);

/**
 * \brief Configures all external interrupts to target the
 *        NS state, apart for the ones associated to secure
 *        peripherals (plus SPU)
 *
 * \return Returns values as specified by the \ref tfm_plat_err_t
 */
enum tfm_plat_err_t nvic_interrupt_target_state_cfg(void);

/**
 * \brief This function enable the interrupts associated
 *        to the secure peripherals (plus the isolation boundary violation
 *        interrupts)
 *
 * \return Returns values as specified by the \ref tfm_plat_err_t
 */
enum tfm_plat_err_t nvic_interrupt_enable(void);

#endif /* __TARGET_CFG_H__ */
