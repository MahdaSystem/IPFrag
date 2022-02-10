
/**
 **********************************************************************************
 * @file   IPFrag.h
 * @author Ali Moallem (https://github.com/AliMoal)
 * @brief  
 **********************************************************************************
 *
 *! Copyright (c) 2022 Mahda Embedded System (MIT License)
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
// 2. The static size would be ((IPFrag_PoolNumber + 1) * (IPFrag_DataMTUSize + 8)) - 8 Bytes
// 3. Maximum size of a whole packet must be less than or equal to IPFrag_PoolNumber * (IPFrag_DataMTUSize - 4)
// 4. This library uses dynamic memory allocation
#define IPFrag_DataMTUSize             1472         // Must be a factor of 8 | Max number of data in a frame to transfer
#define IPFrag_PoolNumber              10          // Number of array to save data
#define IPFrag_USE_MACRO_DELAY         0           // 0: Use handler delay ,So you have to set IPFrag_Delay in Handler | 1: use Macro delay, So you have to set IPFrag_MACRO_DELAY Macro
// #define IPFrag_MACRO_DELAY(x)                      // If you want to use Macro delay, place your delay function
#define IPFRAG_Debug_Enable            1           // 0: Disable debug | 1: Enable debug (depends on printf in stdio.h)              
// #define IPFRAG_Optimization                        // WILL BE ADDED LATER
//? ------------------------------------------------------------------------------- //

/**
 ** ==================================================================================
 **                                ##### Struct #####                               
 ** ==================================================================================
 **/
/**
 * @brief  Handling Library
 * @note   Information about paramters are added in their lines
 */
typedef struct IPFrag_Handler_s
{
    void            (*TransmitData)(uint8_t * Data, uint32_t SizeOfData);   //* Transmit function | Must be initialized at first
    void            (*ReceiveData)(uint8_t * Data, uint32_t * SizeOfData);  //* Receive function | Must be initialized at first
    uint16_t        (*RandomID)(void);                                      //* Random ID function | Can be initialized
    void            (*Delay)(uint32_t);                                     //* Delay function | Can be initialized
    uint32_t        (*GetTick)(void);                                       //* Get Tick of program function | Can be initialized
    const uint32_t    ReceiveTimeout;                                       //* Receiving data | Can be defined
    bool              DataReady;                                            //! DO NOT EDIT THIS
} IPFrag_Handler_t;

/**
 ** ==================================================================================
 **                            ##### Public Functions #####                               
 ** ==================================================================================
 **/
/**
 * @brief  Transmitting data with fragmantation
 * @note   This function works as blocking mode
 * @param  Handler:         Pointer of library handler
 * @param  DataBuff:        Pointer of data to transmit
 * @param  SizeofDataBuff:  Size of data to transmit
 * @retval  0: Successful
 *          1: ---
 *          2: ---
 *          3: Invalid input pointer
 */
uint8_t
IPFrag_TransmitData(IPFrag_Handler_t* Handler, uint8_t* DataBuff, uint32_t SizeofDataBuff);
/**
 *  @brief  Receiving data with fragmantation
 *  @note   This function works as blocking mode
 *  @param  Handler         Pointer of library handler
 *  @param  DataBuff        Pointer of pointer of data to receive
 *          @note           In this function pointer of data will be malloced, 
 *                          Do not malloc it before calling the function to avoid from memory lost!
 *                          And user should free it itself.
 *  @param  SizeofDataBuff  Pointer of size of data to receive
 *  @param  Timeout         Maximum time to be kept in this function
 *          @note           If user does not initialize delay in handler, this parameters treats as number of tries.
 *  @return 0: Successful
 *          1: Memory error
 *          2: Timeout error
 *          3: Invalid input pointer
 */
uint8_t
IPFrag_ReceiveData(IPFrag_Handler_t* Handler, uint8_t** DataBuff, uint32_t* SizeofDataBuff, uint32_t Timeout);
/**
 *  @brief   Receiving data callback
 *  @note    Call this function when a data received
 *  @param   Handler  Pointer of library handler
 *  @return  0: Successful
 *           1: ---
 *           2: ---
 *           3: Invalid input pointer
 *           4: No completed packets
 */
uint8_t
IPFrag_CallbackReceive(IPFrag_Handler_t* Handler);
/**
 *  @brief                  Reading received data
 *  @param  Handler         Pointer of library handler
 *  @param  DataBuff        Pointer of pointer of data to receive
 *          @note           In this function pointer of data will be malloced, 
 *                          Do not malloc it before calling the function to avoid from memory lost!
 *                          And user should free it itself.
 *  @param  SizeofDataBuff  Pointer of size of data to receive
 *  @return 0: Successful
 *          1: Memory error
 *          2: ---
 *          3: Invalid input pointer
 *          4: ---
 *          5: Data is not ready to read, recall the function.
 */
uint8_t
IPFrag_ReadReceive(IPFrag_Handler_t* Handler, uint8_t** DataBuff, uint32_t* SizeofDataBuff);

#ifdef __cplusplus
}
#endif
#endif
