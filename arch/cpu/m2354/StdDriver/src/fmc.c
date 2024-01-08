/**************************************************************************//**
 * @file     fmc.c
 * @version  V3.00
 * @brief    M2354 Series Flash Memory Controller(FMC) driver source file
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include "NuMicro.h"

/** @addtogroup Standard_Driver Standard Driver
  @{
*/

/** @addtogroup FMC_Driver FMC Driver
  @{
*/

int32_t g_FMC_i32ErrCode = 0; /*!< FMC global error code */

/** @addtogroup FMC_EXPORTED_FUNCTIONS FMC Exported Functions
  @{
*/

/**
  * @brief Run flash all one verification and get result.
  *
  * @param[in] u32addr   Starting flash address. It must be a page aligned address.
  * @param[in] u32count  Byte count of flash to be calculated. It must be multiple of 512 bytes.
  *
  * @retval   READ_ALLONE_YES       The contents of verified flash area are 0xA11FFFFF.
  * @retval   READ_ALLONE_NOT       Some contents of verified flash area are not 0xA1100000.
  * @retval   READ_ALLONE_CMD_FAIL  Unexpected error occurred.
  *
  * @details  Run ISP check all one command to check specify area is all one or not.
  *
  * @note     Global error code g_FMC_i32ErrCode
  *           -1  RUN_ALL_ONE or CHECK_ALL_ONE commands time-out
  */
uint32_t  FMC_CheckAllOne(uint32_t u32addr, uint32_t u32count)
{
    uint32_t  ret = READ_ALLONE_CMD_FAIL;
    int32_t   i32TimeOutCnt0, i32TimeOutCnt1;

    g_FMC_i32ErrCode = 0;

    FMC_ISP->ISPSTS = 0x80UL;   /* clear check all one bit */

    FMC_ISP->ISPCMD   = FMC_ISPCMD_RUN_ALL1;
    FMC_ISP->ISPADDR  = u32addr;
    FMC_ISP->ISPDAT   = u32count;
    FMC_ISP->ISPTRG   = FMC_ISPTRG_ISPGO_Msk;

    i32TimeOutCnt0 = FMC_TIMEOUT_CHKALLONE;
    while(FMC_ISP->ISPSTS & FMC_ISPSTS_ISPBUSY_Msk)
    {
        if( i32TimeOutCnt0-- <= 0)
        {
            g_FMC_i32ErrCode = -1;
            break;
        }
    }

    if(g_FMC_i32ErrCode == 0)
    {
        i32TimeOutCnt1 = FMC_TIMEOUT_CHKALLONE;
        do
        {
            FMC_ISP->ISPCMD = FMC_ISPCMD_READ_ALL1;
            FMC_ISP->ISPADDR = u32addr;
            FMC_ISP->ISPTRG = FMC_ISPTRG_ISPGO_Msk;

            i32TimeOutCnt0 = FMC_TIMEOUT_CHKALLONE;
            while(FMC_ISP->ISPSTS & FMC_ISPSTS_ISPBUSY_Msk)
            {
                if( i32TimeOutCnt0-- <= 0)
                {
                    g_FMC_i32ErrCode = -1;
                    break;
                }
            }

            if( i32TimeOutCnt1-- <= 0)
            {
                g_FMC_i32ErrCode = -1;
            }
        }
        while( (FMC_ISP->ISPDAT == 0UL) && (g_FMC_i32ErrCode == 0) );
    }

    if( g_FMC_i32ErrCode == 0)
    {
        if( FMC_ISP->ISPDAT == READ_ALLONE_YES )
            ret = READ_ALLONE_YES;
        else if( FMC_ISP->ISPDAT == READ_ALLONE_NOT )
            ret = READ_ALLONE_NOT;
        else
            g_FMC_i32ErrCode = -1;
    }

    return ret;
}


/**
  * @brief    Disable ISP Functions
  *
  * @param    None
  *
  * @return   None
  *
  * @details  This function will clear ISPEN bit of ISPCON to disable ISP function
  *
  */
void FMC_Close(void)
{
    FMC->ISPCTL &= ~FMC_ISPCTL_ISPEN_Msk;
}

/**
  * @brief     Config XOM Region
  * @param[in] u32XomNum    The XOM number(0~3)
  * @param[in] u32XomBase   The XOM region base address.
  * @param[in] u8XomPage    The XOM page number of region size.
  *
  * @retval   0   Success
  * @retval   1   XOM is has already actived.
  * @retval   -1  Program failed.
  * @retval   -2  Invalid XOM number.
  * @retval   -3  Invalid XOM region.
  *
  * @details  Program XOM base address and XOM size(page)
  *
  * @note     Global error code g_FMC_i32ErrCode
  *           -1  Program failed or program time-out
  *           -2  Invalid XOM number.
  *           -3  Invalid XOM region.
  */
