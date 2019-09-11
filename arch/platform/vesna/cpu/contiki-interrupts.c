/**
 ******************************************************************************
 * @file    CortexM3/BitBand/stm32f10x_it.c
 * @author  MCD Application Team
 * @version V3.1.2
 * @date    09/28/2009
 *  Main Interrupt Service Routines.
 *          This file provides template for all exceptions handler and peripherals
 *          interrupt service routine.
 ******************************************************************************
 * @copy
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
 * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
 */

/* Includes ------------------------------------------------------------------*/
#include "newlib.h"
#include "vsn.h"
#include "stm32f10x_it.h"
#include "vsntime.h"
#include "vsnusart.h"
#include "vsnpm.h"
#include "vsnsd.h"
#include "vsnsetup.h"
#include "vsnledind.h"

#include "vsnresetbutton.h"

#include "rtimer-arch.h"
#include "sys/clock.h"

#include "dev/uart1.h"
#include "at86rf2xx/rf2xx.h"


extern void clock_interrupt_handler(void);



/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * This function handles NMI exception.
 * @param  None
 * @return None
 */
void NMI_Handler(void)
{
	/* This interrupt is generated when HSE clock fails, nothing can be printed here because the
	 * SysClock has changed */
	if (RCC_GetITStatus(RCC_IT_CSS) != RESET) {
		/* At this stage: HSE, PLL are disabled (but no change on PLL config) and HSI
	       is selected as system clock source */
		/* Enable HSE */
		RCC_HSEConfig(RCC_HSE_ON);
		/* Enable HSE Ready interrupt */
		RCC_ITConfig(RCC_IT_HSERDY, ENABLE);
        /* Enable PLL Ready interrupt */
		RCC_ITConfig(RCC_IT_PLLRDY, ENABLE);
		/* Clear Clock Security System interrupt pending bit */
		RCC_ClearITPendingBit(RCC_IT_CSS);
		/* Once HSE clock recover, the HSERDY interrupt is generated and in the RCC ISR
		 routine the system clock will be reconfigured to its previous state (before
		 HSE clock failure) */
		/* TODO if HSE fails completely we have to reinitialize clock dependent drivers or restart the system */
	}
}


unsigned int faultStack[51];
unsigned int *stackPointer;
/**
 * This function handles Hard Fault exception.
 * Useful documents for debugging Hard Faults: 	Application Note 209: Using Cortex-M3 and Cortex-M4 Fault Exceptions (apnt209.pdf)
 * 												Cortex-M3 Devices Generic User Guide (DUI0552A_cortex_m3_dgug.pdf)
 * When Hard Fault occurs some system registers are pushed to stack. The Hard Fault Handler
 * saves the stack pointer and creates a new fault stack in case the stack pointer or
 * the stack is corrupted. The registers that are pushed to stack are printed to debug
 * port along with the fault status registers
 * @param  None
 * @return None
 *
 * @TODO Implement the HardFault_Handler for the process stack (PSP), check actual fault stack size needed, maybe reset MCU after hard fault
 */
