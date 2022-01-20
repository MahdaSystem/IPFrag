 /**
 **********************************************************************************
 * @file   IPFrag.c
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

/**
 * @brief  
 * @note   
 * @retval 
 */
#ifdef IPFrag_USE_MACRO_DELAY
#if IPFrag_USE_MACRO_DELAY == 0
#define Delay_MS(x)   Handler->Delay_MS(x)
#else
#define Delay_MS(x)   IPFrag_MACRO_Delay_MS(x)
#ifndef IPFrag_MACRO_Delay_MS
#error "IPFrag_MACRO_Delay_MS is not defined. Please Use handler delay or config IPFrag_MACRO_Delay_MS macro, You can choose it on IPFrag_USE_MACRO_DELAY define"
#endif
#endif
#else
#define Delay_MS(x)
#endif

/**
 ** ==================================================================================
 **                            ##### Private Enums #####                               
 ** ==================================================================================
 **/

/**
 ** ==================================================================================
 **                           ##### Private Typedef #####                               
 ** ==================================================================================
 **/

/**
 ** ==================================================================================
 **                           ##### Private Struct #####                               
 ** ==================================================================================
 **/

/**
 ** ==================================================================================
 **                          ##### Private Variables #####                               
 ** ==================================================================================
 **/

static uint8_t  DataPool[IPFrag_PoolNumber][IPFrag_DataMTUSize] = { 0 };
static uint16_t DataPoolSize[IPFrag_PoolNumber] = { 0 };
static bool     DataPoolLastPos[IPFrag_PoolNumber] = { 0 };

/**
 ** ==================================================================================
 **                            ##### Private Union #####                               
 ** ==================================================================================
 **/

/**
 *! ==================================================================================
 *!                          ##### Private Functions #####                               
 *! ==================================================================================
 **/