int32_t FMC_ConfigXOM(uint32_t u32XomNum, uint32_t u32XomBase, uint8_t u8XomPage)
{
    int32_t  ret = 0;
    uint32_t u32ApromSize;
    int32_t i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    /* Get APROM size */
    u32ApromSize = FULL_ROMSIZE_SEL ? FMC_APROM_END : FMC_APROM_END/2;

    if(u32XomNum >= 4UL)
    {
        g_FMC_i32ErrCode = -2;
        ret = -2;
    }

    if((u32XomBase == FMC_APROM_BASE) || ((u32XomBase + FMC_FLASH_PAGE_SIZE * u8XomPage) > u32ApromSize))
    {
        g_FMC_i32ErrCode = -3;
        ret = -3;
    }

    if(ret == 0)
    {
        ret = FMC_GetXOMState(u32XomNum);
    }

    if(ret == 0)
    {
        FMC_ISP->ISPCMD = FMC_ISPCMD_PROGRAM;
        FMC_ISP->ISPADDR = FMC_XOM_BASE + (u32XomNum * 0x10u);
        FMC_ISP->ISPDAT = u32XomBase;
        FMC_ISP->ISPTRG = FMC_ISPTRG_ISPGO_Msk;
        i32TimeOutCnt = FMC_TIMEOUT_WRITE;
        while(FMC_ISP->ISPTRG & FMC_ISPTRG_ISPGO_Msk)
        {
            if( i32TimeOutCnt-- <= 0)
            {
                g_FMC_i32ErrCode = -1;
                ret = -1;
                break;
            }
        }

        if(FMC_ISP->ISPSTS & FMC_ISPSTS_ISPFF_Msk)
        {
            FMC_ISP->ISPSTS |= FMC_ISPSTS_ISPFF_Msk;
            g_FMC_i32ErrCode = -1;
            ret = -1;
        }
    }

    if(ret == 0)
    {
        FMC_ISP->ISPCMD = FMC_ISPCMD_PROGRAM;
        FMC_ISP->ISPADDR = FMC_XOM_BASE + (u32XomNum * 0x10u + 0x04u);
        FMC_ISP->ISPDAT = u8XomPage;
        FMC_ISP->ISPTRG = FMC_ISPTRG_ISPGO_Msk;
        i32TimeOutCnt = FMC_TIMEOUT_WRITE;
        while(FMC_ISP->ISPTRG & FMC_ISPTRG_ISPGO_Msk)
        {
            if( i32TimeOutCnt-- <= 0)
            {
                g_FMC_i32ErrCode = -1;
                ret = -1;
                break;
            }
        }

        if(FMC_ISP->ISPSTS & FMC_ISPSTS_ISPFF_Msk)
        {
            FMC_ISP->ISPSTS |= FMC_ISPSTS_ISPFF_Msk;
            g_FMC_i32ErrCode = -1;
            ret = -1;
        }
    }

    if(ret == 0)
    {
        FMC_ISP->ISPCMD = FMC_ISPCMD_PROGRAM;
        FMC_ISP->ISPADDR = FMC_XOM_BASE + (u32XomNum * 0x10u + 0x08u);
        FMC_ISP->ISPDAT = 0u;
        FMC_ISP->ISPTRG = FMC_ISPTRG_ISPGO_Msk;
        i32TimeOutCnt = FMC_TIMEOUT_WRITE;
        while(FMC_ISP->ISPTRG & FMC_ISPTRG_ISPGO_Msk)
        {
            if( i32TimeOutCnt-- <= 0)
            {
                g_FMC_i32ErrCode = -1;
                ret = -1;
                break;
            }
        }

        if(FMC_ISP->ISPSTS & FMC_ISPSTS_ISPFF_Msk)
        {
            FMC_ISP->ISPSTS |= FMC_ISPSTS_ISPFF_Msk;
            g_FMC_i32ErrCode = -1;
            ret = -1;
        }
    }

    return ret;
}

/**
  * @brief      Execute Flash Page erase
  *
  * @param[in]  u32PageAddr Address of the flash page to be erased.
  *             It must be a 2048 bytes aligned address.
  *
  * @return     ISP page erase success or not.
  * @retval     0  Success
  * @retval     -1  Erase failed
  *
  * @details    Execute FMC_ISPCMD_PAGE_ERASE command to erase a flash page. The page size is 2048 bytes.
  *
  * @note       Global error code g_FMC_i32ErrCode
  *             -1  Erase failed or erase time-out
  */
