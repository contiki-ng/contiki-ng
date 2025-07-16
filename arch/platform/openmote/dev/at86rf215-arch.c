/*
 * Copyright (c) 2023, ComLab, Jozef Stefan Institute - https://e6.ijs.si/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
/**
 * \file
 *     AT86RF215 OpenMoteB arch specific code
 * 
 * \author
 *      Grega Morano <grega.morano@ijs.si>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "contiki-net.h"
#include "dev/spi-arch-legacy.h"
#include "dev/spi-legacy.h"
#include "dev/ioc.h"
#include "dev/gpio.h"
#include "dev/gpio-hal.h"
/*---------------------------------------------------------------------------*/
#define AT86RF215_SPI_CSN_PORT_BASE   GPIO_PORT_TO_BASE(AT86RF215_SPI_CSN_PORT)
#define AT86RF215_SPI_CSN_PIN_MASK    GPIO_PIN_MASK(AT86RF215_SPI_CSN_PIN)
#define AT86RF215_RSTN_PORT_BASE      GPIO_PORT_TO_BASE(AT86RF215_RSTN_PORT)
#define AT86RF215_RSTN_PIN_MASK       GPIO_PIN_MASK(AT86RF215_RSTN_PIN)
#define AT86RF215_PWR_PORT_BASE       GPIO_PORT_TO_BASE(AT86RF215_PWR_PORT)
#define AT86RF215_PWR_PIN_MASK        GPIO_PIN_MASK(AT86RF215_PWR_PIN)
#define AT86RF215_IRQ_PORT_BASE       GPIO_PORT_TO_BASE(AT86RF215_IRQ_PORT)
#define AT86RF215_IRQ_PIN_MASK        GPIO_PIN_MASK(AT86RF215_IRQ_PIN)
/*---------------------------------------------------------------------------*/
extern void at86rf215_isr(void);
/*---------------------------------------------------------------------------*/
static void
at86rf215_interrupt_handler(gpio_hal_pin_mask_t pin_mask)
{
    at86rf215_isr();
}
/*---------------------------------------------------------------------------*/
static gpio_hal_event_handler_t irq_handler = {
  .next = NULL,
  .handler = at86rf215_interrupt_handler,
  .pin_mask = gpio_hal_pin_to_mask(AT86RF215_IRQ_PIN) << (AT86RF215_IRQ_PORT << 3),
};
/*---------------------------------------------------------------------------*/
void 
at86rf215_arch_enable_EXTI(void)
{
    GPIO_ENABLE_INTERRUPT(AT86RF215_IRQ_PORT_BASE, AT86RF215_IRQ_PIN_MASK);
    NVIC_EnableIRQ(AT86RF215_GPIOx_VECTOR);
}
/*---------------------------------------------------------------------------*/
void
at86rf215_arch_disable_EXTI(void)
{
    GPIO_DISABLE_INTERRUPT(AT86RF215_IRQ_PORT_BASE, AT86RF215_IRQ_PIN_MASK);
}
/*---------------------------------------------------------------------------*/
void
at86rf215_arch_set_RSTN(void)
{   
    GPIO_CLR_PIN(AT86RF215_RSTN_PORT_BASE, AT86RF215_RSTN_PIN_MASK);
}
/*---------------------------------------------------------------------------*/
void
at86rf215_arch_clear_RSTN(void)
{
    GPIO_SET_PIN(AT86RF215_RSTN_PORT_BASE, AT86RF215_RSTN_PIN_MASK);
}
/*---------------------------------------------------------------------------*/
void
at86rf215_arch_spi_select(void)
{   
    GPIO_CLR_PIN(AT86RF215_SPI_CSN_PORT_BASE, AT86RF215_SPI_CSN_PIN_MASK);
}
/*---------------------------------------------------------------------------*/
void
at86rf215_arch_spi_deselect(void)
{
    GPIO_SET_PIN(AT86RF215_SPI_CSN_PORT_BASE, AT86RF215_SPI_CSN_PIN_MASK);
}
/*---------------------------------------------------------------------------*/
uint8_t 
at86rf215_arch_spi_txrx(uint8_t b)
{
    SPIX_WAITFORTxREADY(AT86RF215_SPI_INSTANCE);
    SPIX_BUF(AT86RF215_SPI_INSTANCE) = b;
    SPIX_WAITFOREOTx(AT86RF215_SPI_INSTANCE);
    SPIX_WAITFOREORx(AT86RF215_SPI_INSTANCE);
    return SPIX_BUF(AT86RF215_SPI_INSTANCE);;
}
/*---------------------------------------------------------------------------*/
void
at86rf215_arch_init(void)
{
    /* Initialize power pin */
    GPIO_SOFTWARE_CONTROL(AT86RF215_PWR_PORT_BASE , AT86RF215_PWR_PIN_MASK );
    GPIO_SET_OUTPUT(AT86RF215_PWR_PORT_BASE , AT86RF215_PWR_PIN_MASK );
    
    /* Initialize reset pin */
    GPIO_SOFTWARE_CONTROL(AT86RF215_RSTN_PORT_BASE ,AT86RF215_RSTN_PIN_MASK );
    GPIO_SET_OUTPUT(AT86RF215_RSTN_PORT_BASE ,AT86RF215_RSTN_PIN_MASK );

    /* Turn the radio off */
    GPIO_CLR_PIN(AT86RF215_PWR_PORT_BASE , AT86RF215_PWR_PIN_MASK );
    GPIO_CLR_PIN(AT86RF215_RSTN_PORT_BASE ,AT86RF215_RSTN_PIN_MASK );

    /* Power up the radio */
    GPIO_SET_PIN(AT86RF215_PWR_PORT_BASE , AT86RF215_PWR_PIN_MASK );

    /* "Un-reset" the radio */
    at86rf215_arch_clear_RSTN();

    /* Initialize SPI */
    spix_init(AT86RF215_SPI_INSTANCE);
    spix_cs_init(AT86RF215_SPI_CSN_PORT, AT86RF215_SPI_CSN_PIN);
    spix_set_mode(AT86RF215_SPI_INSTANCE, SSI_CR0_FRF_MOTOROLA, 0, 0, 8);

    /* AT86RF215 supports SPI clock up to 25MHz, but CC2538 supports max 16MHz
     * by using 32MHz CPU clock (defined in board.h) */
    spix_set_clock_freq(AT86RF215_SPI_INSTANCE, 16000000);

    /* Disable SPI interrupts - to reduce the time needed for read/write execution */
    NVIC_DisableIRQ(SSI0_IRQn);
    REG(SSI0_BASE + SSI_IM) = 0x0F;

    /* Initialize IRQs */
    GPIO_SOFTWARE_CONTROL(AT86RF215_IRQ_PORT_BASE, AT86RF215_IRQ_PIN_MASK);
    GPIO_SET_INPUT(AT86RF215_IRQ_PORT_BASE, AT86RF215_IRQ_PIN_MASK);
    ioc_set_over(AT86RF215_IRQ_PORT, AT86RF215_IRQ_PIN, IOC_OVERRIDE_PUE);

    GPIO_DETECT_EDGE(AT86RF215_IRQ_PORT_BASE, AT86RF215_IRQ_PIN_MASK);
    GPIO_TRIGGER_SINGLE_EDGE(AT86RF215_IRQ_PORT_BASE, AT86RF215_IRQ_PIN_MASK);
    GPIO_DETECT_FALLING(AT86RF215_IRQ_PORT_BASE, AT86RF215_IRQ_PIN_MASK);

    /* Set radio priority higher than rtimer - so isr can execute durring tx/rx routine */
    NVIC_SetPriority(SMT_IRQn, 1);
    NVIC_SetPriority(GPT1A_IRQn, 1);
    NVIC_SetPriority(AT86RF215_GPIOx_VECTOR, 0);

    gpio_hal_register_handler(&irq_handler);
}