/**
 ** ==================================================================================
 **                           ##### Public Functions #####                               
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
IPFrag_TransmitData(IPFrag_Handler_t *Handler, uint8_t *DataBuff, uint32_t SizeofDataBuff)
{
    if (!Handler->TransmitData) return 4;
    if (!DataBuff) return 4;
    if (IPFrag_DataMTUSize % 8 != 0)
    {
        PROGRAMLOG("IPFrag_DataMTUSize is not a factor of 8!\r\n");
        return 5;
    }

    uint8_t CounterPacket = 0;
    bool FullPool = true;
    for (CounterPacket = 0; CounterPacket < IPFrag_PoolNumber; CounterPacket++)
    {
        if (DataPoolSize[CounterPacket] == 0)
        {
//            PROGRAMLOG("Selected CP for Transmit: %u\r\n", CounterPacket);
            FullPool = false;
            break;
        }
    }
    if (FullPool)
    {
        for (uint8_t CounterP = 0; CounterP < IPFrag_PoolNumber; CounterP++)
            DataPoolSize[CounterP] = 0;
        PROGRAMLOG("Pool is Full! It will be reset and CP: 0 will be selected\r\n"
                   "If you are using IPFrag_ReceiveData function, It is highly recommended to flush the input buffer\r\n");
        CounterPacket = 0;
    }

    uint16_t IPVal = 0;
    
    if (Handler->RandomIP)
        IPVal = Handler->RandomIP();
    else
        IPVal = (DataPool[CounterPacket][0] << 16) | (DataPool[CounterPacket][1]) + 1;

    DataPool[CounterPacket][0] = IPVal >> 16;
    DataPool[CounterPacket][1] = IPVal;

    memset(DataPool[CounterPacket] + 2, 0, IPFrag_DataMTUSize - 2);

    if (SizeofDataBuff > (IPFrag_DataMTUSize - 4))
    {
        uint16_t CounterBuffer = 0;

        DataPool[CounterPacket][2] = 0x20; // MF (More Fragments): 1 | DF (Don't Fragment): 0
        DataPool[CounterPacket][3] = 0;  

        do
        {
            memcpy(DataPool[CounterPacket] + 4, &DataBuff[(IPFrag_DataMTUSize - 4) * CounterBuffer], IPFrag_DataMTUSize - 4);
            
            Handler->TransmitData(DataPool[CounterPacket], IPFrag_DataMTUSize);

            Delay_MS(1);
                    
            CounterBuffer++;
            DataPool[CounterPacket][2] = 0x20 | (((IPFrag_DataMTUSize * CounterBuffer / 8) >> 8) & 0x1F); // MF (More Fragments): 1 | DF (Don't Fragment): 0
            DataPool[CounterPacket][3] = IPFrag_DataMTUSize * CounterBuffer / 8;  
            
            SizeofDataBuff -= (IPFrag_DataMTUSize - 4);
        
        } while (SizeofDataBuff > (IPFrag_DataMTUSize - 4));

        DataPool[CounterPacket][2] = ((IPFrag_DataMTUSize * CounterBuffer / 8) >> 8) & 0x1F; // MF (More Fragments): 0 | DF (Don't Fragment): 0
        memcpy(DataPool[CounterPacket] + 4, &DataBuff[(IPFrag_DataMTUSize - 4) * CounterBuffer], SizeofDataBuff);

        Handler->TransmitData(DataPool[CounterPacket], SizeofDataBuff + 4);
    }
    else
    {
        DataPool[CounterPacket][2] = 0x40; // MF (More Fragments): 0 | DF (Don't Fragment): 1
        DataPool[CounterPacket][3] = 0;
    
        memcpy(DataPool[CounterPacket] + 4, DataBuff, SizeofDataBuff);

        Handler->TransmitData(DataPool[CounterPacket], SizeofDataBuff + 4);
    }

    return 0;
}
// Return values:
// 0: successful
// 1: memory error
// 2: timeout error
// 3: full pool buffer
// 4: invalid input pointer
// 5: IPFrag_DataMTUSize is not a factor of 8
uint8_t
IPFrag_ReceiveData(IPFrag_Handler_t* Handler, uint8_t **DataBuff, uint32_t *SizeofDataBuff)
{
    if (!Handler->ReceiveData) return 4;
    if (!DataBuff) return 4;
    if (!SizeofDataBuff) return 4;
    if (IPFrag_DataMTUSize % 8 != 0)
    {
        PROGRAMLOG("IPFrag_DataMTUSize is not a factor of 8!\r\n");
        return 5;
    }

    while (1)
    {
        uint8_t CounterPacket = 0;
        bool FullPool = true;
        for (CounterPacket = 0; CounterPacket < IPFrag_PoolNumber; CounterPacket++)
        {
            if (DataPoolSize[CounterPacket] == 0)
            {
                Handler->ReceiveData(DataPool[CounterPacket], &DataPoolSize[CounterPacket]);
                if (DataPoolSize[CounterPacket] == 0)
                {
                    PROGRAMLOG("No Data received, Timeout!\r\n");
                    return 2;
                }
                else if (DataPoolSize[CounterPacket] < 5)
                {
                    PROGRAMLOG("The size is less than 4 bytes!\r\n");
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
            for (uint8_t CounterP = 0; CounterP < IPFrag_PoolNumber; CounterP++)
            {
                DataPoolSize[CounterP] = 0;
            }
            PROGRAMLOG("Pool is Full! It will be reset, Please recall the function\r\n"
                       "Note that it is highly recommended to flush the input buffer\r\n"
                       "Pay attention to the muximum size of a whole packet\r\n");
            return 3;
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
                    return 2;
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
                PROGRAMLOG("First or middle Packet with offset that is not equal to IPFrag_DataMTUSize, The packet is ignored\r\n");
                DataPoolSize[CounterPacket] = 0;
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
                                if(DataPoolSize[CounterPP] > 0)
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
                                    return 2;
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
                    return 2;
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

        Delay_MS(10);
    }
}
