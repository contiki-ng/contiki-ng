/**************************************************************************//**
 * @file     retarget.c
 * @version  V3.00
 * @brief    Debug Port and Semihost Setting Source File
 *
 * @note
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2016-2020 Nuvoton Technology Corp. All rights reserved.
 *
 ******************************************************************************/


#include <stdio.h>
#include "NuMicro.h"

#if defined (__ICCARM__)
# pragma diag_suppress=Pm150
#endif


#if defined ( __CC_ARM   )
#if (__ARMCC_VERSION < 400000)
#else
/* Insist on keeping widthprec, to avoid X propagation by benign code in C-lib */
#pragma import _printf_widthprec
#endif
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#if !(defined(__ICCARM__) && (__VER__ >= 6010000))
# if (__ARMCC_VERSION < 6040000)
struct __FILE
{
    int handle; /* Add whatever you need here */
};
# endif
#elif(__VER__ >= 8000000)
struct __FILE
{
    int handle; /* Add whatever you need here */
};
#endif
FILE __stdout;
FILE __stdin;


#if (defined(__ARMCC_VERSION) || defined(__ICCARM__))
extern int32_t SH_DoCommand(int32_t n32In_R0, int32_t n32In_R1, int32_t *pn32Out_R0);

#if defined( __ICCARM__ )
__WEAK
#else
__attribute__((weak))
#endif
uint32_t ProcessHardFault(uint32_t lr, uint32_t msp, uint32_t psp);

#endif 

#pragma pack(push)
#pragma pack(1)
typedef struct {
    char *name;
    uint32_t u32Addr;
    uint8_t u8NSIdx;
} IP_T;
#pragma pack(pop)

