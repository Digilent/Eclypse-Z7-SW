/************************************************************************/
/*                                                                      */
/*  ZmodDAC.h - ZmodDAC function declarations                           */
/*                                                                      */
/************************************************************************/
/*  Author: Michael T. Alexander                                        */
/*  Copyright 2020, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  This header file contains the declarations for functions that can   */
/*  be used to calculate and display the calibration constants          */
/*  associated with a Zmod DAC.                                         */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  01/13/2020 (MichaelA): created                                      */
/*  02/21/2024 (ArtVVB): added identification functions                 */
/*                                                                      */
/************************************************************************/

#ifndef ZMODDAC_H_
#define ZMODDAC_H_

/* ------------------------------------------------------------ */
/*                  Miscellaneous Declarations                  */
/* ------------------------------------------------------------ */

#define prodZmodDAC 0x802

/* Define the SYZYGY addresses used for accessing the factory
** and user calibration areas over the SYZYGY I2C bus.
*/
#define addrDacFactCalStart 0x8100
#define addrDacUserCalStart 0x7000

#define cbDacCalMax         128

/* ------------------------------------------------------------ */
/*                  General Type Declarations                   */
/* ------------------------------------------------------------ */

#pragma pack(push, 1)

typedef struct {
    BYTE    id;
    int32_t date; // unix time
    float   cal[2][2][2]; // [channel 0:1][low/high gain 0:1][0 multiplicative : 1 additive]
    BYTE    lin[2][34]; // [0:32] libearity calibration, 33 number of averages used during calibration, AD9717 pg 45
    char    log[22]; // BT log: Reference Board SN
    BYTE    crc; // to generate: init 0 and -= 127 bytes; the checksum of the structure should be 0
} ZMOD_DAC_CAL;

#pragma pack(pop)

typedef struct {
	// individual fields are signed 18-bit values with 0s filling the uppermost 14 bits, in the format used by PL calibration hardware
	unsigned int cal[2][2][2]; // [channel 0:1][low/high gain 0:1][0 multiplicative : 1 additive]
} ZMOD_DAC_CAL_S18;

typedef enum {
	ZMOD_DAC_VARIANT_1411_125=0,
	ZMOD_DAC_VARIANT_UNSUPPORTED
} ZMOD_DAC_VARIANT;

/* ------------------------------------------------------------ */
/*                  Variable Declarations                       */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/*                  Procedure Declarations                      */
/* ------------------------------------------------------------ */

BOOL    FDisplayZmodDACCal(int fdI2cDev, BYTE addrI2cSLave);
BOOL    FGetZmodDACCal(int fdI2cDev, BYTE addrI2cSlave, ZMOD_DAC_CAL* pFactoryCal, ZMOD_DAC_CAL* pUserCal);
void    FZmodDACCalConvertToS18(ZMOD_DAC_CAL adcal, ZMOD_DAC_CAL_S18 *pReturn);
BOOL 	FZmodIsDAC(DWORD Pdid);
BOOL	FGetZmodDACVariant(DWORD Pdid, ZMOD_DAC_VARIANT *pVariant);
BOOL 	FGetZmodDACResolution(ZMOD_DAC_VARIANT variant, DWORD *pResolution);

/* ------------------------------------------------------------ */

#endif /* ZMODDAC_H_ */
