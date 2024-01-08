#ifndef _NSC_H_
#define _NSC_H_

#include "NuMicro.h"

__NONSECURE_ENTRY
void SYS_ResetModule_S(uint32_t u32ModuleIndex);

__NONSECURE_ENTRY
void CLK_SetModuleClock_S(uint32_t u32ModuleIdx, uint32_t u32ClkSrc, uint32_t u32ClkDiv);
__NONSECURE_ENTRY
void CLK_EnableModuleClock_S(uint32_t u32ModuleIdx);

__NONSECURE_ENTRY
void CLK_DisableModuleClock_S(uint32_t u32ModuleIdx);

__NONSECURE_ENTRY
void pin_function_s(int32_t port, int32_t pin, int32_t data);

__NONSECURE_ENTRY
void I2C_Close_S(I2C_T *i2c);

void reset_chip(void);


#endif