#ifndef DEBUG_ENABLE_SEMIHOST
static IP_T s_IpTbl[] = {
{"SYS",SYS_BASE,0},
{"CLK",CLK_BASE,0},
{"INT",INT_BASE,0},
{"GPIOA",GPIOA_BASE,224+0},
{"GPIOB",GPIOB_BASE,224+1},
{"GPIOC",GPIOC_BASE,224+2},
{"GPIOD",GPIOD_BASE,224+3},
{"GPIOE",GPIOE_BASE,224+4},
{"GPIOF",GPIOF_BASE,224+5},
{"GPIOG",GPIOG_BASE,224+6},
{"GPIOH",GPIOH_BASE,224+7},
{"GPIO_DBCTL",GPIO_DBCTL_BASE,0},
{"PA",GPIO_PIN_DATA_BASE       ,224+0},
{"PB",GPIO_PIN_DATA_BASE+16*4  ,224+0},
{"PC",GPIO_PIN_DATA_BASE+2*16*4,224+0},
{"PD",GPIO_PIN_DATA_BASE+3*16*4,224+0},
{"PE",GPIO_PIN_DATA_BASE+4*16*4,224+0},
{"PF",GPIO_PIN_DATA_BASE+5*16*4,224+0},
{"PG",GPIO_PIN_DATA_BASE+6*16*4,224+0},
{"PH",GPIO_PIN_DATA_BASE+7*16*4,224+0},
{"PDMA0",PDMA0_BASE,0},
{"PDMA1",PDMA1_BASE,PDMA1_Attr},
{"USBH",USBH_BASE,USBH_Attr},
{"FMC",FMC_BASE,0},
{"SDH0",SDH0_BASE,SDH0_Attr},
{"EBI",EBI_BASE,EBI_Attr},
{"SCU",SCU_BASE,0},
{"CRC",CRC_BASE,CRC_Attr},
{"CRPT",CRPT_BASE,CRPT_Attr},
{"WDT",WDT_BASE,0},
{"WWDT",WWDT_BASE,0},
{"RTC",RTC_BASE,RTC_Attr},
{"EADC",EADC_BASE,EADC_Attr},
{"ACMP01",ACMP01_BASE,ACMP01_Attr},
{"DAC0",DAC0_BASE,DAC_Attr},
{"DAC1",DAC1_BASE,DAC_Attr},
{"I2S0",I2S0_BASE,I2S0_Attr},
{"OTG",OTG_BASE,OTG_Attr},
{"TMR01",TMR01_BASE,0},
{"TMR23",TMR23_BASE,TMR23_Attr},
{"EPWM0",EPWM0_BASE,EPWM0_Attr},
{"EPWM1",EPWM1_BASE,EPWM1_Attr},
{"BPWM0",BPWM0_BASE,BPWM0_Attr},
{"BPWM1",BPWM1_BASE,BPWM1_Attr},
{"QSPI0",QSPI0_BASE,QSPI0_Attr},
{"SPI0",SPI0_BASE,SPI0_Attr},
{"SPI1",SPI1_BASE,SPI1_Attr},
{"SPI2",SPI2_BASE,SPI2_Attr},
{"SPI3",SPI3_BASE,SPI3_Attr},
{"UART0",UART0_BASE,UART0_Attr},
{"UART1",UART1_BASE,UART1_Attr},
{"UART2",UART2_BASE,UART2_Attr},
{"UART3",UART3_BASE,UART3_Attr},
{"UART4",UART4_BASE,UART4_Attr},
{"UART5",UART5_BASE,UART5_Attr},
{"I2C0",I2C0_BASE,I2C0_Attr},
{"I2C1",I2C1_BASE,I2C1_Attr},
{"I2C2",I2C2_BASE,I2C2_Attr},
{"SC0",SC0_BASE,SC0_Attr},
{"SC1",SC1_BASE,SC1_Attr},
{"SC2",SC2_BASE,SC2_Attr},
{"CAN0",CAN0_BASE,CAN0_Attr},
{"QEI0",QEI0_BASE,QEI0_Attr},
{"QEI1",QEI1_BASE,QEI1_Attr},
{"ECAP0",ECAP0_BASE,ECAP0_Attr},
{"ECAP1",ECAP1_BASE,ECAP1_Attr},
{"TRNG",TRNG_BASE,TRNG_Attr},
{"USBD",USBD_BASE,USBD_Attr},
{"USCI0",USCI0_BASE, USCI0_Attr},
{"USCI1",USCI1_BASE, USCI1_Attr},
{0,USCI1_BASE+4096, 0},
};
#endif

int kbhit(void);
int IsDebugFifoEmpty(void);
void _ttywrch(int ch);
int fputc(int ch, FILE *stream);

#if (defined(__ARMCC_VERSION) || defined(__ICCARM__))
int fgetc(FILE *stream);
int ferror(FILE *stream);
#endif

char GetChar(void);
void SendChar_ToUART(int ch);
void SendChar(int ch);
int32_t SH_Return(int32_t n32In_R0, int32_t n32In_R1, int32_t *pn32Out_R0);
void _sys_exit(int return_code)__attribute__((noreturn));

#if (defined(DEBUG_ENABLE_SEMIHOST) || defined(OS_USE_SEMIHOSTING))
#if (defined(__ARMCC_VERSION) || defined(__ICCARM__) || defined(__GNUC__))
/* The static buffer is used to speed up the semihost */
static char g_buf[16];
static char g_buf_len = 0;
static volatile int32_t g_ICE_Conneced = 1;

/**
 * @brief    This function is called by Hardfault handler.
 * @param    None
 * @returns  None
 * @details  This function is called by Hardfault handler and check if it is caused by __BKPT or not.
 *
 */

