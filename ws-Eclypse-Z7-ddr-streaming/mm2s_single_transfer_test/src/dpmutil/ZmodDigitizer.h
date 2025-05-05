/************************************************************************/
/*                                                                      */
/*  ZmodDigitizer.h - ZmodADC function declarations                     */
/*                                                                      */
/************************************************************************/
/*  Author: Arthur Brown                                                */
/*  Copyright 2022, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  This header file contains the declarations for functions that can   */
/*  be used to calculate and display the calibration constants          */
/*  associated with a Zmod Digitizer.                                   */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  08/30/2022 (ArtVVB): created, adapted from ZmodADC.h                */
/*  02/21/2024 (ArtVVB): added identification functions                 */
/*                                                                      */
/************************************************************************/

#ifndef ZMODDIGITIZER_H_
#define ZMODDIGITIZER_H_

/* ------------------------------------------------------------ */
/*                  Miscellaneous Declarations                  */
/* ------------------------------------------------------------ */

#define prodZmodDigitizer 0x804

/* Define the SYZYGY addresses used for accessing the factory
** and user calibration areas over the SYZYGY I2C bus.
*/
#define addrDigitizerFactCalStart	0x8100
#define addrDigitizerUserCalStart	0x7000

#define cbDigitizerCalMax         128

// The number of frequencies that the digitizer was calibrated at, for which coefficients are stored in DNA
#define cbDigitizerCalibHzSteps   7

/* ------------------------------------------------------------ */
/*                  General Type Declarations                   */
/* ------------------------------------------------------------ */

#pragma pack(push, 1)

typedef struct ZMOD_DIGITIZER_CAL {    			  // 128 B
    BYTE     id;             					  // 0xDD
    int32_t  date;           					  // unix time, secs since epoch
    BYTE     hz[cbDigitizerCalibHzSteps];         // 7 steps: 0=122.88MHz, 50(MHz), 80(MHz), 100(MHz), 110(MHz), 120(MHz), 125(MHz)
    BYTE     nop[3];         					  // reserved
    float    cal[cbDigitizerCalibHzSteps][2][2];  // 7x16B, [hz step][channel][0 multiplicative : 1 additive]
    											  //gain(1.0=100% normally around ~5%), add(Volts normally ~10mV)
    BYTE     crc;            					  // to generate: init 0 and -=byte; the checksum of the structure should be 0
} ZMOD_DIGITIZER_CAL;

#pragma pack(pop)

typedef struct {
	// individual fields are signed 18-bit values with 0s filling the uppermost 14 bits, in the format used by PL calibration hardware
	unsigned int cal[cbDigitizerCalibHzSteps][2][2]; // [hz step][channel 0:1][0 multiplicative : 1 additive]
} ZMOD_DIGITIZER_CAL_S18;

typedef enum {
	ZMOD_DIGITIZER_VARIANT_1430_125=0,
	ZMOD_DIGITIZER_VARIANT_UNSUPPORTED
} ZMOD_DIGITIZER_VARIANT;

/* ------------------------------------------------------------ */
/*                  Variable Declarations                       */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/*                  Procedure Declarations                      */
/* ------------------------------------------------------------ */

BOOL    FDisplayZmodDigitizerCal(int fdI2cDev, BYTE addrI2cSlave);
BOOL    FGetZmodDigitizerCal(int fdI2cDev, BYTE addrI2cSlave, ZMOD_DIGITIZER_CAL* pFactoryCal, ZMOD_DIGITIZER_CAL* pUserCal);
void    FZmodDigitizerCalConvertToS18(ZMOD_DIGITIZER_CAL adcal, ZMOD_DIGITIZER_CAL_S18 *pReturn);
float   FZmodDigitizerGetFrequencyStepMHz(BYTE hz);
BOOL 	FZmodIsDigitizer(DWORD Pdid);
BOOL	FGetZmodDigitizerVariant(DWORD Pdid, ZMOD_DIGITIZER_VARIANT *pVariant);
BOOL 	FGetZmodDigitizerResolution(ZMOD_DIGITIZER_VARIANT variant, DWORD *pResolution);

/* ------------------------------------------------------------ */

#endif /* ZMODADC_H_ */
