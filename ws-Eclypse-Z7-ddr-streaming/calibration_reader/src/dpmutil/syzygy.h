/************************************************************************/
/*                                                                      */
/*  syzygy.h - SYZYGY function declarations                             */
/*                                                                      */
/************************************************************************/
/*  Author: Michael T. Alexander                                        */
/*  Copyright 2019, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  This header file contains the declarations for functions that can   */
/*  be used to communicate with any SYZYGY pods that are attached       */
/*  to the shared I2C bus on the Eclypse Z7.							*/
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  08/19/2019 (MichaelA): created                                      */
/*	08/20/2019 (MichaelA): added declaration of structures SzgStdFwRegs	*/
/*      and SzgDnaStrings as well as several new functions              */
/*	08/27/2019 (MichaelA): added fCheckCrc parameter to the             */
/*      SyzygyReadDNAHeader function                                    */
/*  01/03/2020 (MichaelA): added SyzygyI2cWrite							*/
/*	01/06/2020 (MichaelA): modified SyzygyI2cRead parameter types to	*/
/*		support larger data transfers									*/
/*                                                                      */
/************************************************************************/

#ifndef SYZYGY_H_
#define SYZYGY_H_

/* ------------------------------------------------------------ */
/*                  Miscellaneous Declarations                  */
/* ------------------------------------------------------------ */

/* Define the different types of SmartVIO ports.
*/
#define ptypeNone               0
#define ptypeSyzygyStd          1
#define ptypeSyzygyTxr2         2
#define ptypeSyzygyTxr4         3

/* Define the major and minor version of the SYZYGY DNA specification
** supported by this firmware.
*/
#define szgverMajor     1
#define szgverMinor     0

/* Define the number of bytes contained in the SZYYGY DNA header.
*/
#define cbSyzygyDnaHeader   40

/* Define the SYZYGY attribute fields.
*/
#define sattrLvds           0x0001
#define sattrDoubleWide     0x0002
#define sattrTxr4			0x0004

/* Define useful SYZYGY addresses.
*/
#define addrFlashMagic		0x6999
#define addrDnaStart		0x8000

/* Define the maximum number of bytes that can be stored in the
** SYZYGY DNA section of the pMCU flash.
*/
#define cbSyzygyDnaMax		4096

/* ------------------------------------------------------------ */
/*                  General Type Declarations                   */
/* ------------------------------------------------------------ */

typedef struct {
	BYTE    fwverMjr;
	BYTE    fwverMin;
	BYTE    dnaverMjr;
	BYTE    dnaverMin;
	WORD    cbEeprom;
} SzgStdFwRegs;

typedef struct {
    WORD    cbDna;              // DNA full data length (bytes)
    WORD    cbDnaHeader;        // DNA header length (bytes)
    BYTE    dnaverMjr;          // SYZYGY DNA major version
    BYTE    dnaverMin;          // SYZYGY DNA minor version
    BYTE    dnaverRequiredMjr;  // Required SYZYGY DNA major version
    BYTE    dnaverRequiredMin;  // Required SYZYGY DNA minor version
    WORD    crntRequired5v0;    // Maximum operating current 5V0 rail (mA)
    WORD    crntRequired3v3;    // Maximum operating current 3V3 rail (mA)
    WORD    crntRequiredVio;    // Maximum operating current VIO rail (mA)
    WORD    fsAttributes;       // Attribute flags
    WORD    vltgRange1Min;      // VIO Range 1 minimum voltage (10mV)
    WORD    vltgRange1Max;      // VIO Range 1 maximum voltage (10mV)
    WORD    vltgRange2Min;      // VIO Range 2 minimum voltage (10mV)
    WORD    vltgRange2Max;      // VIO Range 2 maximum voltage (10mV)
    WORD    vltgRange3Min;      // VIO Range 3 minimum voltage (10mV)
    WORD    vltgRange3Max;      // VIO Range 3 maximum voltage (10mV)
    WORD    vltgRange4Min;      // VIO Range 4 minimum voltage (10mV)
    WORD    vltgRange4Max;      // VIO Range 4 maximum voltage (10mV)
    BYTE    cbManufacturerName; // Manufacturer Name Length (bytes)
    BYTE    cbProductName;      // Product Name Length (bytes)
    BYTE    cbProductModel;     // Produt model / Part number length (bytes)
    BYTE    cbProductVersion;   // Product version / revision length (bytes)
    BYTE    cbSerialNumber;     // Serial number length (bytes)
    BYTE    reserved;           // RESERVED field
    BYTE    crcHigh;            // CRC-16 most significant byte
    BYTE    crcLow;             // CRC-16 least significant byte
} SzgDnaHeader;

typedef struct {
	char*   szManufacturerName;
	char*   szProductName;
	char*   szProductModel;
	char*   szProductVersion;
	char*   szSerialNumber;
} SzgDnaStrings;

/* ------------------------------------------------------------ */
/*                  Variable Declarations                       */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/*                  Procedure Declarations                      */
/* ------------------------------------------------------------ */

BOOL	SyzygyI2cRead(int fdI2cDev, BYTE addrI2cSlave, WORD addrRead, BYTE* pbRead, WORD cbRead, WORD* pcbRead);
BOOL	SyzygyI2cWrite(int fdI2cDev, BYTE addrI2cSlave, WORD addrWrite, BYTE* pbWrite, WORD cbWrite, WORD* pcbWritten);
BOOL	SyzygyReadStdFwRegisters(int fdI2cDev, BYTE addrI2cSlave, SzgStdFwRegs* pszgfwregs);
BOOL	SyzygyReadDNAHeader(int fdI2cDev, BYTE addrI2cSlave, SzgDnaHeader* pszgdnahdr, BOOL fCheckCrc);
BOOL	SyzygyReadDNAStrings(int fdI2cDev, BYTE addrI2cSlave, SzgDnaHeader* pszgdnahdr, SzgDnaStrings* pszgdnastrings);
void	SyzygyFreeDNAStrings(SzgDnaStrings* pszgdnastrings);
WORD	SyzygyComputeCRC(const BYTE* pbBuf, BYTE cbBuf);
BOOL	IsSyzygyPort(BYTE ptypeCheck );

/* ------------------------------------------------------------ */

#endif /* SYZYGY_H_ */
