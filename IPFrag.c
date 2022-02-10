 /**
 **********************************************************************************
 * @file   IPFrag.c
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

//* Private Includes -------------------------------------------------------------- //
#include "IPFrag.h"

//* Private Defines and Macros ---------------------------------------------------- //


//* Others ------------------------------------------------------------------------ //

#ifdef IPFRAG_Debug_Enable
#if IPFRAG_Debug_Enable == 1
#include <stdio.h> // for debug
#define PROGRAMLOG(...) printf(__VA_ARGS__)
#else
#define PROGRAMLOG(...)
#endif
#else 
#define PROGRAMLOG(...)
#endif

#ifdef IPFrag_USE_MACRO_DELAY
#if IPFrag_USE_MACRO_DELAY == 0
#define Delay(x)   Handler->Delay(x)
#else
#define Delay(x)   IPFrag_MACRO_Delay(x)
#ifndef IPFrag_MACRO_Delay
#error "IPFrag_MACRO_Delay is not defined. Please Use handler delay or config IPFrag_MACRO_Delay macro, You can choose it on IPFrag_USE_MACRO_DELAY define"
#endif
#endif
#else
#define Delay(x)
#endif

#if IPFrag_DataMTUSize % 8 != 0
#error "IPFrag_DataMTUSize MUST BE A FACTOR OF 8"
#endif

/**
 ** ==================================================================================
 **                          ##### Private Variables #####                               
 ** ==================================================================================
 **/

static uint8_t  DataPool[IPFrag_PoolNumber + 1/*Transmit buffer*/][IPFrag_DataMTUSize] = { 0 };
static uint16_t DataPoolSize[IPFrag_PoolNumber] = { 0 };
static bool     DataPoolLastPos[IPFrag_PoolNumber] = { 0 };
static uint32_t DataPoolTimeout[IPFrag_PoolNumber] = { 0 };

/**
 *! ==================================================================================
 *!                          ##### Private Functions #####                               
 *! ==================================================================================
 **/

static uint32_t GetTickTemp(void) { return 0; }

