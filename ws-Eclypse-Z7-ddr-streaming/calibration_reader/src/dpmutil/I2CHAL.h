/************************************************************************/
/*                                                                      */
/*  I2CHAL.h - I2C Hardware Abstraction Layer definitions and 			*/
/* 		function declarations   										*/
/*                                                                      */
/************************************************************************/
/*  Author: Thomas Kappenman                                        */
/*  Copyright 2020, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  This header file contains the declarations for functions that can   */
/*  be used to communicate with the platform MCU over the I2C			*/
/*  bus.                                                                */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  05/04/2020 (ThomasK): created                                      */
/*                                                                      */
/************************************************************************/


#ifndef I2CHAL_H_
#define I2CHAL_H_

#include "../dpmutil/stdtypes.h"

#if !defined(__linux__)
#include "xparameters.h"
#endif

/* ------------------------------------------------------------ */
/*            			HAL Declarations                   		*/
/* ------------------------------------------------------------ */

/*#ifdef PLATFORM_ZYNQ
#include "xiicps.h"
#define Iic_Config XIicPs_Config
#define Iic_LookupConfig XIicPs_LookupConfig
#define Iic XIicPs
#define NUMINSTANCES XPAR_XIICPS_NUM_INSTANCES
#define Iic_CfgInitialize XIicPs_CfgInitialize
#elif defined(XPAR_XIIC_NUM_INSTANCES)
#include "xiic.h"
#define Iic_Config XIic_Config
#define Iic_LookupConfig XIic_LookupConfig
#define Iic XIic
#define NUMINSTANCES XPAR_XIIC_NUM_INSTANCES
#define Iic_CfgInitialize XIic_CfgInitialize
#else
#define cchDeviceNameMax	64
#endif*/

/* ------------------------------------------------------------ */
/*                  Procedure Declarations                      */
/* ------------------------------------------------------------ */
#if defined(__linux__)
int I2CHALOpenI2cController();
#else
BOOL I2CHALInit(UINT32 deviceID);
#endif
BOOL I2CHALRead(int fdI2cDev, BYTE slaveAddr, WORD addrRead, BYTE* pbRead, BYTE cbRead, WORD* pcbRead, UINT32 uWait);
BOOL I2CHALWrite(int fdI2cDev, BYTE slaveAddr, WORD addrWrite, BYTE* pbWrite, BYTE cbWrite, INT32 cbDevRxMax, WORD* pcbWritten, INT32 uWait);


#endif
