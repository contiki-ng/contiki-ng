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
/*
 *  ======== CC26X2R1_LAUNCHXL_NoRTOS.lds ========
 *  Default Linker script for the Texas Instruments CC1352
 */

STACKSIZE = 1024;
HEAPSIZE = 256; /* Size of heap buffer used by HeapMem */

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

REGION_ALIAS("REGION_TEXT", FLASH);
REGION_ALIAS("REGION_BSS", SRAM);
REGION_ALIAS("REGION_DATA", SRAM);
REGION_ALIAS("REGION_STACK", SRAM);
REGION_ALIAS("REGION_HEAP", SRAM);
REGION_ALIAS("REGION_ARM_EXIDX", FLASH);
REGION_ALIAS("REGION_ARM_EXTAB", FLASH);

SECTIONS {

    PROVIDE (_resetVecs_base_address =
        DEFINED(_resetVecs_base_address) ? _resetVecs_base_address : 0x0);

    .resetVecs (_resetVecs_base_address) : AT (_resetVecs_base_address) {
        KEEP (*(.resetVecs))
    } > REGION_TEXT

    .ramVecs (NOLOAD) : ALIGN(1024){
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
     * --------- DO NOT MODIFY -----------
     */
    DMA_SPI0_RX_CONTROL_TABLE_ENTRY_ADDRESS   = (UDMACC26XX_CONFIG_BASE + 0x30);
    DMA_SPI0_TX_CONTROL_TABLE_ENTRY_ADDRESS   = (UDMACC26XX_CONFIG_BASE + 0x40);
    DMA_ADC_PRI_CONTROL_TABLE_ENTRY_ADDRESS   = (UDMACC26XX_CONFIG_BASE + 0x70);
    DMA_GPT0A_PRI_CONTROL_TABLE_ENTRY_ADDRESS = (UDMACC26XX_CONFIG_BASE + 0x90);
    DMA_SPI1_RX_CONTROL_TABLE_ENTRY_ADDRESS   = (UDMACC26XX_CONFIG_BASE + 0x100);
    DMA_SPI1_TX_CONTROL_TABLE_ENTRY_ADDRESS   = (UDMACC26XX_CONFIG_BASE + 0x110);
    DMA_ADC_ALT_CONTROL_TABLE_ENTRY_ADDRESS   = (UDMACC26XX_CONFIG_BASE + 0x270);
    DMA_GPT0A_ALT_CONTROL_TABLE_ENTRY_ADDRESS = (UDMACC26XX_CONFIG_BASE + 0x290);

    /*
     * Allocate SPI0, SPI1, ADC, and GPTimer0 DMA descriptors at absolute addresses.
     * --------- DO NOT MODIFY -----------
     */
    UDMACC26XX_dmaSpi0RxControlTableEntry_is_placed = 0;
    .dmaSpi0RxControlTableEntry DMA_SPI0_RX_CONTROL_TABLE_ENTRY_ADDRESS : AT (DMA_SPI0_RX_CONTROL_TABLE_ENTRY_ADDRESS) {*(.dmaSpi0RxControlTableEntry)} > REGION_DATA

    UDMACC26XX_dmaSpi0TxControlTableEntry_is_placed = 0;
    .dmaSpi0TxControlTableEntry DMA_SPI0_TX_CONTROL_TABLE_ENTRY_ADDRESS : AT (DMA_SPI0_TX_CONTROL_TABLE_ENTRY_ADDRESS) {*(.dmaSpi0TxControlTableEntry)} > REGION_DATA

    UDMACC26XX_dmaADCPriControlTableEntry_is_placed = 0;
    .dmaADCPriControlTableEntry DMA_ADC_PRI_CONTROL_TABLE_ENTRY_ADDRESS : AT (DMA_ADC_PRI_CONTROL_TABLE_ENTRY_ADDRESS) {*(.dmaADCPriControlTableEntry)} > REGION_DATA

    UDMACC26XX_dmaGPT0APriControlTableEntry_is_placed = 0;
    .dmaGPT0APriControlTableEntry DMA_GPT0A_PRI_CONTROL_TABLE_ENTRY_ADDRESS : AT (DMA_GPT0A_PRI_CONTROL_TABLE_ENTRY_ADDRESS) {*(.dmaGPT0APriControlTableEntry)} > REGION_DATA

    UDMACC26XX_dmaSpi1RxControlTableEntry_is_placed = 0;
    .dmaSpi1RxControlTableEntry DMA_SPI1_RX_CONTROL_TABLE_ENTRY_ADDRESS : AT (DMA_SPI1_RX_CONTROL_TABLE_ENTRY_ADDRESS) {*(.dmaSpi1RxControlTableEntry)} > REGION_DATA

    UDMACC26XX_dmaSpi1TxControlTableEntry_is_placed = 0;
    .dmaSpi1TxControlTableEntry DMA_SPI1_TX_CONTROL_TABLE_ENTRY_ADDRESS : AT (DMA_SPI1_TX_CONTROL_TABLE_ENTRY_ADDRESS) {*(.dmaSpi1TxControlTableEntry)} > REGION_DATA

    UDMACC26XX_dmaADCAltControlTableEntry_is_placed = 0;
    .dmaADCAltControlTableEntry DMA_ADC_ALT_CONTROL_TABLE_ENTRY_ADDRESS : AT (DMA_ADC_ALT_CONTROL_TABLE_ENTRY_ADDRESS) {*(.dmaADCAltControlTableEntry)} > REGION_DATA

    UDMACC26XX_dmaGPT0AAltControlTableEntry_is_placed = 0;
    .dmaGPT0AAltControlTableEntry DMA_GPT0A_ALT_CONTROL_TABLE_ENTRY_ADDRESS : AT (DMA_GPT0A_ALT_CONTROL_TABLE_ENTRY_ADDRESS) {*(.dmaGPT0AAltControlTableEntry)} > REGION_DATA

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

    .data : ALIGN(4) {
        __data_load__ = LOADADDR (.data);
        __data_start__ = .;
        *(.data)
        *(.data.*)
        . = ALIGN (4);
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
        KEEP (*(.ccfg))
    } > FLASH_CCFG AT> FLASH_CCFG

    .bss : {
        __bss_start__ = .;
        *(.shbss)
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN (4);
        __bss_end__ = .;
    } > REGION_BSS AT> REGION_BSS

    .heap : {
        __heap_start__ = .;
        end = __heap_start__;
        _end = end;
        __end = end;
        . = . + HEAPSIZE;
        KEEP(*(.heap))
        __heap_end__ = .;
        __HeapLimit = __heap_end__;
    } > REGION_HEAP AT> REGION_HEAP

    .stack (NOLOAD) : ALIGN(0x8) {
        _stack = .;
        __stack = .;
        KEEP(*(.stack))
        . += STACKSIZE;
        _stack_end = .;
        __stack_end = .;
    } > REGION_STACK AT> REGION_STACK
}