int32_t FMC_Erase(uint32_t u32PageAddr)
{
    int32_t  ret = 0;
    int32_t  i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    if(ret == 0)
    {
        FMC_ISP->ISPCMD = FMC_ISPCMD_PAGE_ERASE;
        FMC_ISP->ISPADDR = u32PageAddr;
        FMC_ISP->ISPTRG = FMC_ISPTRG_ISPGO_Msk;

        i32TimeOutCnt = FMC_TIMEOUT_ERASE;
        while(FMC_ISP->ISPTRG & FMC_ISPTRG_ISPGO_Msk)
        {
            if( i32TimeOutCnt-- <= 0)
            {
                g_FMC_i32ErrCode = -1;
                ret = -1;
                break;
            }
        }

        if(FMC_ISP->ISPCTL & FMC_ISPCTL_ISPFF_Msk)
        {
            FMC_ISP->ISPCTL |= FMC_ISPCTL_ISPFF_Msk;
            g_FMC_i32ErrCode = -1;
            ret = -1;
        }
    }
    return ret;
}

/**
  * @brief      Execute Flash Bank erase
  *
  * @param[in]  u32BankAddr Base address of the flash bank to be erased.
  *
  * @return     ISP bank erase success or not.
  * @retval     0  Success
  * @retval     -1  Erase failed
  *
  * @details    Execute FMC_ISPCMD_BANK_ERASE command to erase a flash block.
  *
  * @note       Global error code g_FMC_i32ErrCode
  *             -1  Erase failed or erase time-out
  */
int32_t FMC_EraseBank(uint32_t u32BankAddr)
{
    int32_t  ret = 0;
    int32_t  i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    FMC->ISPCMD = FMC_ISPCMD_BANK_ERASE;
    FMC->ISPADDR = u32BankAddr;
    FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;

    i32TimeOutCnt = FMC_TIMEOUT_ERASE;
    while(FMC->ISPTRG & FMC_ISPTRG_ISPGO_Msk)
    {
        if( i32TimeOutCnt-- <= 0)
        {
            g_FMC_i32ErrCode = -1;
            ret = -1;
            break;
        }
    }

    if(FMC->ISPCTL & FMC_ISPCTL_ISPFF_Msk)
    {
        FMC->ISPCTL |= FMC_ISPCTL_ISPFF_Msk;
        g_FMC_i32ErrCode = -1;
        ret = -1;
    }
    return ret;
}


/**
  * @brief  Execute Erase XOM Region
  *
  * @param[in]  u32XomNum  The XOMRn(n=0~3)
  *
  * @return   XOM erase success or not.
  * @retval    0  Success
  * @retval   -1  Erase failed
  * @retval   -2  Invalid XOM number.
  *
  * @details Execute FMC_ISPCMD_PAGE_ERASE command to erase XOM.
  *
  * @note     Global error code g_FMC_i32ErrCode
  *           -1  Program failed or program time-out
  *           -2  Invalid XOM number.
  */
int32_t FMC_EraseXOM(uint32_t u32XomNum)
{
    uint32_t u32Addr;
    int32_t i32Active, err = 0;
    int32_t i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    if(u32XomNum >= 4UL)
    {
        err = -2;
    }

    if(err == 0)
    {
        i32Active = FMC_GetXOMState(u32XomNum);

        if(i32Active)
        {
            u32Addr = (((uint32_t)(&FMC->XOMR0STS)[u32XomNum]) & 0xFFFFFF00u) >> 8u;

            FMC->ISPCMD = FMC_ISPCMD_PAGE_ERASE;
            FMC->ISPADDR = u32Addr;
            FMC->ISPDAT = 0x55aa03u;
            FMC->ISPTRG = 0x1u;
#if ISBEN
            __ISB();
#endif
            i32TimeOutCnt = FMC_TIMEOUT_ERASE;
            while(FMC->ISPTRG)
           {
                if( i32TimeOutCnt-- <= 0)
                {
                    err = -1;
                    break;
                }
            }

            /* Check ISPFF flag to know whether erase OK or fail. */
            if(FMC->ISPCTL & FMC_ISPCTL_ISPFF_Msk)
            {
                FMC->ISPCTL |= FMC_ISPCTL_ISPFF_Msk;
                err = -1;
            }
        }
        else
        {
            err = -1;
        }
    }
    g_FMC_i32ErrCode = err;
    return err;
}

/**
  * @brief     Run CRC32 checksum calculation and get result.
  *
  * @param[in] u32addr   Starting flash address. It must be a page aligned address.
  * @param[in] u32count  Byte count of flash to be calculated. It must be multiple of 2048bytes.
  *
  * @return    Success or not.
  * @retval    0           Success.
  * @retval    0xFFFFFFFF  Invalid parameter.
  *
  * @details   Run ISP CRC32 checksum command to calculate checksum then get and return checksum data.
  *
  * @note      Global error code g_FMC_i32ErrCode
  *            -1  Run/Read check sum time-out failed
  *            -2  u32addr or u32count must be aligned with 2048
  */
