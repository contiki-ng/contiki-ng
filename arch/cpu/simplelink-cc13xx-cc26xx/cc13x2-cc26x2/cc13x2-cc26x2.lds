/*
 * Copyright (c) 2017-2018, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

MIN_STACKSIZE = 0x600;  /* 1536 bytes */
HEAPSIZE      = 0x100;  /* 256 bytes */

MEMORY
{
    FLASH (RX)      : ORIGIN = 0x00000000, LENGTH = 0x00057fa8
    /*
     * Customer Configuration Area and Bootloader Backdoor configuration in
     * flash, 40 bytes
     */
    FLASH_CCFG (RX) : ORIGIN = 0x00057fa8, LENGTH = 0x00000058
    SRAM (RWX)      : ORIGIN = 0x20000000, LENGTH = 0x00014000
    GPRAM (RWX)     : ORIGIN = 0x11000000, LENGTH = 0x00002000
}

REGION_ALIAS("REGION_TEXT",      FLASH);
REGION_ALIAS("REGION_BSS",       SRAM);
REGION_ALIAS("REGION_DATA",      SRAM);
REGION_ALIAS("REGION_STACK",     SRAM);
REGION_ALIAS("REGION_HEAP",      SRAM);
REGION_ALIAS("REGION_ARM_EXIDX", FLASH);
REGION_ALIAS("REGION_ARM_EXTAB", FLASH);