void HardFault_Handler(void)
{
	asm (	"mrs	%[origStackPtr], msp\n\t"		/* Get the stack pointer */
			"msr	msp, %[newStackPtr]\n\t"     	/* Set the new stack MSP to fault_stack */
			"push	{r7, lr}\n\t"					/* Start the ISR on the new stack */
			"add 	r7, sp, #0\n\t"
			:[origStackPtr] "=r" (stackPointer)
			:[newStackPtr] "r" (&(faultStack[51]))
		);
	/* Disable all interrupts */
	__disable_irq();
	/* Disable both DMAs */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, DISABLE);

	debug_str("\r\nHARD FAULT EXCEPTION");
	debug_str("\r\nR0 =  ");
	debug_hex(*(stackPointer + STACKED_R0_OFFSET), 8);
	debug_str("\r\nR1 = ");
	debug_hex(*(stackPointer + STACKED_R1_OFFSET), 8);
	debug_str("\r\nR2 = ");
	debug_hex(*(stackPointer + STACKED_R2_OFFSET), 8);
	debug_str("\r\nR3 = ");
	debug_hex(*(stackPointer + STACKED_R3_OFFSET), 8);
	debug_str("\r\nR12 = ");
	debug_hex(*(stackPointer + STACKED_R12_OFFSET), 8);
	debug_str("\r\nLR = ");
	debug_hex(*(stackPointer + STACKED_LR_OFFSET), 8);
	debug_str("\r\nPC = ");
	debug_hex(*(stackPointer + STACKED_PC_OFFSET), 8);
	debug_str("\r\nPSR = ");
	debug_hex(*(stackPointer + STACKED_PSR_OFFSET), 8);


	debug_str("\r\nBFAR = ");
	debug_hex((*((volatile unsigned long *)(0xE000ED38))),8);
	debug_str("\r\nCFSR = ");
	debug_hex((*((volatile unsigned long *)(0xE000ED28))),8);
	debug_str("\r\nHFSR = ");
	debug_hex((*((volatile unsigned long *)(0xE000ED2C))),8);
	debug_str("\r\nDFSR = ");
	debug_hex((*((volatile unsigned long *)(0xE000ED30))),8);
	debug_str("\r\nAFSR = ");
	debug_hex((*((volatile unsigned long *)(0xE000ED3C))),8);

	/* Go to infinite loop when Hard Fault exception occurs */
	while (1)
	{
		NVIC_SystemReset();
	}
}

/**
 * This function handles Memory Manage exception.
 * @param  None
 * @return None
 */
void MemManage_Handler(void)
{
	/* Disable all interrupts */
	__disable_irq();
	/* Disable both DMAs */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, DISABLE);

	debug_str("\r\nMEMORY MANAGE EXCEPTION");
	/* Go to infinite loop when Memory Manage exception occurs */
	while (1)
	{
	}
}

/**
 * This function handles Bus Fault exception.
 * @param  None
 * @return None
 */
void BusFault_Handler(void)
{
	/* Disable all interrupts */
	__disable_irq();
	/* Disable both DMAs */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, DISABLE);
	debug_str("\r\nBUS FAULT EXCEPTION");
	/* Go to infinite loop when Bus Fault exception occurs */
	while (1)
	{
	}
}

/**
 * This function handles Usage Fault exception.
 * @param  None
 * @return None
 */
void UsageFault_Handler(void)
{
	/* Disable all interrupts */
	__disable_irq();
	/* Disable both DMAs */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, DISABLE);
	debug_str("\r\nUSGAGE FAULT EXCEPTION");
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1)
	{
	}
}

/**
 * This function handles SVCall exception.
 * @param  None
 * @return None
 */
void SVC_Handler(void)
{
}

/**
 * This function handles Debug Monitor exception.
 * @param  None
 * @return None
 */
void DebugMon_Handler(void)
{
}

/**
 * This function handles PendSV_Handler exception.
 * @param  None
 * @return None
 */
void PendSV_Handler(void)
{
}



/**
 * This function handles SysTick interrupt.
 * @param  None
 * @return None
 */