uint32_t  FMC_GetChkSum(uint32_t u32addr, uint32_t u32count)
{
    uint32_t   ret;
    int32_t i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    if((u32addr % 2048UL) || (u32count % 2048UL))
    {
        g_FMC_i32ErrCode = -2;
        ret = 0xFFFFFFFF;
    }
    else
    {
        FMC_ISP->ISPCMD  = FMC_ISPCMD_RUN_CKS;
        FMC_ISP->ISPADDR = u32addr;
        FMC_ISP->ISPDAT  = u32count;
        FMC_ISP->ISPTRG  = FMC_ISPTRG_ISPGO_Msk;

        i32TimeOutCnt = FMC_TIMEOUT_CHKSUM;
        while(FMC_ISP->ISPSTS & FMC_ISPSTS_ISPBUSY_Msk)
        {
            if( i32TimeOutCnt-- <= 0)
            {
                g_FMC_i32ErrCode = -1;
                return 0xFFFFFFFF;
            }
        }

        FMC_ISP->ISPCMD = FMC_ISPCMD_READ_CKS;
        FMC_ISP->ISPADDR = u32addr;
        FMC_ISP->ISPTRG = FMC_ISPTRG_ISPGO_Msk;

        i32TimeOutCnt = FMC_TIMEOUT_CHKSUM;
        while(FMC_ISP->ISPSTS & FMC_ISPSTS_ISPBUSY_Msk)
        {
            if( i32TimeOutCnt-- <= 0)
            {
                g_FMC_i32ErrCode = -1;
                return 0xFFFFFFFF;
            }
        }

        ret = FMC_ISP->ISPDAT;
    }

    return ret;
}

/**
  * @brief  Check the OTP is locked or not.
  *
  * @param[in] u32OtpNum    The OTP number.
  *
  * @retval   1   OTP is locked.
  * @retval   0   OTP is not locked.
  * @retval   -1  Failed to read OTP lock bits.
  * @retval   -2  Invalid OTP number.
  *
  * @details  To get specify OTP lock status
  *
  * @note     Global error code g_FMC_i32ErrCode
  *           -1  Failed to read OTP lock bits or read time-out
  *           -2  Invalid OTP number
  */
int32_t FMC_IsOTPLocked(uint32_t u32OtpNum)
{
    int32_t  ret = 0;
    int32_t i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    if(u32OtpNum > 255UL)
    {
        g_FMC_i32ErrCode = -2;
        ret = -2;
    }

    if(ret == 0)
    {
        FMC->ISPCMD = FMC_ISPCMD_READ;
        FMC->ISPADDR = FMC_OTP_BASE + 0x800UL + u32OtpNum * 4UL;
        FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;

        i32TimeOutCnt = FMC_TIMEOUT_READ;
        while(FMC->ISPTRG & FMC_ISPTRG_ISPGO_Msk)
        {
            if( i32TimeOutCnt-- <= 0)
            {
                g_FMC_i32ErrCode = -1;
                ret = -1;
                break;
            }
        }

        if(FMC->ISPSTS & FMC_ISPSTS_ISPFF_Msk)
        {
            FMC->ISPSTS |= FMC_ISPSTS_ISPFF_Msk;
            g_FMC_i32ErrCode = -1;
            ret = -1;
        }
        else
        {
            if(FMC->ISPDAT != 0xFFFFFFFFUL)
            {
                g_FMC_i32ErrCode = -1;
                ret = 1;   /* Lock work was progrmmed. OTP was locked. */
            }
        }
    }
    return ret;
}

/**
  * @brief  Check the XOM is actived or not.
  *
  * @param[in] u32XomNum    The xom number(0~3).
  *
  * @retval   1   XOM is actived.
  * @retval   0   XOM is not actived.
  * @retval   -2  Invalid XOM number.
  *
  * @details To get specify XOMRn(n=0~3) active status
  */
int32_t FMC_GetXOMState(uint32_t u32XomNum)
{
    uint32_t u32act;
    int32_t  ret = 0;

    if(u32XomNum >= 4UL)
    {
        ret = -2;
    }

    if(ret >= 0)
    {
        u32act = (((FMC_ISP->XOMSTS) & 0xful) & (1ul << u32XomNum)) >> u32XomNum;
        ret = (int32_t)u32act;
    }
    return ret;
}

/**
  * @brief  Lock the specified OTP.
  *
  * @param[in] u32OtpNum    The OTP number.
  *
  * @retval    0  Success
  * @retval   -1  Failed to write OTP lock bits.
  * @retval   -2  Invalid OTP number.
  *
  * @details  To lock specified OTP number
  *
  * @note     Global error code g_FMC_i32ErrCode
  *           -1  Failed to write OTP lock bits or write time-out
  *           -2  Invalid OTP number
  */