uint32_t ProcessHardFault(uint32_t lr, uint32_t msp, uint32_t psp)
{
    uint32_t *sp = 0;
    uint32_t inst;

    /* Check the used stack */
    if(lr & 0x40)
    {
        /* Secure stack used */
        if(lr & 4)
            sp = (uint32_t *)psp;
        else
            sp = (uint32_t *)msp;
    
    }
#if defined (__ARM_FEATURE_CMSE) &&  (__ARM_FEATURE_CMSE == 3U)    
    else
    {
        /* Non-secure stack used */
        if(lr & 4)
            sp = (uint32_t *)__TZ_get_PSP_NS();
        else
            sp = (uint32_t *)__TZ_get_MSP_NS();
    
    }
#endif    
        
    if (sp != 0)
    {
        /* Get the instruction caused the hardfault */
        inst = M16(sp[6]);


        if(inst == 0xBEAB)
        {
            /*
                If the instruction is 0xBEAB, it means it is caused by BKPT without ICE connected.
                We still return for output/input message to UART.
            */
            g_ICE_Conneced = 0; // Set a flag for ICE offline
            sp[6] += 2; // return to next instruction
            return lr;  // Keep lr in R0
        }
    }
    
    /* It is casued by hardfault (Not semihost). Just process the hard fault here. */
    /* TODO: Implement your hardfault handle code here */
    
    /*
    printf("  HardFault!\n\n");
    printf("r0  = 0x%x\n", sp[0]);
    printf("r1  = 0x%x\n", sp[1]);
    printf("r2  = 0x%x\n", sp[2]);
    printf("r3  = 0x%x\n", sp[3]);
    printf("r12 = 0x%x\n", sp[4]);
    printf("lr  = 0x%x\n", sp[5]);
    printf("pc  = 0x%x\n", sp[6]);
    printf("psr = 0x%x\n", sp[7]);
    */
    
    while(1){}
    
}



/**
 *
 * @brief      The function to process semihosted command
 * @param[in]  n32In_R0  : semihost register 0
 * @param[in]  n32In_R1  : semihost register 1
 * @param[out] pn32Out_R0: semihost register 0
 * @retval     0: No ICE debug
 * @retval     1: ICE debug
 *
 */

int32_t SH_Return(int32_t n32In_R0, int32_t n32In_R1, int32_t *pn32Out_R0)
{
    (void)n32In_R1;
    if(g_ICE_Conneced)
    {
        if(pn32Out_R0)
            *pn32Out_R0 = n32In_R0;
        
        return 1;
    }
    return 0;
}



#endif
#else // defined(DEBUG_ENABLE_SEMIHOST)

int32_t SH_Return(int32_t n32In_R0, int32_t n32In_R1, int32_t *pn32Out_R0);

#if defined( __ICCARM__ )
__WEAK
#else
__attribute__((weak)) 
#endif
uint32_t ProcessHardFault(uint32_t lr, uint32_t msp, uint32_t psp)
{
    extern void SCU_IRQHandler(void);
    uint32_t *sp = 0ul;
    uint32_t i;
    uint32_t inst, addr,taddr = 0ul,tdata;
    int32_t secure;
    uint32_t rm,rn,rt, imm5, imm8;
    int32_t eFlag;
    uint8_t idx, bit;
    int32_t s;

    /* Check the used stack */
    secure = (lr & 0x40ul)?1ul:0ul;
    if(secure)
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
            sp = (uint32_t *)(uint32_t)__TZ_get_PSP_NS();
        else
            sp = (uint32_t *)(uint32_t)__TZ_get_MSP_NS();
    
    }