void SysTick_Handler(void)
{
	vsnLEDInd_toggle();
	vsnTime_uptimeIsr();
    clock_interrupt_handler(); // rename to contiki_clock_isr
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/*
 * .word  WWDG_IRQHandler
 * .word  PVD_IRQHandler
 * .word  TAMPER_IRQHandler
 * .word  RTC_IRQHandler
 * .word  FLASH_IRQHandler
 * .word  RCC_IRQHandler
 * .word  EXTI0_IRQHandler
 * .word  EXTI1_IRQHandler
 * .word  EXTI2_IRQHandler
 * .word  EXTI3_IRQHandler
 * .word  EXTI4_IRQHandler
 * .word  DMA1_Channel1_IRQHandler
 * .word  DMA1_Channel2_IRQHandler
 * .word  DMA1_Channel3_IRQHandler
 * .word  DMA1_Channel4_IRQHandler
 * .word  DMA1_Channel5_IRQHandler
 * .word  DMA1_Channel6_IRQHandler
 * .word  DMA1_Channel7_IRQHandler
 * .word  ADC1_2_IRQHandler
 * .word  USB_HP_CAN1_TX_IRQHandler
 * .word  USB_LP_CAN1_RX0_IRQHandler
 * .word  CAN1_RX1_IRQHandler
 * .word  CAN1_SCE_IRQHandler
 */

/**
 * This function handles RTC Second interrupt.
 * @param  None
 * @return None
 */
void RTC_IRQHandler(void)
{
	// RTC Second IRQ handler
	if (RTC_GetITStatus(RTC_IT_SEC) != RESET) {
		vsnSetup_measureHsiFreq();
		vsnTime_rtcSecondIsr();
		// Clear the RTC Second interrupt flag
		RTC_WaitForLastTask();
		RTC_ClearITPendingBit(RTC_IT_SEC);
	}
}

/**
 * This function handles USART1 interrupt.
 * @param  None
 * @return None
 */
void USART1_IRQHandler(void)
{
	vsnUSART_usart1Isr();

	//contiki_uart1_isr(); // it just informs process
}

/**
 * This function handles USART2 interrupt.
 * @param  None
 * @return None
 */
void USART2_IRQHandler(void)
{
	vsnUSART_usart2Isr();
}

/**
 * This function handles USART3 interrupt.
 * @param  None
 * @return None
 */
void USART3_IRQHandler(void)
{
	vsnUSART_usart3Isr();
}

/**
 * This function handles UART4 interrupt.
 * @param  None
 * @return None
 */
void UART4_IRQHandler(void)
{
	vsnUSART_uart4Isr();
}

/*
 .word  TIM1_BRK_IRQHandler
 .word  TIM1_UP_IRQHandler
 .word  TIM1_TRG_COM_IRQHandler
 .word  TIM1_CC_IRQHandler
 .word  TIM2_IRQHandler
 .word  TIM3_IRQHandler
 */

void TIM2_IRQHandler(void)
{
	//contiki_rtimer_isr();
}

/**
 * This function handles SPI1 interrupt Handler.
 * @param  None
 * @return None
 */
void SPI1_IRQHandler(void)
{
#ifdef SPI1_DRIVER_MODE_INTERRUPT
	vsnSPI1_processSpiIrq();
#endif
#ifdef SPI1_NEW_DRIVER_MODE_INTERRUPT
	vsnSPILowx_processIrq(SPI1);
#endif
}

void SPI2_IRQHandler(void)
{
#ifdef SPI2_NEW_DRIVER_MODE_INTERRUPT
	vsnSPILowx_processIrq(SPI2);
#endif
}




/**
 * This function handles DMA1 Channel2 Handler.
 * @param  None	DMA1_Channel2
 * @return None  vsnfram_processdmarxirq();
 */
void DMA1_Channel2_IRQHandler(void)
{
	//vsnFram_processDmaRxIrq();
	//vsnUSART_dmaTxUsart3Isr();

#ifdef USART3_DMA_MODE
	vsnUSART_dmaTxUsart3Isr();
#endif

#ifdef SPI1_DRIVER_MODE_DMA
	vsnSPI1_processDmaRxIrq();
#endif

#ifdef SPI1_NEW_DRIVER_MODE_DMA
	vsnSPILowx_dmaRxIrq(SPI1);
#endif

#if (defined(USART3_DMA_MODE) + defined(SPI1_DRIVER_MODE_DMA)+defined(SPI1_NEW_DRIVER_MODE_DMA)) > 1
#error "DMA1 channel 2: should be used by USART3 DMA or SPI1 DMA, bot not both!"
#endif
}

/**
 * This function handles DMA1 Channel3 Handler.
 * @param  None	DMA1_Channel3
 * @return None
 */
void DMA1_Channel3_IRQHandler(void)
{
	//vsnFram_processDmaTxIrq();
	//vsnUSART_dmaRxUsart3Isr();
#ifdef SPI1_DRIVER_MODE_DMA
	vsnSPI1_processDmaTxIrq();
#endif

#ifdef SPI1_NEW_DRIVER_MODE_DMA
	vsnSPILowx_dmaTxIrq(SPI1);
#endif

#ifdef USART3_DMA_MODE
	vsnUSART_dmaRxUsart3Isr();
#endif

#if (defined(USART3_DMA_MODE) + defined(SPI1_DRIVER_MODE_DMA) + defined(SPI1_NEW_DRIVER_MODE_DMA)) > 1
#error "DMA1 channel 3: should be used by USART3 DMA or SPI1 DMA, bot not both!"
#endif
}


/**
 * This function handles DMA1 Channel4 Handler.
 * @param  None	DMA1_Channel4
 * @return None
 */
void DMA1_Channel4_IRQHandler(void)
{
	//vsnUSART_dmaTxUsart1Isr();

#ifdef USART1_DMA_MODE
	vsnUSART_dmaTxUsart1Isr();
#endif
#ifdef I2C2_DMA_MODE
	vsnI2C_dmaTcIsr(I2C_2_DMA_TX);
#endif
#ifdef SPI2_NEW_DRIVER_MODE_DMA
	vsnSPILowx_dmaRxIrq(SPI2);
#endif

#if (defined(USART1_DMA_MODE) + defined(I2C2_DMA_MODE)+ defined(SPI2_NEW_DRIVER_MODE_DMA)) > 1
#error "DMA1 channel 4: should be used just by one periphery: USART1 DMA, SPI2 DMA or I2C2!"
#endif
}

/**
 * This function handles DMA1 Channel5 Handler.
 * @param  None	DMA1_Channel5
 * @return None
 */
void DMA1_Channel5_IRQHandler(void)
{
	//vsnUSART_dmaRxUsart1Isr();
#ifdef USART1_DMA_MODE
	vsnUSART_dmaRxUsart1Isr();
#endif
#ifdef I2C2_DMA_MODE
	vsnI2C_dmaTcIsr(I2C_2_DMA_RX);
#endif
#ifdef SPI2_NEW_DRIVER_MODE_DMA
	vsnSPILowx_dmaTxIrq(SPI2);
#endif

#if (defined(USART3_DMA_MODE) + defined(I2C2_DMA_MODE)+ defined(SPI2_NEW_DRIVER_MODE_DMA)) > 1
#error "DMA1 channel 5: should be used just by one periphery: USART3 DMA, SPI2 DMA or I2C2!"
#endif
}


/**
 * This function handles DMA1 Channel6 Handler.
 * @param  None	DMA1_Channel6
 * @return None
 */
void DMA1_Channel6_IRQHandler(void)
{
	//vsnUSART_dmaRxUsart2Isr();
#ifdef USART2_DMA_MODE
	vsnUSART_dmaRxUsart2Isr();
#elif defined (I2C1_DMA_MODE)
	vsnI2C_dmaTcIsr(I2C_1_DMA_TX);
#endif
}

/**
 * This function handles DMA1 Channel7 Handler.
 * @param  None	DMA1_Channel7
 * @return None
 */
void DMA1_Channel7_IRQHandler(void)
{
	vsnUSART_dmaTxUsart2Isr();
}

/**
 * This function handles DMA2 Channel3 Handler.
 * @param  None	DMA2_Channel3
 * @return None
 */
void DMA2_Channel3_IRQHandler(void)
{
	vsnUSART_dmaRxUart4Isr();
}

/**
 * This function handles DMA2 Channel5 Handler.
 * @param  None	DMA2_Channel4_5
 * @return None
 */
void DMA2_Channel4_5_IRQHandler(void)
{
	vsnUSART_dmaTxUart4Isr();
}

#ifdef WITH_SDIO
/**
  * This function handles SDIO global interrupt request.
  * @param  None
  * @return None
  */
void SDIO_IRQHandler(void)
{
  /* Process All SDIO Interrupt Sources */
	//vsnSD_processIrqSrc();
}
#endif /* WITH_SDIO */

/*
 .word  I2C1_EV_IRQHandler
 .word  I2C1_ER_IRQHandler
 .word  I2C2_EV_IRQHandler
 .word  I2C2_ER_IRQHandler
 .word  SPI1_IRQHandler
 .word  SPI2_IRQHandler
 */

//#if defined(CORE) || defined(ELF_APP)
/**
 * Function pointer proxy_vsnCC1101_handleRadioInterrupt has to be defined in
 * core application, and must not be defined in elf application.
 */
//void (*proxy_vsnCC1101_handleRadioInterrupt)(void) = 0;
//#endif

/**
 * This function handles External interrupt line 2.
 * @param  None
 * @return None
 */
void EXTI2_IRQHandler(void) {}



void EXTI3_IRQHandler(void) {
	#if (AT86RF2XX_BOARD_ISMTV_V1_0 || AT86RF2XX_BOARD_ISMTV_V1_1)
		if (EXTI_GetITStatus(EXTI_Line3) != RESET) {
			rf2xx_isr();
			EXTI_ClearITPendingBit(EXTI_Line3);
		}
	#endif
}


/**
 * This function handles External interrupt lines 5 thru 9.
 * @param  None
 * @return None
 */
void EXTI9_5_IRQHandler(void) {
	#if AT86RF2XX_BOARD_SNR
		if (EXTI_GetITStatus(EXTI_Line9) != RESET) {
			rf2xx_isr();
			EXTI_ClearITPendingBit(EXTI_Line9);
		}
	#endif

	/* Interrupt routine for cc1101 */
	/* Is GDO0 line activated? */
	/* line 9, for reset button */
	if(EXTI_GetITStatus(EXTI_RESETBUTTON) != RESET){
		/* reset button pressed */
		//button_sensor_interrupt_handler();
		//resetbutton_sensor_handleInterrupt();
		/* clear interrupt */
		EXTI_ClearITPendingBit(EXTI_RESETBUTTON);

	}

}


/*
 .word  USART2_IRQHandler
 .word  USART3_IRQHandler
 .word  EXTI15_10_IRQHandler
 .word  RTCAlarm_IRQHandler
 .word  USBWakeUp_IRQHandler
 .word  TIM8_BRK_IRQHandler
 .word  TIM8_UP_IRQHandler
 .word  TIM8_TRG_COM_IRQHandler
 .word  TIM8_CC_IRQHandler
 .word  ADC3_IRQHandler
 .word  FSMC_IRQHandler
 .word  SDIO_IRQHandler
 */

/**
 * This function handles TIM4 interrupts.
 * @param  None
 * @return None
 */
/* void TIM4_IRQHandler(void)
{

}*/



void TIM5_IRQHandler(void)
{
    contiki_rtimer_isr();
}

/*
 * .word  SPI3_IRQHandler
 * .word  UART4_IRQHandler
 * .word  UART5_IRQHandler
 * .word  TIM6_IRQHandler
 * .word  TIM7_IRQHandler
 * .word  DMA2_Channel1_IRQHandler
 * .word  DMA2_Channel2_IRQHandler
 * .word  DMA2_Channel3_IRQHandler
 * .word  DMA2_Channel4_5_IRQHandler
 */

/**
 * This function handles RCC interrupt request.
 * @param  None
 * @return None
 */
void RCC_IRQHandler(void) {
	if (RCC_GetITStatus(RCC_IT_HSERDY) != RESET) {
		/* Clear HSERDY interrupt pending bit */
		RCC_ClearITPendingBit(RCC_IT_HSERDY);
		/* Check if the HSE clock is still available */
		if (RCC_GetFlagStatus(RCC_FLAG_HSERDY) != RESET) {
			/* Select HSE as system clock source */
			RCC_SYSCLKConfig(RCC_SYSCLKSource_HSE);
			/* Enable PLL: once the PLL is ready the PLLRDY interrupt is generated */
			RCC_PLLCmd(ENABLE);
		}
	}
	if (RCC_GetITStatus(RCC_IT_PLLRDY) != RESET) {
		/* Clear PLLRDY interrupt pending bit */
		RCC_ClearITPendingBit(RCC_IT_PLLRDY);
		/* Check if the PLL is still locked */
		if (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) != RESET) {
			/* Select PLL as system clock source */
			RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
			/* Disable HSE Ready interrupt */
			RCC_ITConfig(RCC_IT_HSERDY, DISABLE);
			/* Disable PLL Ready interrupt */
			RCC_ITConfig(RCC_IT_PLLRDY, DISABLE);
		}
	}
}
