#ifndef NRF54L15_TZ_TARGET_CFG_H_
#define NRF54L15_TZ_TARGET_CFG_H_

#include <stdint.h>

enum tfm_plat_err_t {
  TFM_PLAT_ERR_SUCCESS = 0,
  TFM_PLAT_ERR_SYSTEM_ERR = 0x3A5C,
  TFM_PLAT_ERR_MAX_VALUE = 0x55A3,
  TFM_PLAT_ERR_INVALID_INPUT = 0xA3C5,
  TFM_PLAT_ERR_UNSUPPORTED = 0xC35A,
};

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

void configure_nonsecure_vtor_offset(uint32_t vtor_ns);
enum tfm_plat_err_t spu_init_cfg(void);
enum tfm_plat_err_t spu_periph_init_cfg(void);
void tz_nonsecure_state_setup(const tz_nonsecure_setup_conf_t *p_ns_conf);
void spu_periph_configure_to_secure(uint32_t periph_num);
void spu_periph_configure_to_non_secure(uint32_t periph_num);
void spu_periph_config_uarte(void);
void spu_clear_irq(void);
void sau_and_idau_cfg(void);
void non_secure_configuration(void);
uint32_t tfm_spm_hal_get_ns_VTOR(void);
uint32_t tfm_spm_hal_get_ns_MSP(void);
uint32_t tfm_spm_hal_get_ns_entry_point(void);
enum tfm_plat_err_t enable_fault_handlers(void);
enum tfm_plat_err_t system_reset_cfg(void);
enum tfm_plat_err_t init_debug(void);
enum tfm_plat_err_t nvic_interrupt_target_state_cfg(void);
enum tfm_plat_err_t nvic_interrupt_enable(void);

#endif /* NRF54L15_TZ_TARGET_CFG_H_ */