#endif    
    
    /*
        r0  = sp[0]
        r1  = sp[1]
        r2  = sp[2]
        r3  = sp[3]
        r12 = sp[4]
        lr  = sp[5]
        pc  = sp[6]
        psr = sp[7]
    */

    printf("!!---------------------------------------------------------------!!\n");
    printf("                       <<< HardFault >>>\n");
    /* Get the instruction caused the hardfault */
    if (sp != 0)
    {
        addr = sp[6];
        inst = M16(addr);
    }
    eFlag = 0;
    if((!secure) && ((addr & NS_OFFSET) == 0) )
    {
        printf("  Non-secure CPU try to fetch secure code in 0x%x\n", addr);
        printf("  Try to check NSC region or SAU settings.\n");

        eFlag = 1;
    }else if(inst == 0xBEAB)
    {
        printf("  Execute BKPT without ICE connected\n");
        eFlag = 2;
    }    
    else if((inst >> 12) == 5)
    {
        eFlag = 3;
        /* 0101xx Load/store (register offset) on page C2-327 of armv8m ref */
        rm = (inst >> 6) & 0x7;
        rn = (inst >> 3) & 0x7;
        rt = inst & 0x7;
        
        taddr = sp[rn] + sp[rm];
        tdata = sp[rt];
        if(rn == rt)
        {
            printf("  [0x%08x] 0x%04x %s R%d [0x%x]\n",addr, inst, 
            (inst&BIT11)?"LDR":"STR",rt, taddr);
        }
        else
        {
            printf("  [0x%08x] 0x%04x %s 0x%x [0x%x]\n",addr, inst, 
            (inst&BIT11)?"LDR":"STR",tdata, taddr);
        }
        
    }
    else if((inst >> 13) == 3)
    {
        eFlag = 3;
        /* 011xxx	 Load/store word/byte (immediate offset) on page C2-327 of armv8m ref */
        imm5 = (inst >> 6) & 0x1f;
        rn = (inst >> 3) & 0x7;
        rt = inst & 0x7;
        
        taddr = sp[rn] + imm5;
        tdata = sp[rt];
        if(rt == rn)
        {
            printf("  [0x%08x] 0x%04x %s R%d [0x%x]\n",addr, inst, 
            (inst&BIT11)?"LDR":"STR",rt, taddr);
        }
        else
        {
            printf("  [0x%08x] 0x%04x %s 0x%x [0x%x]\n",addr, inst, 
            (inst&BIT11)?"LDR":"STR",tdata, taddr);
        }
    }
    else if((inst >> 12) == 8)
    {
        eFlag = 3;
        /* 1000xx	 Load/store halfword (immediate offset) on page C2-328 */
        imm5 = (inst >> 6) & 0x1f;
        rn = (inst >> 3) & 0x7;
        rt = inst & 0x7;
        
        taddr = sp[rn] + imm5;
        tdata = sp[rt];
        if(rt == rn)
        {
            printf("  [0x%08x] 0x%04x %s R%d [0x%x]\n",addr, inst, 
            (inst&BIT11)?"LDR":"STR",rt, taddr);
        }
        else
        {
            printf("  [0x%08x] 0x%04x %s 0x%x [0x%x]\n",addr, inst, 
            (inst&BIT11)?"LDR":"STR",tdata, taddr);
        }
        
    }
    else if((inst >> 12) == 9)
    {
        eFlag = 3;
        /* 1001xx	 Load/store (SP-relative) on page C2-328 */
        imm8 = inst & 0xff;
        rt = (inst >> 8) & 0x7;
        
        taddr = sp[6] + imm8;
        tdata = sp[rt];
        printf("  [0x%08x] 0x%04x %s 0x%x [0x%x]\n",addr, inst, 
        (inst&BIT11)?"LDR":"STR",tdata, taddr);
    }
    else
    {
        eFlag = 4;
        printf("  Unexpected instruction: 0x%04x \n", inst);
    }
    
    if(eFlag == 3)
    {
        /* It is LDR/STR hardfault */
        if(!secure) 
        {
            /* It is happened in Nonsecure code */
            
            for(i=0;i< (uint32_t)(sizeof(s_IpTbl)/sizeof(IP_T)-1);i++)
            {
                /* Case 1: Nonsecure code try to access secure IP. It also causes SCU violation */
                if((taddr >= s_IpTbl[i].u32Addr) && (taddr < (s_IpTbl[i+1].u32Addr)))
                {
                    idx = s_IpTbl[i].u8NSIdx;
                    bit = idx & 0x1f;
                    idx = idx >> 5;
                    s = (SCU->PNSSET[idx] >> bit) & 1ul;
                    printf("  Illegal access to %s %s in Nonsecure code.\n",(s)?"Nonsecure":"Secure", s_IpTbl[i].name);
                    break;
                }
                
                /* Case 2: Nonsecure code try to access Nonsecure IP but the IP is secure IP */
                if((taddr >= (s_IpTbl[i].u32Addr+NS_OFFSET)) && (taddr < (s_IpTbl[i+1].u32Addr+NS_OFFSET)))
                {
                    idx = s_IpTbl[i].u8NSIdx;
                    bit = idx & 0x1f;
                    idx = idx >> 5;
                    s = (SCU->PNSSET[idx] >> bit) & 1ul;
                    printf("  Illegal access to %s %s in Nonsecure code.\nIt may be set as secure IP here.\n",(s)?"Nonsecure":"Secure", s_IpTbl[i].name);
                    break;
                }
            }
        }
        else
        {
            /* It is happened in secure code */
            
            
            if(taddr > NS_OFFSET)
            {
                /* Case 3: Secure try to access secure IP through Nonsecure address. It also causes SCU violation */
                for(i=0;i< (uint32_t)(sizeof(s_IpTbl)/sizeof(IP_T)-1);i++)
                {
                    if((taddr >= (s_IpTbl[i].u32Addr+NS_OFFSET)) && (taddr < (s_IpTbl[i+1].u32Addr+NS_OFFSET)))
                    {
                        idx = s_IpTbl[i].u8NSIdx;
                        bit = idx & 0x1f;
                        idx = idx >> 5;
                        s = (SCU->PNSSET[idx] >> bit) & 1ul;
                        printf("  Illegal to use Nonsecure address to access %s %s in Secure code\n",(s)?"Nonsecure":"Secure", s_IpTbl[i].name);
                        break;
                    }
                }
            }
        
        
        }
    }
    
    SCU_IRQHandler();
    
    printf("!!---------------------------------------------------------------!!\n");
    
    /* Or *sp to remove compiler warning */
    while(1U|*sp){}
    
    return lr;
}


