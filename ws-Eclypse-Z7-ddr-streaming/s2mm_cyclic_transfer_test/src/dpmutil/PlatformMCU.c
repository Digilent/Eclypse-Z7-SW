/************************************************************************/
/*                                                                      */
/*  PlatformMCU.c - Platform MCU functions implementation                */
/*                                                                      */
/************************************************************************/
/*  Author: Michael T. Alexander                                        */
/*  Copyright 2019, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  This source file contains the implementation of functions that can  */
/*  be used to communicate with the Eclypse Platform MCU utilizing the  */
/*  I2C bus.                                                            */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  08/21/2019 (MichaelA): created                                      */
/*	08/22/2019 (MIchaelA): rewrote PmcuI2cWrite based to work within	*/
/*      the limitations of the PMCU firmware                            */
/*	05/04/2020 (ThomasK): added baremetal support. changed to I2CHAL	*/
/* 		I2C calls														*/
/*                                                                      */
/************************************************************************/

/* ------------------------------------------------------------ */
/*              Include File Definitions                        */
/* ------------------------------------------------------------ */

#if defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <time.h>
#else
#include "sleep.h"
#endif
#include <stdlib.h>
#include "stdtypes.h"
#include "../dpmutil/PlatformMCU.h"
#include "../dpmutil/I2CHAL.h"

/* ------------------------------------------------------------ */
/*              Miscellaneous Declarations                      */
/* ------------------------------------------------------------ */

/* Define the maximum number of bytes that the PMCU firmware can
** receive in a single transaction after it has received SLA+W.
*/
#define cbPmcuRxMax	6

/* Define the maximum number of bytes that the PMCU firmware can
** transmit in a single transaction after it has received SLA+R.
*/
#define cbPmcuTxMax 32


/* ------------------------------------------------------------ */
/*              Local Type Definitions                          */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Global Variables                                */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Local Variables                                 */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Forward Declarations                            */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Procedure Definitions                           */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/***    PmcuI2cRead
**
**  Parameters:
**      fdI2cDev        - open file descriptor for underlying I2C device (linux only)
**      addrRead        - memory address to read
**      pbRead          - pointer to a buffer to receive data
**      cbRead          - number of bytes to read
**      pcbRead         - pointer to variable to receive count of bytes
**                        read
**
**  Return Value:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function reads the specified number of bytes from the
**      Platform MCU starting at the specified address. Read operations
**      may be split into multiple transactions with a maximum of
**      cbPmcuTxMax bytes being retrieved during a single read operation.
*/
BOOL
PmcuI2cRead(int fdI2cDev, WORD addrRead, BYTE* pbRead, BYTE cbRead, WORD* pcbRead) {
	return I2CHALRead(fdI2cDev, addrPlatformMcuI2c, addrRead, pbRead, cbRead, pcbRead, 50);
}

/* ------------------------------------------------------------ */
/***    PmcuI2cWrite
**
**  Parameters:
**      fdI2cDev        - open file descriptor for underlying I2C device
**      addrWrite       - memory address to write
**      pbWrite         - pointer to a buffer to containing data to transmit
**      cbWrite         - number of bytes to write
**      pcbWrite        - pointer to variable to receive count of bytes
**                        written
**
**  Return Value:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function writes the specified number of bytes to the
**      Platform MCU starting at the specified address. Write operations
**      may be split into multiple transactions with a maximum of
**      cbPmcuRxMax bytes being written during a single write operation.
*/
BOOL
PmcuI2cWrite(int fdI2cDev, WORD addrWrite, BYTE* pbWrite, BYTE cbWrite, WORD* pcbWritten) {
	return I2CHALWrite(fdI2cDev, addrPlatformMcuI2c, addrWrite, pbWrite, cbWrite, cbPmcuRxMax, pcbWritten, 0);
}
