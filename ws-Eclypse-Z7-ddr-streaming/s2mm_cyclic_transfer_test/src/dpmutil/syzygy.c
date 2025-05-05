/************************************************************************/
/*                                                                      */
/*  syzygy.c - SYZYGY functions implementation                          */
/*                                                                      */
/************************************************************************/
/*  Author: Michael T. Alexander                                        */
/*  Copyright 2019, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  This source file contains the implementation of functions that can  */
/*  be used to communicate with any SYZYGY pods that are attached       */
/*  to the shared I2C bus of the Eclypse Z7.							*/
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  08/19/2019 (MichaelA): created                                      */
/*	08/20/2019 (MichaelA): added SyzygyReadStdFwRegisters,              */
/*     SyzygyReadDNAHeader, SyzygyReadDNAStrings, SyzygyFreeDNAStrings, */
/*     and SyzygyComputeCRC                                             */
/*	08/27/2019 (MichaelA): added fCheckCrc parameter to the             */
/*      SyzygyReadDNAHeader function                                    */
/*	10/25/2019 (MichaelA): fixed a bug in SyzygyI2cRead that would have	*/
/*		caused the wrong address to be sent if the read operation		*/
/*		required more than two transactions to perform the read			*/
/*		operation														*/
/*  01/03/2020 (MichaelA): added SyzygyI2cWrite							*/
/*	01/06/2020 (MichaelA): modified SyzygyI2cRead parameter types to	*/
/*		support larger data transfers									*/
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
#endif
#include <stdlib.h>
#include <string.h>
#include "stdtypes.h"
#include "../dpmutil/syzygy.h"
#include "../dpmutil/I2CHAL.h"

/* ------------------------------------------------------------ */
/*              Miscellaneous Declarations                      */
/* ------------------------------------------------------------ */

/* Define the maximum number of bytes that the pMCU firmware can
** receive in a single transaction after it has received SLA+W.
*/
#define cbPmcuRxMax	34

/* Define the maximum number of bytes that the pMCU firmware can
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
/***    SyzygyI2cRead
**
**  Parameters:
**  	fdI2cDev        - open file descriptor for underlying I2C device (linux only)
**      addrI2cSlave    - I2C bus address for the slave
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
**      device with the specified I2C bus address. Read operations
**      may be split into multiple transactions with a maximum of
**      32 bytes (per SYZYGY DNA specification 1.0) being retrieved
**      during a single read operation.
*/
BOOL
SyzygyI2cRead(int fdI2cDev, BYTE addrI2cSlave, WORD addrRead, BYTE* pbRead, WORD cbRead, WORD* pcbRead) {
	return I2CHALRead(fdI2cDev, addrI2cSlave, addrRead, pbRead, cbRead, pcbRead, 0);
}

/* ------------------------------------------------------------ */
/***    SyzygyI2cWrite
**
**  Parameters:
**  	fdI2cDev        - open file descriptor for underlying I2C device (linux only)
**      addrI2cSlave    - I2C bus address for the slave
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
**      Peripheral MCU starting at the specified address. Write operations
**      may be split into multiple transactions with a maximum of
**      cbPmcuRxMax bytes being written during a single write operation.
*/
BOOL
SyzygyI2cWrite(int fdI2cDev, BYTE addrI2cSlave, WORD addrWrite, BYTE* pbWrite, WORD cbWrite, WORD* pcbWritten) {
	/* The Attiny44A requires a minimum of 9ms after each erase
	** and 4.5ms after a flash write. The Opal Kelly and Digilent
	** derivative pMCU firmware is written such that up to 32 bytes
	** data will be buffered before the flash write operation is
	** performed. If the specified address falls on a page boundary
	** then a flash page erase is performed. This function splits
	** any data transfer into 32 byte chunks. It is assumed that
	** the caller specifies a starting address that aligns with
	** a page boundary. We kill some time by sleeping before
	** attempting another write operation.
	*/

	return I2CHALWrite(fdI2cDev, addrI2cSlave, addrWrite, pbWrite, cbWrite, cbPmcuRxMax, pcbWritten, 10000);
}

/* ------------------------------------------------------------ */
/***    SyzygyReadStdFwRegisters
**
**  Parameters:
**  	fdI2cDev        - open file descriptor for underlying I2C device (linux only)
**      addrI2cSlave    - I2C bus address for the slave
**      pszgfwregs      - pointer to struce to receive standard fw registers
**
**  Return Value:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function reads the Standard SYZYGY Firmware Registers from the
**      SYZYGY pod with the specified I2C slave address.
*/
BOOL
SyzygyReadStdFwRegisters(int fdI2cDev, BYTE addrI2cSlave, SzgStdFwRegs* pszgfwregs) {

	BYTE	rgbFwReg[6];

	/* Read the standard SYZYGY firmware registers from the SYZYGY pod.
	*/
	if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, 0x0000, rgbFwReg, 6, NULL) ) {
		return fFalse;
	}

	if ( NULL != pszgfwregs ) {
		pszgfwregs->fwverMjr = rgbFwReg[0];
		pszgfwregs->fwverMin = rgbFwReg[1];
		pszgfwregs->dnaverMjr = rgbFwReg[2];
		pszgfwregs->dnaverMin = rgbFwReg[3];
		pszgfwregs->cbEeprom = (rgbFwReg[4] << 8) + rgbFwReg[5];
	}

	return fTrue;
}

