#ifndef NRF54L15_TZ_SPU_H_
#define NRF54L15_TZ_SPU_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <nrfx.h>
#include <hal/nrf_mpc.h>
#include <hal/nrf_spu.h>

#define SPU_LOCK_CONF_LOCKED true
#define SPU_LOCK_CONF_UNLOCKED false
#define SPU_SECURE_ATTR_SECURE true
#define SPU_SECURE_ATTR_NONSECURE false

enum spu_events {
  SPU_EVENT_PERIPHACCERR = 1 << 0,
  MPC_EVENT_MEMACCERR = 1 << 1
};

static NRF_SPU_Type * const spu_instances[] = {
#ifdef NRF_SPU00
  NRF_SPU00,
#endif
#ifdef NRF_SPU10
  NRF_SPU10,
#endif
#ifdef NRF_SPU20
  NRF_SPU20,
#endif
#ifdef NRF_SPU30
  NRF_SPU30,
#endif
};

void spu_enable_interrupts(void);
uint32_t spu_events_get(void);
void spu_clear_events(void);

void mpc_enable_interrupts(void);
uint32_t mpc_events_get(void);
void mpc_clear_events(void);

void spu_regions_reset_all_secure(void);
void spu_regions_flash_config_non_secure(uint32_t start_addr, uint32_t limit_addr);
void spu_regions_sram_config_non_secure(uint32_t start_addr, uint32_t limit_addr);
void spu_regions_flash_config_non_secure_callable(uint32_t start_addr, uint32_t limit_addr);
void spu_peripheral_config_secure(uint32_t periph_base_addr, bool periph_lock);
void spu_peripheral_config_non_secure(uint32_t periph_base_addr, bool periph_lock);
static inline void spu_dppi_config_non_secure(bool dppi_lock) { (void)dppi_lock; }
static inline void spu_gpio_config_non_secure(uint8_t port_number, bool gpio_lock)
{
  (void)port_number;
  (void)gpio_lock;
}
uint32_t spu_get_peri_addr(void);
uint32_t spu_regions_flash_get_base_address_in_region(uint32_t region_id);
uint32_t spu_regions_flash_get_last_address_in_region(uint32_t region_id);
uint32_t spu_regions_flash_get_start_id(void);
uint32_t spu_regions_flash_get_last_id(void);
uint32_t spu_regions_flash_get_region_size(void);
uint32_t spu_regions_sram_get_base_address_in_region(uint32_t region_id);
uint32_t spu_regions_sram_get_last_address_in_region(uint32_t region_id);
uint32_t spu_regions_sram_get_start_id(void);
uint32_t spu_regions_sram_get_last_id(void);
uint32_t spu_regions_sram_get_region_size(void);

static inline NRF_SPU_Type *
spu_instance_from_peripheral_addr(uint32_t peripheral_addr)
{
  uint32_t apb_bus_number = peripheral_addr & 0x00FC0000u;

  return (NRF_SPU_Type *)(0x50000000u | apb_bus_number);
}

#endif /* NRF54L15_TZ_SPU_H_ */