int32_t FMC_LockOTP(uint32_t u32OtpNum)
{
    int32_t  ret = 0;
    int32_t i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    if(u32OtpNum > 255UL)
    {
        g_FMC_i32ErrCode = -2;
        ret = -2;
    }

    if(ret == 0)
    {
        FMC->ISPCMD = FMC_ISPCMD_PROGRAM;
        FMC->ISPADDR = FMC_OTP_BASE + 0x800UL + u32OtpNum * 4UL;
        FMC->ISPDAT = 0UL;
        FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;

        i32TimeOutCnt = FMC_TIMEOUT_WRITE;
        while(FMC->ISPTRG & FMC_ISPTRG_ISPGO_Msk)
        {
            if( i32TimeOutCnt-- <= 0)
            {
                g_FMC_i32ErrCode = -1;
                ret = -1;
                break;
            }
        }

        if(FMC->ISPSTS & FMC_ISPSTS_ISPFF_Msk)
        {
            FMC->ISPSTS |= FMC_ISPSTS_ISPFF_Msk;
            g_FMC_i32ErrCode = -1;
            ret = -1;
        }
    }
    return ret;
}

/**
  * @brief    Enable FMC ISP function
  *
  * @param    None
  *
  * @return   None
  *
  * @details  ISPEN bit of ISPCON must be set before we can use ISP commands.
  *           Therefore, To use all FMC function APIs, user needs to call FMC_Open() first to enable ISP functions.
  *
  * @note     ISP functions are write-protected. user also needs to unlock it by calling SYS_UnlockReg() before using all ISP functions.
  *
  */
void FMC_Open(void)
{
    FMC_ISP->ISPCTL |=  FMC_ISPCTL_ISPEN_Msk;
}


/**
  * @brief      Read a word bytes from flash
  *
  * @param[in]  u32Addr Address of the flash location to be read.
  *             It must be a word aligned address.
  *
  * @return     The word data read from specified flash address.
  *             Return 0xFFFFFFFF if read failed.
  *
  * @details    Execute FMC_ISPCMD_READ command to read a word from flash.
  *
  * @note       Global error code g_FMC_i32ErrCode
  *             -1  Read time-out
  */
uint32_t FMC_Read(uint32_t u32Addr)
{
    int32_t i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    FMC_ISP->ISPCMD = FMC_ISPCMD_READ;
    FMC_ISP->ISPADDR = u32Addr;
    FMC_ISP->ISPTRG = FMC_ISPTRG_ISPGO_Msk;

    i32TimeOutCnt = FMC_TIMEOUT_READ;
    while(FMC_ISP->ISPTRG & FMC_ISPTRG_ISPGO_Msk)
    {
        if( i32TimeOutCnt-- <= 0)
        {
            g_FMC_i32ErrCode = -1;
            return 0xFFFFFFFF;
        }
    }

    return FMC_ISP->ISPDAT;
}

/**
  * @brief      Read a double-word bytes from flash
  *
  * @param[in]  u32addr   Address of the flash location to be read.
  *             It must be a double-word aligned address.
  *
  * @param[out] u32data0  Place holder of word 0 read from flash address u32addr.
  * @param[out] u32data1  Place holder of word 0 read from flash address u32addr+4.
  *
  * @return     0   Success
  * @return     -1  Failed
  *
  * @details    Execute FMC_ISPCMD_READ_64 command to read a double-word from flash.
  *
  * @note       Global error code g_FMC_i32ErrCode
  *             -1  Read time-out
  */
int32_t FMC_Read64(uint32_t u32addr, uint32_t * u32data0, uint32_t * u32data1)
{
    int32_t  ret = 0;
    int32_t i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    FMC->ISPCMD = FMC_ISPCMD_READ_64;
    FMC->ISPADDR = u32addr;
    FMC->ISPDAT = 0x0UL;
    FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;

    i32TimeOutCnt = FMC_TIMEOUT_READ;
    while(FMC->ISPSTS & FMC_ISPSTS_ISPBUSY_Msk)
    {
        if( i32TimeOutCnt-- <= 0)
        {
            g_FMC_i32ErrCode = -1;
            ret = -1;
            break;
        }
    }

    if(FMC->ISPSTS & FMC_ISPSTS_ISPFF_Msk)
    {
        FMC->ISPSTS |= FMC_ISPSTS_ISPFF_Msk;
        g_FMC_i32ErrCode = -1;
        ret = -1;
    }
    else
    {
        *u32data0 = FMC->MPDAT0;
        *u32data1 = FMC->MPDAT1;
    }
    return ret;
}

/**
  * @brief  Read data from OTP
  *
  * @param[in] u32OtpNum    The OTP number(0~255).
  * @param[in] u32LowWord   Low word of the 64-bits data.
  * @param[in] u32HighWord  High word of the 64-bits data.
  *
  * @retval    0  Success
  * @retval   -1  Read failed.
  * @retval   -2  Invalid OTP number.
  *
  * @details  Read the 64-bits data from the specified OTP.
  *
  * @note     Global error code g_FMC_i32ErrCode
  *           -1  Read failed or time-out
  *           -2  Invalid OTP number
  */