/* ------------------------------------------------------------ */
/***    SyzygyReadDNAHeader
**
**  Parameters:
**  	fdI2cDev        - open file descriptor for underlying I2C device (linux only)
**      addrI2cSlave    - I2C bus address for the slave
**      pszgdnahdr      - pointer to DNA Header struct to receive header
**      fCheckCrc       - fTrue to check the header CRC, fFalse to skip check
**
**  Return Value:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function reads the SYZYGY DNA header from the SYZYGY
**      pod with the specified I2C slave address. The CRC is computed
**      and if it matches then a copy of the DNA header is placed into
**      the data structure pointed to by pszgdnahdr.
*/
BOOL
SyzygyReadDNAHeader(int fdI2cDev, BYTE addrI2cSlave, SzgDnaHeader* pszgdnahdr, BOOL fCheckCrc) {

	BYTE	rgbDnaBuf[cbSyzygyDnaHeader];

	/* Read the DNA header from the SYZYGY pod.
	*/
	if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, 0x8000, rgbDnaBuf, cbSyzygyDnaHeader, NULL) ) {
		return fFalse;
	}

	/* Compute the CRC and verify that it's valid.
	*/
	if ( fCheckCrc ) {
		if ( 0 != SyzygyComputeCRC(rgbDnaBuf, cbSyzygyDnaHeader) ) {
			return fFalse;
		}
	}

	if ( NULL != pszgdnahdr ) {
		memcpy(pszgdnahdr, rgbDnaBuf, sizeof(SzgDnaHeader));
	}

	return fTrue;
}

/* ------------------------------------------------------------ */
/***    SyzygyReadDNAStrings
**
**  Parameters:
**  	fdI2cDev        - open file descriptor for underlying I2C device (linux only)
**      addrI2cSlave    - I2C bus address for the slave
**      pszgdnahdr      - pointer to SYZYGY DNA Header
**      pszgdnastrings  - pointer to SYZYGY DNA Strings structure
**
**  Return Value:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function allocates memory for each field of the SzgDnaStrings
**      structure and then reads the associated strings from the SYZYGY pod
**      whose I2C address and DNA header were specified.
**
**      If the string fields of the structure pointed to by pszgdnastrings
**      point to existing memory then that memory will be de-allocated prior
**      to new memory being allocated. This makes it possible to avoid
**      calling SyzygyFreeDNAStrings between consecutive calls to this
**      function. However, it also means that the structure pointed to by
**      pszgdnastrings must be zero initialized before the first time this
**      function is called.
**
**      The caller is responsible for freeing any memory that is allocated
**      for the string fields of the structure pointed to by pszgdnastrings
**      once he or she is done using that information. Memory allocated for
**      these fields may be deallocated by calling SyzygyFreeDNAStrings.
**
**  Notes:
**      This function allocates memory prior to attempting to retrieve
**      any data from the SYZYGY pod. Therefore the caller must deallocate
**      any allocated memory even when this function returns fFalse.
*/
BOOL
SyzygyReadDNAStrings(int fdI2cDev, BYTE addrI2cSlave, SzgDnaHeader* pszgdnahdr, SzgDnaStrings* pszgdnastrings) {

	WORD	addrRead;

	if (( NULL == pszgdnahdr ) || ( NULL == pszgdnastrings )) {
		return fFalse;
	}

	/* Deallocate any existing memory that has been allocated in case the
	** caller forgot to do so. This should help us avoid unintentional
	** memory leaks.
	*/
	SyzygyFreeDNAStrings(pszgdnastrings);

	/* Allocate memory for all strings. A minimum of one character will
	** be allocated so that all strings may be zero terminated.
	*/
	pszgdnastrings->szManufacturerName = (char*)malloc(pszgdnahdr->cbManufacturerName + 1);
	pszgdnastrings->szManufacturerName[0] = '\0';

	pszgdnastrings->szProductName = (char*)malloc(pszgdnahdr->cbProductName + 1);
	pszgdnastrings->szProductName[0] = '\0';

	pszgdnastrings->szProductModel = (char*)malloc(pszgdnahdr->cbProductModel + 1);
	pszgdnastrings->szProductModel[0] = '\0';

	pszgdnastrings->szProductVersion = (char*)malloc(pszgdnahdr->cbProductVersion + 1);
	pszgdnastrings->szProductVersion[0] = '\0';

	pszgdnastrings->szSerialNumber = (char*)malloc(pszgdnahdr->cbSerialNumber + 1);
	pszgdnastrings->szSerialNumber[0] = '\0';

	/* Attempt to read the manufacturer name string.
	*/
	addrRead = 0x8000 + pszgdnahdr->cbDnaHeader;
	if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrRead, (BYTE*)pszgdnastrings->szManufacturerName, pszgdnahdr->cbManufacturerName, NULL) ) {
		return fFalse;
	}
	pszgdnastrings->szManufacturerName[pszgdnahdr->cbManufacturerName] = '\0';

	/* Attempt to read the product name string.
	*/
	addrRead += pszgdnahdr->cbManufacturerName;
	if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrRead, (BYTE*)pszgdnastrings->szProductName, pszgdnahdr->cbProductName, NULL) ) {
		return fFalse;
	}
	pszgdnastrings->szProductName[pszgdnahdr->cbProductName] = '\0';

	/* Attempt to read the product model string.
	*/
	addrRead += pszgdnahdr->cbProductName;
	if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrRead, (BYTE*)pszgdnastrings->szProductModel, pszgdnahdr->cbProductModel, NULL) ) {
		return fFalse;
	}
	pszgdnastrings->szProductModel[pszgdnahdr->cbProductModel] = '\0';

	/* Attempt to read the product version string.
	*/
	addrRead += pszgdnahdr->cbProductModel;
	if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrRead, (BYTE*)pszgdnastrings->szProductVersion, pszgdnahdr->cbProductVersion, NULL) ) {
		return fFalse;
	}
	pszgdnastrings->szProductVersion[pszgdnahdr->cbProductVersion] = '\0';

	/* Attempt to read the serial number string.
	*/
	addrRead += pszgdnahdr->cbProductVersion;
	if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrRead, (BYTE*)pszgdnastrings->szSerialNumber, pszgdnahdr->cbSerialNumber, NULL) ) {
		return fFalse;
	}
	pszgdnastrings->szSerialNumber[pszgdnahdr->cbSerialNumber] = '\0';

	return fTrue;
}

