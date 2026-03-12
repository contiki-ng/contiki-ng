#include "tz-target-cfg.h"
#include "region_defs.h"
#include "spu.h"

#include <arm_cmse.h>
#include <nrfx.h>

#define SCB_AIRCR_WRITE_MASK (0x5FAUL << SCB_AIRCR_VECTKEY_Pos)
#define MPC_OVERRIDE_LAST_INDEX 4u

extern uint32_t __sg_start;
extern uint32_t __sg_end;

volatile uint32_t tz_debug_last_route_irq;
volatile uint32_t tz_debug_last_route_itns_word;
volatile uint32_t tz_debug_last_route_itns_value;
volatile uint32_t tz_debug_last_route_target_state;
volatile uint32_t tz_debug_last_route_iser_ns;

static void
sau_region_configure(uint32_t region, uint32_t base, uint32_t limit, bool nsc)
{
  SAU->RNR = region;
  SAU->RBAR = base & SAU_RBAR_BADDR_Msk;
  SAU->RLAR = (limit & SAU_RLAR_LADDR_Msk)
    | (nsc ? SAU_RLAR_NSC_Msk : 0u)
    | SAU_RLAR_ENABLE_Msk;
}

static void
keep_irq_secure(IRQn_Type irqn)
{
  NVIC_DisableIRQ(irqn);
  NVIC_ClearPendingIRQ(irqn);
  NVIC_ClearTargetState(irqn);
  NVIC_EnableIRQ(irqn);
}

static void
route_irq_non_secure(IRQn_Type irqn)
{
  uint32_t irq_number = (uint32_t)irqn;
  uint32_t word = irq_number >> 5;
  uint32_t mask = 1u << (irq_number & 31u);

  NVIC_DisableIRQ(irqn);
  NVIC_ClearPendingIRQ(irqn);
  NVIC->ITNS[word] |= mask;
  tz_debug_last_route_irq = irq_number;
  tz_debug_last_route_itns_word = word;
  tz_debug_last_route_target_state = NVIC_SetTargetState(irqn);
  /*
   * Use the TrustZone-specific CMSIS helpers so we touch the non-secure NVIC
   * bank explicitly from secure state.
   */
  TZ_NVIC_ClearPendingIRQ_NS(irqn);
  TZ_NVIC_EnableIRQ_NS(irqn);
  tz_debug_last_route_itns_value = NVIC->ITNS[word];
  tz_debug_last_route_iser_ns = TZ_NVIC_GetEnableIRQ_NS(irqn);
}

static void
set_grtc_feature_security(nrf_spu_feature_t feature, uint8_t index, bool secure)
{
  NRF_SPU_Type *spu = spu_instance_from_peripheral_addr(NRF_GRTC_S_BASE);

  nrf_spu_feature_secattr_set(spu, feature, index, 0, secure);
  nrf_spu_feature_lock_enable(spu, feature, index, 0);
}

static void
configure_grtc_security_split(void)
{
  for(uint8_t channel = 0; channel < NRF_SPU_FEATURE_GRTC_CC_COUNT; channel++) {
    bool secure = !(channel <= 2u);

    set_grtc_feature_security(NRF_SPU_FEATURE_GRTC_CC, channel, secure);
  }

  for(uint8_t group = 0; group < NRF_SPU_FEATURE_GRTC_INTERRUPT_COUNT; group++) {
    bool secure = group != 1u;

    set_grtc_feature_security(NRF_SPU_FEATURE_GRTC_INTERRUPT, group, secure);
  }

  for(uint8_t group = 0; group < GRTC_SYSCOUNTER_MaxCount; group++) {
    bool secure = group != 1u;

    set_grtc_feature_security(NRF_SPU_FEATURE_GRTC_SYSCOUNTER, group, secure);
  }
#ifdef NRF_SPU_FEATURE_GRTC_CLK
  set_grtc_feature_security(NRF_SPU_FEATURE_GRTC_CLK, 0, true);
#endif
}

struct mpc_region_override {
  nrf_mpc_override_config_t config;
  nrf_owner_t owner_id;
  uintptr_t start_address;
  size_t end_address;
  uint32_t perm;
  uint32_t perm_mask;
  size_t index;
};

static void
mpc_configure_override(NRF_MPC_Type *mpc, struct mpc_region_override *override)
{
  nrf_mpc_override_startaddr_set(mpc, override->index, override->start_address);
  nrf_mpc_override_endaddr_set(mpc, override->index, override->end_address);
  nrf_mpc_override_perm_set(mpc, override->index, override->perm);
  nrf_mpc_override_permmask_set(mpc, override->index, override->perm_mask);
#if defined(NRF_MPC_HAS_OVERRIDE_OWNERID) && NRF_MPC_HAS_OVERRIDE_OWNERID
  nrf_mpc_override_ownerid_set(mpc, override->index, override->owner_id);
#endif
  nrf_mpc_override_config_set(mpc, override->index, &override->config);
}