int32_t FMC_ReadOTP(uint32_t u32OtpNum, uint32_t *u32LowWord, uint32_t *u32HighWord)
{
    int32_t  ret = 0;
    int32_t i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    if(u32OtpNum > 255UL)
    {
        g_FMC_i32ErrCode = -2;
        ret = -2;
    }

    if(ret == 0)
    {
        FMC->ISPCMD = FMC_ISPCMD_READ_64;
        FMC->ISPADDR = FMC_OTP_BASE + u32OtpNum * 8UL ;
        FMC->ISPDAT = 0x0UL;
        FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;

        i32TimeOutCnt = FMC_TIMEOUT_READ;
        while(FMC->ISPSTS & FMC_ISPSTS_ISPBUSY_Msk)
        {
            if( i32TimeOutCnt-- <= 0)
            {
                g_FMC_i32ErrCode = -1;
                ret = -1;
                break;
            }
        }

        if(FMC->ISPSTS & FMC_ISPSTS_ISPFF_Msk)
        {
            FMC->ISPSTS |= FMC_ISPSTS_ISPFF_Msk;
            g_FMC_i32ErrCode = -1;
            ret = -1;
        }
        else
        {
            *u32LowWord = FMC->MPDAT0;
            *u32HighWord = FMC->MPDAT1;
        }
    }
    return ret;
}

/**
  * @brief       Read the User Configuration words.
  *
  * @param[out]  u32Config[]  The word buffer to store the User Configuration data.
  * @param[in]   u32Count   The word count to be read.
  *
  * @return      Success or not.
  * @retval       0 Success
  * @retval      -1 Failed
  *
  * @details     This function is used to read the settings of user configuration.
  *              if u32Count = 1, Only CONFIG0 will be returned to the buffer specified by u32Config.
  *              if u32Count = 2, Both CONFIG0 and CONFIG1 will be returned.
  *
  * @note        Global error code g_FMC_i32ErrCode
  *              -1  Read failed
  */
int32_t FMC_ReadConfig(uint32_t u32Config[], uint32_t u32Count)
{
    uint32_t i;
    int32_t ret = 0;

    for(i = 0u; i < u32Count; i++)
    {
        u32Config[i] = FMC_Read(FMC_CONFIG_BASE + i * 4u);
    }

    if (g_FMC_i32ErrCode != 0) ret = g_FMC_i32ErrCode;

    return ret;
}

/**
  * @brief      Write a word bytes to flash.
  *
  * @param[in]  u32Addr Address of the flash location to be programmed.
  *             It must be a word aligned address.
  * @param[in]  u32Data The word data to be programmed.
  *
  * @return      0  Success
  * @return     -1  Failed
  *
  * @details    Execute ISP FMC_ISPCMD_PROGRAM to program a word to flash.
  *
  * @note       Global error code g_FMC_i32ErrCode
  *             -1  Program failed or time-out
  */
int32_t FMC_Write(uint32_t u32Addr, uint32_t u32Data)
{
    int32_t i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    FMC_ISP->ISPCMD = FMC_ISPCMD_PROGRAM;
    FMC_ISP->ISPADDR = u32Addr;
    FMC_ISP->ISPDAT = u32Data;
    FMC_ISP->ISPTRG = FMC_ISPTRG_ISPGO_Msk;

    i32TimeOutCnt = FMC_TIMEOUT_WRITE;
    while(FMC_ISP->ISPTRG & FMC_ISPTRG_ISPGO_Msk)
    {
        if( i32TimeOutCnt-- <= 0)
        {
            g_FMC_i32ErrCode = -1;
            return -1;
        }
    }

    return 0;
}

/**
  * @brief      Write a double-word bytes to flash
  *
  * @param[in]  u32addr Address of the flash location to be programmed.
  *             It must be a double-word aligned address.
  * @param[in]  u32data0   The word data to be programmed to flash address u32addr.
  * @param[in]  u32data1   The word data to be programmed to flash address u32addr+4.
  *
  * @return     0   Success
  * @return     -1  Failed
  *
  * @details    Execute ISP FMC_ISPCMD_PROGRAM_64 to program a double-word to flash.
  *
  * @note       Global error code g_FMC_i32ErrCode
  *             -1  Program failed or time-out
  */
int32_t FMC_Write8Bytes(uint32_t u32addr, uint32_t u32data0, uint32_t u32data1)
{
    int32_t  ret = 0;
    int32_t i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    FMC->ISPCMD  = FMC_ISPCMD_PROGRAM_64;
    FMC->ISPADDR = u32addr;
    FMC->MPDAT0  = u32data0;
    FMC->MPDAT1  = u32data1;
    FMC->ISPTRG  = FMC_ISPTRG_ISPGO_Msk;

    i32TimeOutCnt = FMC_TIMEOUT_WRITE;
    while(FMC->ISPSTS & FMC_ISPSTS_ISPBUSY_Msk)
    {
        if( i32TimeOutCnt-- <= 0)
        {
            g_FMC_i32ErrCode = -1;
            return -1;
        }
    }

    if(FMC->ISPSTS & FMC_ISPSTS_ISPFF_Msk)
    {
        FMC->ISPSTS |= FMC_ISPSTS_ISPFF_Msk;
        g_FMC_i32ErrCode = -1;
        ret = -1;
    }
    return ret;
}

