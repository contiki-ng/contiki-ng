#include "spu.h"
#include "region_defs.h"

#define SPU_INSTANCE_COUNT (sizeof(spu_instances) / sizeof(spu_instances[0]))

void
spu_enable_interrupts(void)
{
  for(size_t i = 0; i < SPU_INSTANCE_COUNT; i++) {
    nrf_spu_int_enable(spu_instances[i], NRF_SPU_INT_PERIPHACCERR_MASK);
  }

  mpc_enable_interrupts();
}

uint32_t
spu_events_get(void)
{
  uint32_t events = 0;

  for(size_t i = 0; i < SPU_INSTANCE_COUNT; i++) {
    if(nrf_spu_event_check(spu_instances[i], NRF_SPU_EVENT_PERIPHACCERR)) {
      events |= SPU_EVENT_PERIPHACCERR;
    }
  }

  events |= mpc_events_get();

  return events;
}

void
spu_clear_events(void)
{
  for(size_t i = 0; i < SPU_INSTANCE_COUNT; i++) {
    nrf_spu_event_clear(spu_instances[i], NRF_SPU_EVENT_PERIPHACCERR);
  }
}

void
mpc_enable_interrupts(void)
{
  nrf_mpc_int_enable(NRF_MPC00, NRF_MPC_INT_MEMACCERR_MASK);
}

uint32_t
mpc_events_get(void)
{
  return nrf_mpc_event_check(NRF_MPC00, NRF_MPC_EVENT_MEMACCERR) ? MPC_EVENT_MEMACCERR : 0u;
}

void
mpc_clear_events(void)
{
  nrf_mpc_event_clear(NRF_MPC00, NRF_MPC_EVENT_MEMACCERR);
}

void
spu_regions_reset_all_secure(void)
{
}

void
spu_regions_flash_config_non_secure(uint32_t start_addr, uint32_t limit_addr)
{
  (void)start_addr;
  (void)limit_addr;
}

void
spu_regions_sram_config_non_secure(uint32_t start_addr, uint32_t limit_addr)
{
  (void)start_addr;
  (void)limit_addr;
}

void
spu_regions_flash_config_non_secure_callable(uint32_t start_addr, uint32_t limit_addr)
{
  (void)start_addr;
  (void)limit_addr;
}

static void
spu_peripheral_configure(uint32_t periph_base_addr, bool secure, bool periph_lock)
{
  NRF_SPU_Type *nrf_spu = spu_instance_from_peripheral_addr(periph_base_addr);
  uint8_t periph_id = NRFX_PERIPHERAL_ID_GET(periph_base_addr);
  uint8_t spu_id = NRFX_PERIPHERAL_ID_GET((uint32_t)nrf_spu);
  uint8_t index = periph_id - spu_id;

  nrf_spu_periph_perm_secattr_set(nrf_spu, index, secure);
  nrf_spu_periph_perm_dmasec_set(nrf_spu, index, secure);
  if(periph_lock) {
    nrf_spu_periph_perm_lock_enable(nrf_spu, index);
  }
}

void
spu_peripheral_config_secure(uint32_t periph_base_addr, bool periph_lock)
{
  spu_peripheral_configure(periph_base_addr, true, periph_lock);
}

void
spu_peripheral_config_non_secure(uint32_t periph_base_addr, bool periph_lock)
{
  spu_peripheral_configure(periph_base_addr, false, periph_lock);
}

uint32_t
spu_get_peri_addr(void)
{
  uint32_t addr = 0u;

  for(size_t i = 0; i < SPU_INSTANCE_COUNT; i++) {
    if(spu_instances[i]->EVENTS_PERIPHACCERR != 0u) {
      addr = spu_instances[i]->PERIPHACCERR.ADDRESS | ((uint32_t)spu_instances[i] & 0xFFFF0000u);
    }
  }

  return addr;
}

uint32_t
spu_regions_flash_get_base_address_in_region(uint32_t region_id)
{
  return FLASH_BASE_ADDRESS + (region_id * MPC_FLASH_REGION_SIZE);
}

uint32_t
spu_regions_flash_get_last_address_in_region(uint32_t region_id)
{
  return spu_regions_flash_get_base_address_in_region(region_id)
    + MPC_FLASH_REGION_SIZE - 1u;
}

uint32_t
spu_regions_flash_get_start_id(void)
{
  return 0u;
}

uint32_t
spu_regions_flash_get_last_id(void)
{
  return (FLASH_TOTAL_SIZE / MPC_FLASH_REGION_SIZE) - 1u;
}

uint32_t
spu_regions_flash_get_region_size(void)
{
  return MPC_FLASH_REGION_SIZE;
}

uint32_t
spu_regions_sram_get_base_address_in_region(uint32_t region_id)
{
  return SRAM_BASE_ADDRESS + (region_id * MPC_SRAM_REGION_SIZE);
}

uint32_t
spu_regions_sram_get_last_address_in_region(uint32_t region_id)
{
  return spu_regions_sram_get_base_address_in_region(region_id)
    + MPC_SRAM_REGION_SIZE - 1u;
}

uint32_t
spu_regions_sram_get_start_id(void)
{
  return 0u;
}

uint32_t
spu_regions_sram_get_last_id(void)
{
  return (TOTAL_RAM_SIZE / MPC_SRAM_REGION_SIZE) - 1u;
}

uint32_t
spu_regions_sram_get_region_size(void)
{
  return MPC_SRAM_REGION_SIZE;
}