static void
init_mpc_region_override(struct mpc_region_override *override)
{
  *override = (struct mpc_region_override){
    .config = {
      .slave_number = 0,
      .lock = true,
      .enable = true,
      .secdom_enable = false,
      .secure_mask = true,
    },
    .owner_id = 0,
    .perm = 0,
    .perm_mask = MPC_OVERRIDE_PERM_SECATTR_Msk,
  };
}

static enum tfm_plat_err_t
nrf_mpc_init_cfg(void)
{
  size_t index = 0u;

  {
    struct mpc_region_override override;

    init_mpc_region_override(&override);
    override.start_address = NS_PARTITION_START;
    override.end_address = NRF_UICR_S_BASE;
    override.index = index++;
    mpc_configure_override(NRF_MPC00, &override);
  }

  {
    struct mpc_region_override override;

    init_mpc_region_override(&override);
    override.start_address = NS_DATA_START;
    override.end_address = NS_DATA_LIMIT + 1u;
    override.index = index++;
    mpc_configure_override(NRF_MPC00, &override);
  }

  if(index > MPC_OVERRIDE_LAST_INDEX + 1u) {
    return TFM_PLAT_ERR_SYSTEM_ERR;
  }

  while(index <= MPC_OVERRIDE_LAST_INDEX) {
    struct mpc_region_override override;

    init_mpc_region_override(&override);
    override.config.enable = false;
    override.index = index++;
    mpc_configure_override(NRF_MPC00, &override);
  }

  return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t
enable_fault_handlers(void)
{
  NVIC_SetPriority(SecureFault_IRQn, 0);
  SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk
    | SCB_SHCSR_BUSFAULTENA_Msk
    | SCB_SHCSR_MEMFAULTENA_Msk
    | SCB_SHCSR_SECUREFAULTENA_Msk;

  return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t
system_reset_cfg(void)
{
  uint32_t reg_value = SCB->AIRCR;

  reg_value &= ~(uint32_t)SCB_AIRCR_VECTKEY_Msk;
  reg_value |= SCB_AIRCR_WRITE_MASK | SCB_AIRCR_SYSRESETREQS_Msk;
  SCB->AIRCR = reg_value;

  return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t
init_debug(void)
{
  return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t
spu_init_cfg(void)
{
  return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t
spu_periph_init_cfg(void)
{
#ifdef SECURE_UART20
  spu_peripheral_config_secure(NRF_UARTE20_S_BASE, SPU_LOCK_CONF_UNLOCKED);
#endif

  /*
   * The full TF-M policy resets all SPU blocks to non-secure defaults first,
   * then reapplies secure ownership. On nRF54L15 that broad memset is still
   * risky in this Contiki port, so keep the first hardware bring-up explicit.
   */
  spu_peripheral_config_secure(NRF_MEMCONF_S_BASE, SPU_LOCK_CONF_LOCKED);
  spu_peripheral_config_secure(NRF_REGULATORS_S_BASE, SPU_LOCK_CONF_LOCKED);
  spu_peripheral_config_secure(NRF_CTRLAP_S_BASE, SPU_LOCK_CONF_LOCKED);
  spu_peripheral_config_secure(NRF_TAD_S_BASE, SPU_LOCK_CONF_LOCKED);
  /*
   * Clock and oscillator control stays secure. The non-secure image must
   * consume the already-bootstrapped timer domains rather than reprogramming
   * shared clock hardware.
   */
  spu_peripheral_config_secure(NRF_CLOCK_S_BASE, SPU_LOCK_CONF_UNLOCKED);
  /*
   * On nRF54L15 the GRTC peripheral itself must stay non-secure so the normal
   * world can read the syscounter and use its own interrupt group. Secure radio
   * support is enforced by the feature split below instead of by marking the
   * whole peripheral secure.
   */
  spu_peripheral_config_non_secure(NRF_GRTC_S_BASE, SPU_LOCK_CONF_UNLOCKED);
  configure_grtc_security_split();

  return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t
nvic_interrupt_target_state_cfg(void)
{
  for(size_t i = 0; i < sizeof(NVIC->ITNS) / sizeof(NVIC->ITNS[0]); i++) {
    NVIC->ITNS[i] = 0u;
  }

  keep_irq_secure(SPU00_IRQn);
  keep_irq_secure(SPU10_IRQn);
  keep_irq_secure(SPU20_IRQn);
  keep_irq_secure(SPU30_IRQn);
  keep_irq_secure(MPC00_IRQn);
  keep_irq_secure(GRTC_0_IRQn);
  route_irq_non_secure(GRTC_1_IRQn);
  keep_irq_secure(GRTC_2_IRQn);
  keep_irq_secure(GRTC_3_IRQn);
  keep_irq_secure(GPIOTE20_0_IRQn);
  keep_irq_secure(GPIOTE20_1_IRQn);
  keep_irq_secure(GPIOTE30_0_IRQn);
  keep_irq_secure(GPIOTE30_1_IRQn);
#ifdef SECURE_UART20
  keep_irq_secure(UARTE20_IRQn);
#endif

  return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t
nvic_interrupt_enable(void)
{
  spu_enable_interrupts();

  NVIC_ClearPendingIRQ(SPU00_IRQn);
  NVIC_EnableIRQ(SPU00_IRQn);
  NVIC_ClearPendingIRQ(SPU10_IRQn);
  NVIC_EnableIRQ(SPU10_IRQn);
  NVIC_ClearPendingIRQ(SPU20_IRQn);
  NVIC_EnableIRQ(SPU20_IRQn);
  NVIC_ClearPendingIRQ(SPU30_IRQn);
  NVIC_EnableIRQ(SPU30_IRQn);
  NVIC_ClearPendingIRQ(MPC00_IRQn);
  NVIC_EnableIRQ(MPC00_IRQn);

  return TFM_PLAT_ERR_SUCCESS;
}

void
sau_and_idau_cfg(void)
{
  TZ_SAU_Disable();

  sau_region_configure(0, NS_PARTITION_START, NRF_UICR_S_BASE, false);
  sau_region_configure(2, (uint32_t)&__sg_start, (uint32_t)&__sg_end - 1u, true);
  sau_region_configure(3, NS_DATA_START, 0xffffffffu, false);

  __DSB();
  __ISB();
  SAU->CTRL = SAU_CTRL_ENABLE_Msk;
}

void
non_secure_configuration(void)
{
  (void)spu_init_cfg();
  (void)nrf_mpc_init_cfg();
  (void)spu_periph_init_cfg();
  (void)system_reset_cfg();
  (void)init_debug();
  (void)nvic_interrupt_enable();
}

void
configure_nonsecure_vtor_offset(uint32_t vtor_ns)
{
  SCB_NS->VTOR = vtor_ns;
  __DSB();
  __ISB();
}

void
tz_nonsecure_state_setup(const tz_nonsecure_setup_conf_t *p_ns_conf)
{
  configure_nonsecure_vtor_offset(p_ns_conf->vtor_ns);
  __TZ_set_MSP_NS(p_ns_conf->msp_ns);
  __TZ_set_PSP_NS(p_ns_conf->psp_ns);
  __TZ_set_CONTROL_NS((p_ns_conf->control_ns.npriv << 0)
                      | (p_ns_conf->control_ns.spsel << 1));
#ifdef TZ_DEBUG_MASK_NS_IRQS
  /* Phase-1 hello-world bring-up: keep NS interrupt-free until the
   * final nRF54L15 IRQ ownership model is in place. */
  __TZ_set_PRIMASK_NS(1);
#else
  __TZ_set_PRIMASK_NS(0);
#endif
  __DSB();
  __ISB();
}

void
spu_periph_configure_to_secure(uint32_t periph_num)
{
  spu_peripheral_config_secure(periph_num, SPU_LOCK_CONF_UNLOCKED);
}

void
spu_periph_configure_to_non_secure(uint32_t periph_num)
{
  spu_peripheral_config_non_secure(periph_num, SPU_LOCK_CONF_UNLOCKED);
}

void
spu_periph_config_uarte(void)
{
#ifdef SECURE_UART20
  NVIC_DisableIRQ(UARTE20_IRQn);
  NVIC_ClearPendingIRQ(UARTE20_IRQn);
  NVIC_SetTargetState(UARTE20_IRQn);
  spu_peripheral_config_non_secure(NRF_UARTE20_S_BASE, SPU_LOCK_CONF_UNLOCKED);
  __DSB();
  __ISB();
#endif
}

void
spu_clear_irq(void)
{
  spu_clear_events();
  mpc_clear_events();

  NVIC_ClearPendingIRQ(SPU00_IRQn);
  NVIC_ClearPendingIRQ(SPU10_IRQn);
  NVIC_ClearPendingIRQ(SPU20_IRQn);
  NVIC_ClearPendingIRQ(SPU30_IRQn);
  NVIC_ClearPendingIRQ(MPC00_IRQn);
}

uint32_t
tfm_spm_hal_get_ns_VTOR(void)
{
  return NS_CODE_START;
}

uint32_t
tfm_spm_hal_get_ns_MSP(void)
{
  return *((uint32_t *)NS_CODE_START);
}

uint32_t
tfm_spm_hal_get_ns_entry_point(void)
{
  return *((uint32_t *)(NS_CODE_START + 4u));
}