/**
  * @brief    Write User Configuration
  *
  * @param[in]  au32Config[] The word buffer to store the User Configuration data.
  * @param[in]  u32Count The word count to program to User Configuration.
  *
  * @retval     0 Success
  * @retval    -1 Failed
  *
  * @details  User must enable User Configuration update before writing it.
  *           User must erase User Configuration before writing it.
  *           User Configuration is also be page erase. User needs to backup necessary data
  *           before erase User Configuration.
  *
  * @note     Global error code g_FMC_i32ErrCode
  *           -1  Program failed or time-out
  */
int32_t FMC_WriteConfig(uint32_t au32Config[], uint32_t u32Count)
{
    int32_t  ret = 0;
    uint32_t i;

    FMC_ENABLE_CFG_UPDATE();
    for(i = 0u; i < u32Count; i++)
    {
        FMC_Write(FMC_CONFIG_BASE + i * 4u, au32Config[i]);
        if (g_FMC_i32ErrCode != 0)
        {
            ret = -1;
        }
        if(FMC_Read(FMC_CONFIG_BASE + i * 4u) != au32Config[i])
        {
            ret = -1;
        }
    }
    FMC_DISABLE_CFG_UPDATE();
    return ret;
}

/**
  * @brief      Write Multi-Word bytes to flash
  *
  * @param[in]  u32Addr    Start flash address in APROM where the data chunk to be programmed into.
  *                        This address must be 8-bytes aligned to flash address.
  * @param[in]  pu32Buf    Buffer that carry the data chunk.
  * @param[in]  u32Len     Length of the data chunk in bytes.
  *
  * @retval     >=0  Number of data bytes were programmed.
  * @retval     -1   Program failed.
  * @retval     -2   Invalid address.
  *
  * @details    Program Multi-Word data into specified address of flash.
  *
  * @note       Global error code g_FMC_i32ErrCode
  *             -1  Program failed or time-out
  *             -2  Invalid address
  */

int32_t FMC_WriteMultiple(uint32_t u32Addr, uint32_t pu32Buf[], uint32_t u32Len)
{

    uint32_t i, idx, u32OnProg;
    int32_t err, retval = 0, i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    if((u32Addr >= FMC_APROM_END) || ((u32Addr % 8) != 0))
    {
        g_FMC_i32ErrCode = -2;
        return -2;
    }

    idx = 0u;
    FMC->ISPCMD = FMC_ISPCMD_PROGRAM_MUL;
    FMC->ISPADDR = u32Addr;
    retval += 16;
    do
    {
        err = 0;
        u32OnProg = 1u;
        FMC->MPDAT0 = pu32Buf[idx + 0u];
        FMC->MPDAT1 = pu32Buf[idx + 1u];
        FMC->MPDAT2 = pu32Buf[idx + 2u];
        FMC->MPDAT3 = pu32Buf[idx + 3u];
        FMC->ISPTRG = 0x1u;
        idx += 4u;

        for(i = idx; i < (u32Len / 4u); i += 4u) /* Max data length is 256 bytes (512/4 words) */
        {
            i32TimeOutCnt = FMC_TIMEOUT_WRITE;
            __set_PRIMASK(1u); /* Mask interrupt to avoid status check coherence error */
            do
            {
                if((FMC->MPSTS & FMC_MPSTS_MPBUSY_Msk) == 0u)
                {
                    __set_PRIMASK(0u);

                    FMC->ISPADDR = FMC->MPADDR & (~0xful);
                    idx = (FMC->ISPADDR - u32Addr) / 4u;
                    err = -1;
                }
                if( i32TimeOutCnt-- <= 0)
                {
                    g_FMC_i32ErrCode = -1;
                    err = -1;
                }
            }
            while((FMC->MPSTS & (3u << FMC_MPSTS_D0_Pos)) && (err == 0));

            if(err == 0)
            {
                retval += 8;

                /* Update new data for D0 */
                FMC->MPDAT0 = pu32Buf[i];
                FMC->MPDAT1 = pu32Buf[i + 1u];
                i32TimeOutCnt = FMC_TIMEOUT_WRITE;
                do
                {
                    if((FMC->MPSTS & FMC_MPSTS_MPBUSY_Msk) == 0u)
                    {
                        __set_PRIMASK(0u);
                        FMC->ISPADDR = FMC->MPADDR & (~0xful);
                        idx = (FMC->ISPADDR - u32Addr) / 4u;
                        err = -1;
                    }
                    if( i32TimeOutCnt-- <= 0)
                    {
                        g_FMC_i32ErrCode = -1;
                        err = -1;
                    }
                }
                while((FMC->MPSTS & (3u << FMC_MPSTS_D2_Pos)) && (err == 0));

                if(err == 0)
                {
                    retval += 8;

                    /* Update new data for D2 */
                    FMC->MPDAT2 = pu32Buf[i + 2u];
                    FMC->MPDAT3 = pu32Buf[i + 3u];
                    __set_PRIMASK(0u);
                }
            }

            if(err < 0)
            {
                break;
            }
        }
        if(err == 0)
        {
            u32OnProg = 0u;
            i32TimeOutCnt = FMC_TIMEOUT_WRITE;
            while(FMC->ISPSTS & FMC_ISPSTS_ISPBUSY_Msk)
            {
                if( i32TimeOutCnt-- <= 0)
                {
                    g_FMC_i32ErrCode = -1;
                    break;
                }
            }
        }
    }
    while(u32OnProg);

    return retval;
}

