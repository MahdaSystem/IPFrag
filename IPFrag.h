
/**
 **********************************************************************************
 * @file   IPFrag.h
 * @author Ali Moallem (https://github.com/AliMoal)
 * @brief  
 **********************************************************************************
 *
 *! Copyright (c) 2022 Ali Moallem (MIT License)
 *!
 *! Permission is hereby granted, free of charge, to any person obtaining a copy
 *! of this software and associated documentation files (the "Software"), to deal
 *! in the Software without restriction, including without limitation the rights
 *! to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *! copies of the Software, and to permit persons to whom the Software is
 *! furnished to do so, subject to the following conditions:
 *!
 *! The above copyright notice and this permission notice shall be included in all
 *! copies or substantial portions of the Software.
 *!
 *! THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *! IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *! FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *! AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *! LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *! OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *! SOFTWARE.
 *!
 **********************************************************************************
 **/

//* Define to prevent recursive inclusion ---------------------------------------- //
#ifndef IPFARG_H
#define IPFARG_H

#ifdef __cplusplus
extern "C" {
#endif

//* Includes ---------------------------------------------------------------------- //
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

//? User Configurations and Notes ------------------------------------------------- //
// Important Notes:
// 1. Declare IPFrag_Handler_t one struct and fill it before calling any functions
// 2. The static size would be IPFrag_PoolNumber * (IPFrag_DataMTUSize + 3) Bytes
// 3. Maximum size of a whole packet must be less than or equal to IPFrag_PoolNumber * (IPFrag_DataMTUSize - 4)
#define IPFrag_DataMTUSize             1400        // Must be a factor of 8 | Max number of data in a frame to transfer
#define IPFrag_PoolNumber              10          // Number of array to save data
#define IPFrag_USE_MACRO_DELAY         0           // 0: Use handler delay ,So you have to set IPFrag_Delay_MS in Handler | 1: use Macro delay, So you have to set IPFrag_MACRO_DELAY_MS Macro
// #define IPFrag_MACRO_DELAY_MS(x)                   // If you want to use Macro delay, place your delay function in milliseconds here
#define IPFRAG_Debug_Enable            1           // 0: Disable debug | 1: Enable debug (depends on printf in stdio.h)              
// #define IPFRAG_Optimization                        // WILL BE ADDED LATER
//? ------------------------------------------------------------------------------- //

//* Defines and Macros ------------------------------------------------------------ //


//! DO NOT USE OR EDIT THIS BLOCK ------------------------------------------------- //
//! ------------------------------------------------------------------------------- //

/**
 ** ==================================================================================
 **                                 ##### Enums #####                               
 ** ==================================================================================
 **/

/**
 ** ==================================================================================
 **                                ##### Typedef #####                               
 ** ==================================================================================
 **/

/**
 ** ==================================================================================
 **                                ##### Struct #####                               
 ** ==================================================================================
 **/

typedef struct IPFrag_Handler_s
{
    void      (*TransmitData)(uint8_t *Data, uint32_t SizeOfData);
    void      (*ReceiveData)(uint8_t *Data, uint32_t *SizeOfData);
    uint16_t  (*RandomIP)(void);
    void      (*Delay_MS)(uint32_t MS);
} IPFrag_Handler_t;


/**
 ** ==================================================================================
 **                               ##### Variables #####                               
 ** ==================================================================================
 **/

/**
 ** ==================================================================================
 **                                 ##### Union #####                               
 ** ==================================================================================
 **/

/**
 ** ==================================================================================
 **                            ##### Public Functions #####                               
 ** ==================================================================================
 **/

// Return values:
// 0: successful
// 1: ---
// 2: ---
// 3: ---
// 4: invalid input pointer
// 5: IPFrag_DataMTUSize is not a factor of 8
uint8_t
IPFrag_TransmitData(IPFrag_Handler_t *Handler, uint8_t *DataBuff, uint32_t SizeofDataBuff);
// Return values:
// 0: successful
// 1: memory error
// 2: timeout error
// 3: full pool buffer
// 4: invalid input pointer
// 5: IPFrag_DataMTUSize is not a factor of 8
uint8_t
IPFrag_ReceiveData(IPFrag_Handler_t *Handler, uint8_t **DataBuff, uint32_t *SizeofDataBuff);

#ifdef __cplusplus
}
#endif
#endif
