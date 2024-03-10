#include <inttypes.h>
#include "NuMicro.h"

int32_t SH_Return(int32_t n32In_R0, int32_t n32In_R1, int32_t *pn32Out_R0)
{
    (void)n32In_R0;
    (void)n32In_R1;
    (void)pn32Out_R0;
    return 0;
}

uint32_t ProcessHardFault(uint32_t lr, uint32_t msp, uint32_t psp)
{
	uint32_t *sp = (uint32_t *)psp;

	/* It is casued by hardfault. Just process the hard fault */
	/* TODO: Implement your hardfault handle code here */

	/* Check the used stack */
	if(lr & 0x40UL)
	{
		/* Secure stack used */
		if(lr & 4UL)
		{
			sp = (uint32_t *)psp;
		}
		else
		{
			sp = (uint32_t *)msp;
		}

	}
#if defined (__ARM_FEATURE_CMSE) &&  (__ARM_FEATURE_CMSE == 3)    
	else
	{
		/* Non-secure stack used */
		if(lr & 4)
			sp = (uint32_t *)__TZ_get_PSP_NS();
		else
			sp = (uint32_t *)__TZ_get_MSP_NS();

	}
#endif

	printf("  HardFault!\n\n");
	printf("r0  = 0x%" PRIx32 "\n", sp[0]);
	printf("r1  = 0x%" PRIx32 "\n", sp[1]);
	printf("r2  = 0x%" PRIx32 "\n", sp[2]);
	printf("r3  = 0x%" PRIx32 "\n", sp[3]);
	printf("r12 = 0x%" PRIx32 "\n", sp[4]);
	printf("lr  = 0x%" PRIx32 "\n", sp[5]);
	printf("pc  = 0x%" PRIx32 "\n", sp[6]);
	printf("psr = 0x%" PRIx32 "\n", sp[7]);

	/* Or *sp to remove compiler warning */
	while(1U|*sp){}

	return lr;
}

__attribute__((naked)) void HardFault_Handler(void)
{
	__asm__ volatile
		(
		 "mov r0, lr          \n"
		 "mrs r1, msp         \n"
		 "mrs r2, psp         \n"
		 "bl ProcessHardFault \n"
		 :
		 :
		 : "r0","r4","r5","r6","lr"
		);
}