/**
  * @brief     Write data to OTP
  *
  * @param[in] u32OtpNum    The OTP number(0~255).
  * @param[in] u32LowWord   Low word of the 64-bits data.
  * @param[in] u32HighWord   High word of the 64-bits data.
  *
  * @retval    0   Success
  * @retval    -1  Program failed.
  * @retval    -2  Invalid OTP number.
  *
  * @details  Program a 64-bits data to the specified OTP.
  *
  * @note     Global error code g_FMC_i32ErrCode
  *           -1  Program failed or time-out
  *           -2  Invalid address
  */
int32_t FMC_WriteOTP(uint32_t u32OtpNum, uint32_t u32LowWord, uint32_t u32HighWord)
{
    int32_t  ret = 0, i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    if(u32OtpNum > 255UL)
    {
        g_FMC_i32ErrCode = -2;
        ret = -2;
    }

    if(ret == 0)
    {
        FMC->ISPCMD = FMC_ISPCMD_PROGRAM;
        FMC->ISPADDR = FMC_OTP_BASE + u32OtpNum * 8UL;
        FMC->ISPDAT = u32LowWord;
        FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;

        i32TimeOutCnt = FMC_TIMEOUT_WRITE;
        while(FMC->ISPTRG & FMC_ISPTRG_ISPGO_Msk)
        {
            if( i32TimeOutCnt-- <= 0)
            {
                g_FMC_i32ErrCode = -1;
                ret = -1;
                break;
            }
        }

        if(FMC->ISPSTS & FMC_ISPSTS_ISPFF_Msk)
        {
            FMC->ISPSTS |= FMC_ISPSTS_ISPFF_Msk;
            g_FMC_i32ErrCode = -1;
            ret = -1;
        }
    }

    if(ret == 0)
    {
        FMC->ISPCMD = FMC_ISPCMD_PROGRAM;
        FMC->ISPADDR = FMC_OTP_BASE + u32OtpNum * 8UL + 4UL;
        FMC->ISPDAT = u32HighWord;
        FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;

        i32TimeOutCnt = FMC_TIMEOUT_WRITE;
        while(FMC->ISPTRG & FMC_ISPTRG_ISPGO_Msk)
        {
            if( i32TimeOutCnt-- <= 0)
            {
                g_FMC_i32ErrCode = -1;
                ret = -1;
                break;
            }
        }

        if(FMC->ISPSTS & FMC_ISPSTS_ISPFF_Msk)
        {
            FMC->ISPSTS |= FMC_ISPSTS_ISPFF_Msk;
            g_FMC_i32ErrCode = -1;
            ret = -1;
        }
    }

    return ret;
}

/**
  * @brief     Remap Bank0/Bank1
  *
  * @param[in] u32Bank    Bank Num which will remap to.
  *
  * @retval    0   Success
  * @retval    -1  Program failed.
  *
  * @details  Remap Bank0/Bank1
  *
  * @note     Global error code g_FMC_i32ErrCode
  *           -1  Program failed or time-out
  */
int32_t FMC_RemapBank(uint32_t u32Bank)
{
    int32_t  ret = 0;
    int32_t i32TimeOutCnt;

    g_FMC_i32ErrCode = 0;

    FMC->ISPCMD = FMC_ISPCMD_BANK_REMAP;
    FMC->ISPADDR = u32Bank;
    FMC->ISPDAT = 0x5AA55AA5UL;
    FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;

    i32TimeOutCnt = FMC_TIMEOUT_WRITE;
    while(FMC->ISPTRG & FMC_ISPTRG_ISPGO_Msk)
    {
        if( i32TimeOutCnt-- <= 0)
        {
            g_FMC_i32ErrCode = -1;
            ret = -1;
            break;
        }
    }

    if(FMC->ISPCTL & FMC_ISPCTL_ISPFF_Msk)
    {
        FMC->ISPCTL |= FMC_ISPCTL_ISPFF_Msk;
        g_FMC_i32ErrCode = -1;
        ret = -1;
    }
    return ret;
}

/**@}*/ /* end of group FMC_EXPORTED_FUNCTIONS */

/**@}*/ /* end of group FMC_Driver */

/**@}*/ /* end of group Standard_Driver */
