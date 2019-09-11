#ifndef VESNA_DEF_H
#define VESNA_DEF_H

#include <stdint.h>

typedef union stm32f1_uuid {
	uint32_t u32[3];
	uint16_t u16[6];
	uint8_t u8[12];
} stm32f1_uuid_t;

// STM32 has 96 bits "universal unique ID" on address 0x1FFFF7E8
#define STM32F1_UUID	(*(const stm32f1_uuid_t * const)0x1FFFF7E8)

#endif