int32_t SH_Return(int32_t n32In_R0, int32_t n32In_R1, int32_t *pn32Out_R0)
{
    (void)n32In_R0;
    (void)n32In_R1;
    (void)pn32Out_R0;
    return 0;
}

#endif /* defined(DEBUG_ENABLE_SEMIHOST) */


/**
 * @brief    Routine to send a char
 *
 * @param[in] ch  A character data writes to debug port
 *
 * @returns  Send value from UART debug port
 *
 * @details  Send a target char to UART debug port .
 */
#ifndef NONBLOCK_PRINTF
void SendChar_ToUART(int ch)
{

    while(DEBUG_PORT->FIFOSTS & UART_FIFOSTS_TXFULL_Msk){}
    if((char)ch == '\n')
    {
        DEBUG_PORT->DAT = '\r';
        while(DEBUG_PORT->FIFOSTS & UART_FIFOSTS_TXFULL_Msk){}
    }
    DEBUG_PORT->DAT = (uint32_t)ch;
}

#else
/* Non-block implement of send char */
# define BUF_SIZE    512
void SendChar_ToUART(int ch)
{
    static uint8_t u8Buf[BUF_SIZE] = {0};
    static int32_t i32Head = 0;
    static int32_t i32Tail = 0;
    int32_t i32Tmp;
    
    /* Only flush the data in buffer to UART when ch == 0 */
    if(ch)
    {
        // Push char
        if(ch == '\n')
        {
            i32Tmp = i32Head+1;
            if(i32Tmp > BUF_SIZE) i32Tmp = 0;
            if(i32Tmp != i32Tail)
            {
                u8Buf[i32Head] = '\r';
                i32Head = i32Tmp;
            }
        }

        i32Tmp = i32Head+1;
        if(i32Tmp > BUF_SIZE) i32Tmp = 0;
        if(i32Tmp != i32Tail)
        {
            u8Buf[i32Head] = ch;
            i32Head = i32Tmp;
        }
    }
    else
    {
        if(i32Tail == i32Head)
            return;
    }

    // pop char
    do
    {
        i32Tmp = i32Tail + 1;
        if(i32Tmp > BUF_SIZE) i32Tmp = 0;

        if((DEBUG_PORT->FIFOSTS & UART_FIFOSTS_TXFULL_Msk) == 0)
        {
            DEBUG_PORT->DAT = u8Buf[i32Tail];
            i32Tail = i32Tmp;
        }
        else
            break; // FIFO full
    }while(i32Tail != i32Head);
}
#endif