/* ------------------------------------------------------------ */
/***    SyzygyFreeDNAStrings
**
**  Parameters:
**      pszgdnastrings  - pointer to SYZYGY DNA Strings structure
**
**  Return Value:
**      none
**
**  Errors:
**      none
**
**  Description:
**      This function deallocates the memory associated with each string
**      field in the SYZYGY DNA Header and sets each field to point at
**      NULL.
**
**  Notes:
**      Do NOT call this function for a data structure that has not
**      been properly initialized. Doing so may result in the application
**      crashing.
*/
void
SyzygyFreeDNAStrings(SzgDnaStrings* pszgdnastrings) {

	if ( NULL == pszgdnastrings ) {
		return;
	}

	if ( NULL != pszgdnastrings->szManufacturerName ) {
		free(pszgdnastrings->szManufacturerName);
		pszgdnastrings->szManufacturerName = NULL;
	}

	if ( NULL != pszgdnastrings->szProductName ) {
		free(pszgdnastrings->szProductName);
		pszgdnastrings->szProductName = NULL;
	}

	if ( NULL != pszgdnastrings->szProductModel ) {
		free(pszgdnastrings->szProductModel);
		pszgdnastrings->szProductModel = NULL;
	}

	if ( NULL != pszgdnastrings->szProductVersion ) {
		free(pszgdnastrings->szProductVersion);
		pszgdnastrings->szProductVersion = NULL;
	}

	if ( NULL != pszgdnastrings->szSerialNumber ) {
		free(pszgdnastrings->szSerialNumber);
		pszgdnastrings->szSerialNumber = NULL;
	}
}

/* ------------------------------------------------------------ */
/***    SyzygyComputeCRC
**
**  Parameters:
**      pbBuf   - pointer to data buffer over which to compute CRC
**      cbBuf   - size of data buffer in bytes
**
**  Return Value:
**      computed 16-bit CRC
**
**  Errors:
**      none
**
**  Description:
**      This function computes a 16-bit CRC of the data that's
**      passed in via the pbBuf parameter. The polynomial used is
**      0x1021 (x^16 + x^12 + x^5 + x^0) and the initialization
**      value is 0xFFFF. If the CRC is a SYZYGY DNA buffer is valid
**      then this function should return zero.
**
**  Notes:
**      This code is based on Opal Kelly's szgComputeCRC function,
**      which can be found in their open source Brain Tools.
*/
WORD
SyzygyComputeCRC(const BYTE* pbBuf, BYTE cbBuf) {

	WORD	x;
	WORD	crc;

	crc = 0xFFFF;
	while ( 0 < cbBuf ) {
		x = (crc >> 8) ^ *pbBuf;
		x ^= (x >> 4);
		crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ (x);
		crc &= 0xFFFF; // what's the point in this?
		pbBuf++;
		cbBuf--;
	}

	return crc;
}

/* ------------------------------------------------------------ */
/***    IsSyzygyPort
**
**  Parameters:
**      ptypeCheck      - port type to check
**
**  Return Value:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function checks to see if the specified port type
**      corresponds to a SYZYGY port, and if so it returns fTrue.
**      If the specified port is not a SYZYGY port then fFalse is
**      returned.
*/
BOOL
IsSyzygyPort(BYTE ptypeCheck ) {

	switch ( ptypeCheck ) {
		case ptypeSyzygyStd:
		case ptypeSyzygyTxr2:
		case ptypeSyzygyTxr4:
			return fTrue;
		default:
			return fFalse;
	}
}