/**
 ** ==================================================================================
 **                           ##### Public Functions #####                               
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
IPFrag_TransmitData(IPFrag_Handler_t* Handler, uint8_t* DataBuff, uint32_t SizeofDataBuff)
{
    if (!Handler) return 3;
    if (!Handler->TransmitData) return 3;
    if (!DataBuff) return 3;

    uint16_t IPVal = 0;
    
    if (Handler->RandomID)
        IPVal = Handler->RandomID();
    else
        IPVal = (DataPool[IPFrag_PoolNumber][0] << 16) | (DataPool[IPFrag_PoolNumber][1]) + 1;

    DataPool[IPFrag_PoolNumber][0] = IPVal >> 16;
    DataPool[IPFrag_PoolNumber][1] = IPVal;

    memset(DataPool[IPFrag_PoolNumber] + 2, 0, IPFrag_DataMTUSize - 2);

    if (SizeofDataBuff > (IPFrag_DataMTUSize - 4))
    {
        uint16_t CounterBuffer = 0;

        DataPool[IPFrag_PoolNumber][2] = 0x20; // MF (More Fragments): 1 | DF (Don't Fragment): 0
        DataPool[IPFrag_PoolNumber][3] = 0;  

        do
        {
            memcpy(DataPool[IPFrag_PoolNumber] + 4, &DataBuff[(IPFrag_DataMTUSize - 4) * CounterBuffer], IPFrag_DataMTUSize - 4);
            
            Handler->TransmitData(DataPool[IPFrag_PoolNumber], IPFrag_DataMTUSize);

            if (Handler->Delay) Delay(1);
                    
            CounterBuffer++;
            DataPool[IPFrag_PoolNumber][2] = 0x20 | (((IPFrag_DataMTUSize * CounterBuffer / 8) >> 8) & 0x1F); // MF (More Fragments): 1 | DF (Don't Fragment): 0
            DataPool[IPFrag_PoolNumber][3] = IPFrag_DataMTUSize * CounterBuffer / 8;  
            
            SizeofDataBuff -= (IPFrag_DataMTUSize - 4);
        
        } while (SizeofDataBuff > (IPFrag_DataMTUSize - 4));

        DataPool[IPFrag_PoolNumber][2] = ((IPFrag_DataMTUSize * CounterBuffer / 8) >> 8) & 0x1F; // MF (More Fragments): 0 | DF (Don't Fragment): 0
        memcpy(DataPool[IPFrag_PoolNumber] + 4, &DataBuff[(IPFrag_DataMTUSize - 4) * CounterBuffer], SizeofDataBuff);

        Handler->TransmitData(DataPool[IPFrag_PoolNumber], SizeofDataBuff + 4);
    }
    else
    {
        DataPool[IPFrag_PoolNumber][2] = 0x40; // MF (More Fragments): 0 | DF (Don't Fragment): 1
        DataPool[IPFrag_PoolNumber][3] = 0;
    
        memcpy(DataPool[IPFrag_PoolNumber] + 4, DataBuff, SizeofDataBuff);

        Handler->TransmitData(DataPool[IPFrag_PoolNumber], SizeofDataBuff + 4);
    }

    return 0;
}
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
IPFrag_ReceiveData(IPFrag_Handler_t* Handler, uint8_t** DataBuff, uint32_t* SizeofDataBuff, uint32_t Timeout)
{
    if (!Handler) return 3;
    if (!Handler->ReceiveData) return 3;
    if (!DataBuff) return 3;
    if (!SizeofDataBuff) return 3;
    if (!Handler->GetTick) Handler->GetTick = GetTickTemp;

    uint32_t TimeoutCounter = 0;
    do
    {
        uint8_t CounterPacket = 0;
        bool FullPool = true;
        for (CounterPacket = 0; CounterPacket < IPFrag_PoolNumber; CounterPacket++)
        {
            if ((Handler->GetTick() - DataPoolTimeout[CounterPacket]) > Handler->ReceiveTimeout)
                DataPoolSize[CounterPacket] = 0;
            if (DataPoolSize[CounterPacket] == 0)
            {
                DataPoolTimeout[CounterPacket] = Handler->GetTick();
                Handler->ReceiveData(DataPool[CounterPacket], (uint32_t *)&DataPoolSize[CounterPacket]);
                if (DataPoolSize[CounterPacket] < 5)
                {
                    PROGRAMLOG("The size is less than 5 bytes!\r\n");
                    DataPoolSize[CounterPacket] = 0;
                    CounterPacket--;
                    continue;
                }
                DataPoolSize[CounterPacket] -= 4;
                // PROGRAMLOG("New Packet Received | Size: %u | CP: %u\r\n", DataPoolSize[CounterPacket] + 4, CounterPacket);
                FullPool = false;
                break;
            }
        }
        if (FullPool)
        {
            PROGRAMLOG("Pool is Full!\r\n");
            uint32_t DataPoolTempSize = 0;
            uint8_t* DataPoolTemp = malloc(IPFrag_DataMTUSize);
            if (!DataPoolTemp) continue;
            Handler->ReceiveData(DataPoolTemp, &DataPoolTempSize);
            if (DataPoolTempSize < 5) continue;
            for (uint8_t CounterP = 0; CounterP < IPFrag_PoolNumber; CounterP++)
            {
                if (((DataPool[CounterP][0] >> 16) | DataPool[CounterP][1]) == ((DataPoolTemp[0] >> 16) | DataPoolTemp[1]))
                {
                    DataPoolSize[CounterP] = 0;
                }   
            }
            free(DataPoolTemp);
            continue;
        }

        if ((DataPool[CounterPacket][2] & 0x7F) == 0x40) // MF (More Fragments): 0 | DF (Don't Fragment): 1
        {
            if (DataPool[CounterPacket][3] == 0)
            {
                *SizeofDataBuff = DataPoolSize[CounterPacket];

                (*DataBuff) = malloc(*SizeofDataBuff);
                if (!(*DataBuff))
                {
                    PROGRAMLOG("Memory allocation error (simple packet)\r\n");
                    return 1;
                }

                memcpy(*DataBuff, &DataPool[CounterPacket][4], *SizeofDataBuff);

                DataPoolSize[CounterPacket] = 0;
                return 0;
            }
            else
            {
                PROGRAMLOG("Simple packet with offset! The packet is ignored\r\n");
                DataPoolSize[CounterPacket] = 0;
                continue;
            }
        }
        else if ((DataPool[CounterPacket][2] & 0x60) == 0x20) // MF (More Fragments): 1 | DF (Don't Fragment): 0
        {
            if (DataPoolSize[CounterPacket] != (IPFrag_DataMTUSize - 4))
            {
                PROGRAMLOG("First or middle Packet with offset, that is not equal to IPFrag_DataMTUSize, The packet is ignored\r\n");
                for (uint8_t CounterP = 0; CounterP < IPFrag_PoolNumber; CounterP++)
                {
                    if (((DataPool[CounterP][0] >> 16) | DataPool[CounterP][1]) == ((DataPool[CounterPacket][0] >> 16) | DataPool[CounterPacket][1]))
                    {
                        DataPoolSize[CounterP] = 0;
                    }   
                }
                continue;
            }
            for (uint8_t CounterP = 0; CounterP < IPFrag_PoolNumber; CounterP++)
            {
                if (DataPoolSize[CounterPacket])
                {
                    if (((DataPool[CounterPacket][0] >> 16) | DataPool[CounterPacket][1]) == ((DataPool[CounterP][0] >> 16) | DataPool[CounterP][1]))
                    {
                        if (DataPoolLastPos[CounterP])
                        {
                            uint8_t NumberOfPacket = (((((DataPool[CounterP][2] & 0x1F) << 8) | DataPool[CounterP][3]) * 8) / IPFrag_DataMTUSize) + 1;
                            uint8_t PacketC = 0;
                            for (uint8_t CounterPP = 0; CounterPP < IPFrag_PoolNumber; CounterPP++)
                            {
                                if (DataPoolSize[CounterPP] > 0)
                                {
                                    if (((DataPool[CounterPP][0] >> 16) | DataPool[CounterPP][1]) == ((DataPool[CounterP][0] >> 16) | DataPool[CounterP][1]))
                                    {
                                        PacketC++;
                                    }
                                }
                            }
                            if (NumberOfPacket == PacketC)
                            {
                                *SizeofDataBuff = ((IPFrag_DataMTUSize - 4) * (PacketC - 1)) + DataPoolSize[CounterP];

                                (*DataBuff) = malloc(*SizeofDataBuff);
                                if (!(*DataBuff))
                                {
                                    PROGRAMLOG("Memory allocation error (multiple packet, first or middle)\r\n");
                                    return 1;
                                }

                                uint8_t CounterOffset = 0;
                                for (uint8_t CounterPP = 0; CounterPP < IPFrag_PoolNumber; CounterPP++)
                                {
                                    if (((((DataPool[CounterPP][2] & 0x1F) << 8) | DataPool[CounterPP][3]) * 8) == (CounterOffset * IPFrag_DataMTUSize))
                                    {
                                        memcpy(&(*DataBuff)[CounterOffset * (IPFrag_DataMTUSize - 4)], &DataPool[CounterPP][4], DataPoolSize[CounterPP]);
                                        CounterOffset++;
                                        DataPoolSize[CounterPP] = 0;
                                    }
                                }
                                DataPoolLastPos[CounterP] = false;
                                return 0;
                            }
                            break;
                        }
                    }
                }
            }
        }
        else if ((DataPool[CounterPacket][2] & 0x60) == 0) // MF (More Fragments): 0 | DF (Don't Fragment): 0
        {
            DataPoolLastPos[CounterPacket] = true;
            uint8_t NumberOfPacket = (((((DataPool[CounterPacket][2] & 0x1F) << 8) | DataPool[CounterPacket][3]) * 8) / IPFrag_DataMTUSize) + 1;
            uint8_t PacketC = 0;
            for (uint8_t CounterPP = 0; CounterPP < IPFrag_PoolNumber; CounterPP++)
            {
                if (DataPoolSize[CounterPP] > 0)
                {
                    if (((DataPool[CounterPP][0] >> 16) | DataPool[CounterPP][1]) == ((DataPool[CounterPacket][0] >> 16) | DataPool[CounterPacket][1]))
                    {
                        PacketC++;
                    }
                }
            }

            if (NumberOfPacket == PacketC)
            {
                *SizeofDataBuff = ((IPFrag_DataMTUSize - 4) * (PacketC - 1)) + DataPoolSize[CounterPacket];

                (*DataBuff) = malloc(*SizeofDataBuff);
                if (!(*DataBuff))
                {
                    PROGRAMLOG("Memory allocation error (multiple packet, last)\r\n");
                    return 1;
                }

                uint8_t CounterOffset = 0;
                for (uint8_t CounterPP = 0; CounterPP < IPFrag_PoolNumber; CounterPP++)
                {
                    if (((((DataPool[CounterPP][2] & 0x1F) << 8) | DataPool[CounterPP][3]) * 8) == (CounterOffset * IPFrag_DataMTUSize))
                    {
                        memcpy(&(*DataBuff)[CounterOffset * (IPFrag_DataMTUSize - 4)], &DataPool[CounterPP][4], DataPoolSize[CounterPP]);
                        CounterOffset++;
                        DataPoolSize[CounterPP] = 0;
                    }
                }
                DataPoolLastPos[CounterPacket] = false;
                return 0;
            }
        }
        else // Wrong Packet
        {
            PROGRAMLOG("Wrong packet, The packet is ignored\r\n");
            DataPoolSize[CounterPacket] = 0;
        }

        if (Handler->Delay) 
          Delay(1);
        TimeoutCounter++;
    } while (TimeoutCounter < Timeout);
    return 2;
}
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
IPFrag_CallbackReceive(IPFrag_Handler_t* Handler)
{
    if (!Handler) return 3;
    if (!Handler->ReceiveData) return 3;
    if (!Handler->GetTick) Handler->GetTick = GetTickTemp;

    uint8_t CounterPacket = 0;
    bool FullPool = true;
    for (CounterPacket = 0; CounterPacket < IPFrag_PoolNumber; CounterPacket++)
    {
        if ((Handler->GetTick() - DataPoolTimeout[CounterPacket]) > Handler->ReceiveTimeout)
            DataPoolSize[CounterPacket] = 0;
        if (DataPoolSize[CounterPacket] == 0)
        {
            DataPoolTimeout[CounterPacket] = Handler->GetTick();
            Handler->ReceiveData(DataPool[CounterPacket], (uint32_t *)&DataPoolSize[CounterPacket]);
            if (DataPoolSize[CounterPacket] < 5)
            {
                PROGRAMLOG("The size is less than 5 bytes!\r\n");
                DataPoolSize[CounterPacket] = 0;
                CounterPacket--;
                continue;
            }
            DataPoolSize[CounterPacket] -= 4;
            // PROGRAMLOG("New Packet Received | Size: %u | CP: %u\r\n", DataPoolSize[CounterPacket] + 4, CounterPacket);
            FullPool = false;
            break;
        }
    }
    if (FullPool)
    {
        PROGRAMLOG("Pool is Full!\r\n");
        uint32_t DataPoolTempSize = 0;
        uint8_t* DataPoolTemp = malloc(IPFrag_DataMTUSize);
        if (!DataPoolTemp) return IPFrag_CallbackReceive(Handler);
        Handler->ReceiveData(DataPoolTemp, &DataPoolTempSize);
        if (DataPoolTempSize < 5) return IPFrag_CallbackReceive(Handler);
        for (uint8_t CounterP = 0; CounterP < IPFrag_PoolNumber; CounterP++)
        {
            if (((DataPool[CounterP][0] >> 16) | DataPool[CounterP][1]) == ((DataPoolTemp[0] >> 16) | DataPoolTemp[1]))
            {
                DataPoolSize[CounterP] = 0;
            }
        }
        free(DataPoolTemp);
        return IPFrag_CallbackReceive(Handler);
    }

    if ((DataPool[CounterPacket][2] & 0x7F) == 0x40) // MF (More Fragments): 0 | DF (Don't Fragment): 1
    {
        if (DataPool[CounterPacket][3] == 0)
        {
            Handler->DataReady = true;
            return 0;
        }
        else
        {
            PROGRAMLOG("Simple packet with offset! The packet is ignored\r\n");
            DataPoolSize[CounterPacket] = 0;
        }
    }
    else if ((DataPool[CounterPacket][2] & 0x60) == 0x20) // MF (More Fragments): 1 | DF (Don't Fragment): 0
    {
        if (DataPoolSize[CounterPacket] != (IPFrag_DataMTUSize - 4))
        {
            PROGRAMLOG("First or middle Packet with offset that is not equal to IPFrag_DataMTUSize, The packet is ignored\r\n");
            for (uint8_t CounterP = 0; CounterP < IPFrag_PoolNumber; CounterP++)
            {
                if (((DataPool[CounterP][0] >> 16) | DataPool[CounterP][1]) == ((DataPool[CounterPacket][0] >> 16) | DataPool[CounterPacket][1]))
                {
                    DataPoolSize[CounterP] = 0;
                }   
            }
        }
        for (uint8_t CounterP = 0; CounterP < IPFrag_PoolNumber; CounterP++)
        {
            if (DataPoolSize[CounterPacket])
            {
                if (((DataPool[CounterPacket][0] >> 16) | DataPool[CounterPacket][1]) == ((DataPool[CounterP][0] >> 16) | DataPool[CounterP][1]))
                {
                    if (DataPoolLastPos[CounterP])
                    {
                        uint8_t NumberOfPacket = (((((DataPool[CounterP][2] & 0x1F) << 8) | DataPool[CounterP][3]) * 8) / IPFrag_DataMTUSize) + 1;
                        uint8_t PacketC = 0;
                        for (uint8_t CounterPP = 0; CounterPP < IPFrag_PoolNumber; CounterPP++)
                        {
                            if (DataPoolSize[CounterPP] > 0)
                            {
                                if (((DataPool[CounterPP][0] >> 16) | DataPool[CounterPP][1]) == ((DataPool[CounterP][0] >> 16) | DataPool[CounterP][1]))
                                {
                                    PacketC++;
                                }
                            }
                        }
                        if (NumberOfPacket == PacketC)
                        {
                            Handler->DataReady = true;
                            return 0;
                        }
                        break;
                    }
                }
            }
        }
    }
    else if ((DataPool[CounterPacket][2] & 0x60) == 0) // MF (More Fragments): 0 | DF (Don't Fragment): 0
    {
        DataPoolLastPos[CounterPacket] = true;
        uint8_t NumberOfPacket = (((((DataPool[CounterPacket][2] & 0x1F) << 8) | DataPool[CounterPacket][3]) * 8) / IPFrag_DataMTUSize) + 1;
        uint8_t PacketC = 0;
        for (uint8_t CounterPP = 0; CounterPP < IPFrag_PoolNumber; CounterPP++)
        {
            if (DataPoolSize[CounterPP] > 0)
            {
                if (((DataPool[CounterPP][0] >> 16) | DataPool[CounterPP][1]) == ((DataPool[CounterPacket][0] >> 16) | DataPool[CounterPacket][1]))
                {
                    PacketC++;
                }
            }
        }

        if (NumberOfPacket == PacketC)
        {
            Handler->DataReady = true;
            return 0;
        }
    }
    else // Wrong Packet
    {
        PROGRAMLOG("Wrong packet, The packet is ignored\r\n");
        DataPoolSize[CounterPacket] = 0;
    }

    // if (Handler->Delay) Delay(1);
    return 4;
}
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
IPFrag_ReadReceive(IPFrag_Handler_t* Handler, uint8_t** DataBuff, uint32_t* SizeofDataBuff)
{
    if (!Handler) return 3;
    if (Handler->DataReady)
    {
        if (!DataBuff) return 3;
        if (!SizeofDataBuff) return 3;

        uint32_t CounterPacket = 0;
        for (uint8_t CounterP = 0; CounterP < IPFrag_PoolNumber; CounterP++)
        {
            if (DataPoolLastPos[CounterP])
                CounterPacket = CounterP;
        }

        uint8_t NumberOfPacket = (((((DataPool[CounterPacket][2] & 0x1F) << 8) | DataPool[CounterPacket][3]) * 8) / IPFrag_DataMTUSize) + 1;
        uint8_t PacketC = 0;
        for (uint8_t CounterPP = 0; CounterPP < IPFrag_PoolNumber; CounterPP++)
        {
            if (DataPoolSize[CounterPP] > 0)
            {
                if (((DataPool[CounterPP][0] >> 16) | DataPool[CounterPP][1]) == ((DataPool[CounterPacket][0] >> 16) | DataPool[CounterPacket][1]))
                {
                    PacketC++;
                }
            }
        }

        if (NumberOfPacket == PacketC)
        {
            *SizeofDataBuff = ((IPFrag_DataMTUSize - 4) * (PacketC - 1)) + DataPoolSize[CounterPacket];

            (*DataBuff) = malloc(*SizeofDataBuff);
            if (!(*DataBuff))
            {
                PROGRAMLOG("Memory allocation error\r\n");
                return 1;
            }

            uint8_t CounterOffset = 0;
            for (uint8_t CounterPP = 0; CounterPP < IPFrag_PoolNumber; CounterPP++)
            {
                if (((((DataPool[CounterPP][2] & 0x1F) << 8) | DataPool[CounterPP][3]) * 8) == (CounterOffset * IPFrag_DataMTUSize))
                {
                    memcpy(&(*DataBuff)[CounterOffset * (IPFrag_DataMTUSize - 4)], &DataPool[CounterPP][4], DataPoolSize[CounterPP]);
                    CounterOffset++;
                    DataPoolSize[CounterPP] = 0;
                }
            }
            DataPoolLastPos[CounterPacket] = false;
            Handler->DataReady = false;
            return 0;
        }
    }
    return 5;
}