SECTIONS {

    PROVIDE (_resetVecs_base_addr = DEFINED(_resetVecs_base_addr)
                                  ? _resetVecs_base_addr
                                  : 0x0);

    .resetVecs (_resetVecs_base_addr) : AT (_resetVecs_base_addr) {
        KEEP (*(.resetVecs))
    } > REGION_TEXT

    .ramVecs (NOLOAD) : ALIGN(1024) {
        KEEP (*(.ramVecs))
    } > REGION_DATA

    /*
     * UDMACC26XX_CONFIG_BASE below must match UDMACC26XX_CONFIG_BASE defined
     * by ti/drivers/dma/UDMACC26XX.h
     * The user is allowed to change UDMACC26XX_CONFIG_BASE to move it away from
     * the default address 0x2000_1800, but remember it must be 1024 bytes aligned.
     */
    UDMACC26XX_CONFIG_BASE = 0x20001800;

    /*
     * Define absolute addresses for the DMA channels.
     * DMA channels must always be allocated at a fixed offset from the DMA base address.
     * CTEA := Control Table Entry Address
     * --------- DO NOT MODIFY -----------
     */
    DMA_SPI0_RX_CTEA   = (UDMACC26XX_CONFIG_BASE + 0x30);
    DMA_SPI0_TX_CTEA   = (UDMACC26XX_CONFIG_BASE + 0x40);
    DMA_ADC_PRI_CTEA   = (UDMACC26XX_CONFIG_BASE + 0x70);
    DMA_GPT0A_PRI_CTEA = (UDMACC26XX_CONFIG_BASE + 0x90);
    DMA_SPI1_RX_CTEA   = (UDMACC26XX_CONFIG_BASE + 0x100);
    DMA_SPI1_TX_CTEA   = (UDMACC26XX_CONFIG_BASE + 0x110);

    DMA_SPI0_RX_ALT_CTEA = (UDMACC26XX_CONFIG_BASE + 0x230);
    DMA_SPI0_TX_ALT_CTEA = (UDMACC26XX_CONFIG_BASE + 0x240);
    DMA_ADC_ALT_CTEA     = (UDMACC26XX_CONFIG_BASE + 0x270);
    DMA_GPT0A_ALT_CTEA   = (UDMACC26XX_CONFIG_BASE + 0x290);
    DMA_SPI1_RX_ALT_CTEA = (UDMACC26XX_CONFIG_BASE + 0x300);
    DMA_SPI1_TX_ALT_CTEA = (UDMACC26XX_CONFIG_BASE + 0x310);

    /*
     * Allocate SPI0, SPI1, ADC, and GPTimer0 DMA descriptors at absolute addresses.
     * --------- DO NOT MODIFY -----------
     */
    UDMACC26XX_dmaSpi0RxControlTableEntry_is_placed = 0;
    .dmaSpi0RxControlTableEntry DMA_SPI0_RX_CTEA (NOLOAD) : AT (DMA_SPI0_RX_CTEA) {
        *(.dmaSpi0RxControlTableEntry)
    } > REGION_DATA

    UDMACC26XX_dmaSpi0TxControlTableEntry_is_placed = 0;
    .dmaSpi0TxControlTableEntry DMA_SPI0_TX_CTEA (NOLOAD) : AT (DMA_SPI0_TX_CTEA) {
        *(.dmaSpi0TxControlTableEntry)
    } > REGION_DATA

    UDMACC26XX_dmaADCPriControlTableEntry_is_placed = 0;
    .dmaADCPriControlTableEntry DMA_ADC_PRI_CTEA (NOLOAD) : AT (DMA_ADC_PRI_CTEA) {
        *(.dmaADCPriControlTableEntry)
    } > REGION_DATA

    UDMACC26XX_dmaGPT0APriControlTableEntry_is_placed = 0;
    .dmaGPT0APriControlTableEntry DMA_GPT0A_PRI_CTEA (NOLOAD) : AT (DMA_GPT0A_PRI_CTEA) {
        *(.dmaGPT0APriControlTableEntry)
    } > REGION_DATA

    UDMACC26XX_dmaSpi1RxControlTableEntry_is_placed = 0;
    .dmaSpi1RxControlTableEntry DMA_SPI1_RX_CTEA (NOLOAD) : AT (DMA_SPI1_RX_CTEA) {
        *(.dmaSpi1RxControlTableEntry)
    } > REGION_DATA

    UDMACC26XX_dmaSpi1TxControlTableEntry_is_placed = 0;
    .dmaSpi1TxControlTableEntry DMA_SPI1_TX_CTEA (NOLOAD) : AT (DMA_SPI1_TX_CTEA) {
        *(.dmaSpi1TxControlTableEntry)
    } > REGION_DATA

    UDMACC26XX_dmaSpi0RxAltControlTableEntry_is_placed = 0;
    .dmaSpi0RxAltControlTableEntry DMA_SPI0_RX_ALT_CTEA (NOLOAD) : AT (DMA_SPI0_RX_ALT_CTEA) {
        *(.dmaSpi0RxAltControlTableEntry)
    } > REGION_DATA

    UDMACC26XX_dmaSpi0TxAltControlTableEntry_is_placed = 0;
    .dmaSpi0TxAltControlTableEntry DMA_SPI0_TX_ALT_CTEA (NOLOAD) : AT (DMA_SPI0_TX_ALT_CTEA) {
        *(.dmaSpi0TxAltControlTableEntry)
    } > REGION_DATA

    UDMACC26XX_dmaADCAltControlTableEntry_is_placed = 0;
    .dmaADCAltControlTableEntry DMA_ADC_ALT_CTEA (NOLOAD) : AT (DMA_ADC_ALT_CTEA) {
        *(.dmaADCAltControlTableEntry)
    } > REGION_DATA

    UDMACC26XX_dmaGPT0AAltControlTableEntry_is_placed = 0;
    .dmaGPT0AAltControlTableEntry DMA_GPT0A_ALT_CTEA (NOLOAD) : AT (DMA_GPT0A_ALT_CTEA) {
        *(.dmaGPT0AAltControlTableEntry)
    } > REGION_DATA

    UDMACC26XX_dmaSpi1RxAltControlTableEntry_is_placed = 0;
    .dmaSpi1RxAltControlTableEntry DMA_SPI1_RX_ALT_CTEA (NOLOAD) : AT (DMA_SPI1_RX_ALT_CTEA) {
        *(.dmaSpi1RxAltControlTableEntry)
    } > REGION_DATA

    UDMACC26XX_dmaSpi1TxAltControlTableEntry_is_placed = 0;
    .dmaSpi1TxAltControlTableEntry DMA_SPI1_TX_ALT_CTEA (NOLOAD) : AT (DMA_SPI1_TX_ALT_CTEA) {
        *(.dmaSpi1TxAltControlTableEntry)
    } > REGION_DATA

    .text : {
        CREATE_OBJECT_SYMBOLS
        *(.text)
        *(.text.*)
        . = ALIGN(0x4);
        KEEP (*(.ctors))
        . = ALIGN(0x4);
        KEEP (*(.dtors))
        . = ALIGN(0x4);
        __init_array_start = .;
        KEEP (*(.init_array*))
        __init_array_end = .;
        *(.init)
        *(.fini*)
    } > REGION_TEXT AT> REGION_TEXT

    PROVIDE (__etext = .);
    PROVIDE (_etext = .);
    PROVIDE (etext = .);

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    } > REGION_TEXT AT> REGION_TEXT

    .data : ALIGN(0x4) {
        __data_load__ = LOADADDR(.data);
        __data_start__ = .;
        *(.data)
        *(.data.*)
        . = ALIGN (0x4);
        __data_end__ = .;
    } > REGION_DATA AT> REGION_TEXT

    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > REGION_ARM_EXIDX AT> REGION_ARM_EXIDX

    .ARM.extab : {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > REGION_ARM_EXTAB AT> REGION_ARM_EXTAB

    .nvs (NOLOAD) : ALIGN(0x2000) {
        *(.nvs)
    } > REGION_TEXT

    .ccfg : {
        KEEP(*(.ccfg))
    } > FLASH_CCFG AT> FLASH_CCFG

    .bss : {
        __bss_start__ = .;
        *(.shbss)
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(0x4);
        __bss_end__ = .;
    } > REGION_BSS AT> REGION_BSS

    /* Start of heap must be 4 byte aligned */
    .heap (NOLOAD) : ALIGN(0x4) {
        PROVIDE(__heap_start__ = .);
        PROVIDE(_heap          = __heap_start__);
        PROVIDE(end            = __heap_start__);
        PROVIDE(_end           = __heap_start__);
        PROVIDE(__end          = __heap_start__);

        . += HEAPSIZE;
        KEEP(*(.heap))

        PROVIDE(__heap_end__ = .);
        PROVIDE(_eheap       = __heap_end__);
        PROVIDE(__HeapLimit  = __heap_end__);
    } > REGION_HEAP AT> REGION_HEAP

    PROVIDE(__stack_size = ORIGIN(SRAM) + LENGTH(SRAM) - ALIGN(0x8));

    /* Start of stack must be 8 byte aligned */
    .stack (NOLOAD) : {
        PROVIDE(_stack        = ALIGN(0x8));
        PROVIDE(__stack       = _stack);

        PROVIDE(_stack_end    = ORIGIN(SRAM) + LENGTH(SRAM));
        PROVIDE(__stack_end   = _stack_end);
        PROVIDE(_stack_origin = _stack_end);

        /* Note that the stack check library requires the symbols */
        /* "_stack" and "_stack_origin" to be defined. */

        /* Assert that we have enough stack */
        ASSERT(__stack_size >= MIN_STACKSIZE, "Error: No room left for the stack");
    } > REGION_STACK AT> REGION_STACK
}
