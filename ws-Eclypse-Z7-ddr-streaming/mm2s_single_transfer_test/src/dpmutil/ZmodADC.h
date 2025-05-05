/************************************************************************/
/*                                                                      */
/*  ZmodADC.h - ZmodADC function declarations                           */
/*                                                                      */
/************************************************************************/
/*  Author: Michael T. Alexander                                        */
/*  Copyright 2020, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  This header file contains the declarations for functions that can   */
/*  be used to calculate and display the calibration constants          */
/*	associated with a Zmod ADC.                                         */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  01/13/2020 (MichaelA): created                                      */
/*  02/21/2024 (ArtVVB): added identification functions                 */
/*                                                                      */
/************************************************************************/

#ifndef ZMODADC_H_
#define ZMODADC_H_

/* ------------------------------------------------------------ */
/*                  Miscellaneous Declarations                  */
/* ------------------------------------------------------------ */

#define prodZmodADC 0x801

/* Define the SYZYGY addresses used for accessing the factory
** and user calibration areas over the SYZYGY I2C bus.
*/
#define addrAdcFactCalStart	0x8100
#define addrAdcUserCalStart	0x7000

#define cbAdcCalMax         128

/* ------------------------------------------------------------ */
/*                  General Type Declarations                   */
/* ------------------------------------------------------------ */

#pragma pack(push, 1)

typedef struct {
    BYTE    id;
    int32_t date; // unix time
    float   cal[2][2][2]; // [channel 0:1][low/high gain 0:1][0 multiplicative : 1 additive]
    BYTE    rsv1[68]; // reserved
    char    log[22]; // BT log: Reference Board SN
    BYTE    crc; // to generate: init 0 and -= 127 bytes; the checksum of the structure should be 0
} ZMOD_ADC_CAL;

#pragma pack(pop)

typedef struct {
	// individual fields are signed 18-bit values with 0s filling the uppermost 14 bits, in the format used by PL calibration hardware
	unsigned int cal[2][2][2]; // [channel 0:1][low/high gain 0:1][0 multiplicative : 1 additive]
} ZMOD_ADC_CAL_S18;

typedef enum {
	ZMOD_ADC_VARIANT_1410_105=0,
	ZMOD_ADC_VARIANT_1010_40,
	ZMOD_ADC_VARIANT_1210_40,
	ZMOD_ADC_VARIANT_1410_40,
	ZMOD_ADC_VARIANT_1010_125,
	ZMOD_ADC_VARIANT_1210_125,
	ZMOD_ADC_VARIANT_1410_125,
	ZMOD_ADC_VARIANT_UNSUPPORTED
} ZMOD_ADC_VARIANT;

/* ------------------------------------------------------------ */
/*                  Variable Declarations                       */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/*                  Procedure Declarations                      */
/* ------------------------------------------------------------ */

BOOL    FDisplayZmodADCCal(int fdI2cDev, BYTE addrI2cSlave);
BOOL    FGetZmodADCCal(int fdI2cDev, BYTE addrI2cSlave, ZMOD_ADC_CAL* pFactoryCal, ZMOD_ADC_CAL* pUserCal);
void    FZmodADCCalConvertToS18(ZMOD_ADC_CAL adcal, ZMOD_ADC_CAL_S18 *pReturn);
BOOL 	FZmodIsADC(DWORD Pdid);
BOOL	FGetZmodADCVariant(DWORD Pdid, ZMOD_ADC_VARIANT *pVariant);
BOOL 	FGetZmodADCResolution(ZMOD_ADC_VARIANT variant, DWORD *pResolution);

/* ------------------------------------------------------------ */

#endif /* ZMODADC_H_ */