/**
 * @brief    Routine to send a char
 *
 * @param[in] ch A character data writes to debug port
 *
 * @returns  Send value from UART debug port or semihost
 *
 * @details  Send a target char to UART debug port or semihost.
 */
void SendChar(int ch)
{
#if defined(DEBUG_ENABLE_SEMIHOST)

    g_buf[(uint8_t)g_buf_len++] = (char)ch;
    g_buf[(uint8_t)g_buf_len] = '\0';
    if(g_buf_len + 1 >= sizeof(g_buf) || ch == '\n' || ch == '\0')
    {
        /* Send the char */
        if(g_ICE_Conneced)
        {
            
            if(SH_DoCommand(0x04, (int)g_buf, NULL) != 0)
            {
                g_buf_len = 0;
                
                return;
            }
        }
        else
        {
# if (DEBUG_ENABLE_SEMIHOST == 2) // Re-direct to UART Debug Port only when DEBUG_ENABLE_SEMIHOST=2           
            int i;

            for(i = 0; i < g_buf_len; i++)
                SendChar_ToUART(g_buf[i]);
            g_buf_len = 0;
# endif            
        }
    }
#else
    SendChar_ToUART(ch);
#endif
}

/**
 * @brief    Routine to get a char
 *
 * @param    None
 *
 * @returns  Get value from UART debug port or semihost
 *
 * @details  Wait UART debug port or semihost to input a char.
 */
char GetChar(void)
{
#ifdef DEBUG_ENABLE_SEMIHOST
# if defined (__ICCARM__)
    int nRet;
    while(SH_DoCommand(0x7, 0, &nRet) != 0)
    {
        if(nRet != 0)
            return (char)nRet;
    }
# else
    int nRet;
    while(SH_DoCommand(0x101, 0, &nRet) != 0)
    {
        if(nRet != 0)
        {
            SH_DoCommand(0x07, 0, &nRet);
            return (char)nRet;
        }
    }

    
# if (DEBUG_ENABLE_SEMIHOST == 2) // Re-direct to UART Debug Port only when DEBUG_ENABLE_SEMIHOST=2
    
    /* Use debug port when ICE is not connected at semihost mode */
    while(!g_ICE_Conneced)
    {
        if((DEBUG_PORT->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk) == 0)
        {
            return (DEBUG_PORT->DAT);
        }
    }
# endif    
    
# endif
    return (0);
#else

    while(1)
    {
        if((DEBUG_PORT->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk) == 0U)
        {
            return ((char)DEBUG_PORT->DAT);
        }
    }

#endif
}

/**
 * @brief    Check any char input from UART
 *
 * @param    None
 *
 * @retval   1: No any char input
 * @retval   0: Have some char input
 *
 * @details  Check UART RSR RX EMPTY or not to determine if any char input from UART
 */

int kbhit(void)
{
    return !((DEBUG_PORT->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk) == 0U);
}
/**
 * @brief    Check if debug message finished
 *
 * @param    None
 *
 * @retval   1: Message is finished
 * @retval   0: Message is transmitting.
 *
 * @details  Check if message finished (FIFO empty of debug port)
 */

int IsDebugFifoEmpty(void)
{
    return ((DEBUG_PORT->FIFOSTS & UART_FIFOSTS_TXEMPTYF_Msk) != 0U);
}

/**
 * @brief    C library retargetting
 *
 * @param[in]  ch  Write a character data
 *
 * @returns  None
 *
 * @details  Check if message finished (FIFO empty of debug port)
 */

void _ttywrch(int ch)
{
    SendChar(ch);
    return;
}


/**
 * @brief      Write character to stream
 *
 * @param[in]  ch       Character to be written. The character is passed as its int promotion.
 * @param[in]  stream   Pointer to a FILE object that identifies the stream where the character is to be written.
 *
 * @returns    If there are no errors, the same character that has been written is returned.
 *             If an error occurs, EOF is returned and the error indicator is set (see ferror).
 *
 * @details    Writes a character to the stream and advances the position indicator.\n
 *             The character is written at the current position of the stream as indicated \n
 *             by the internal position indicator, which is then advanced one character.
 *
 * @note       The above descriptions are copied from http://www.cplusplus.com/reference/clibrary/cstdio/fputc/.
 *
 *
 */

int fputc(int ch, FILE *stream)
{
    (void)stream;
    SendChar(ch);
    return ch;
}


#if (defined(__GNUC__) && !defined(__ARMCC_VERSION))

#if !defined(OS_USE_SEMIHOSTING)
int _write (int fd, char *ptr, int len)
{
    int i = len;

    while(i--) {
        while(DEBUG_PORT->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);

        if(*ptr == '\n') {
            DEBUG_PORT->DAT = '\r';
            while(DEBUG_PORT->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);
        }

        DEBUG_PORT->DAT = *ptr++;
    }
    return len;
}

int _read (int fd, char *ptr, int len)
{

    while((DEBUG_PORT->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk) != 0);
    *ptr = DEBUG_PORT->DAT;
    return 1;


}
#endif

#else
/**
 * @brief      Get character from UART debug port or semihosting input
 *
 * @param[in]  stream   Pointer to a FILE object that identifies the stream on which the operation is to be performed.
 *
 * @returns    The character read from UART debug port or semihosting
 *
 * @details    For get message from debug port or semihosting.
 *
 */

int fgetc(FILE *stream)
{
    (void)stream;
    return ((int)GetChar());
}

/**
 * @brief      Check error indicator
 *
 * @param[in]  stream   Pointer to a FILE object that identifies the stream.
 *
 * @returns    If the error indicator associated with the stream was set, the function returns a nonzero value.
 *             Otherwise, it returns a zero value.
 *
 * @details    Checks if the error indicator associated with stream is set, returning a value different
 *             from zero if it is. This indicator is generally set by a previous operation on the stream that failed.
 *
 * @note       The above descriptions are copied from http://www.cplusplus.com/reference/clibrary/cstdio/ferror/.
 *
 */

int ferror(FILE *stream)
{
    (void)stream;
    return EOF;
}
#endif

#ifdef DEBUG_ENABLE_SEMIHOST
# ifdef __ICCARM__
void __exit(int return_code)
{

    /* Check if link with ICE */
    if(SH_DoCommand(0x18, 0x20026, NULL) == 0)
    {
        /* Make sure all message is print out */
        while(IsDebugFifoEmpty() == 0);
    }
label:
    goto label;  /* endless loop */
}
# else
void _sys_exit(int return_code)
{
    (void)return_code;
    /* Check if link with ICE */
    if(SH_DoCommand(0x18, 0x20026, NULL) == 0)
    {
        /* Make sure all message is print out */
        while(IsDebugFifoEmpty() == 0);
    }
label:
    goto label;  /* endless loop */
}
# endif
#endif



/*** (C) COPYRIGHT 2016-2020 Nuvoton Technology Corp. ***/


