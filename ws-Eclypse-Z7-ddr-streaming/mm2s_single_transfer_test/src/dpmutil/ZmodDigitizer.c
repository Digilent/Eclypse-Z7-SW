/************************************************************************/
/*                                                                      */
/*  ZmodDigitizer.c - ZmodDigitizer functions implementation            */
/*                                                                      */
/************************************************************************/
/*  Author: Arthur Brown                                                */
/*  Copyright 2022, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  This source file contains the implementation of functions that can  */
/*  be used to calculate and display the calibration constants          */
/*  associated with a Zmod Digitizer.                                   */
/*                                                                      */
/*  Note: the code for calculating the coefficients was provided by     */
/*  Tudor Gherman.                                                      */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  08/30/2022 (ArtVVB): created, ported from ZmodDigitizer.c           */
/*  02/21/2024 (ArtVVB): added identification functions                 */
/*                                                                      */
/************************************************************************/

/* ------------------------------------------------------------ */
/*              Include File Definitions                        */
/* ------------------------------------------------------------ */

#if defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/i2c-dev.h>
#endif
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "stdtypes.h"
#include "../dpmutil/syzygy.h"
#include "../dpmutil/ZmodDigitizer.h"

/* ------------------------------------------------------------ */
/*              Miscellaneous Declarations                      */
/* ------------------------------------------------------------ */

// Real range value taken from
#define DIGITIZER_IDEAL_RANGE_ADC    1.0
#define DIGITIZER_REAL_RANGE_ADC     1.055

/* ------------------------------------------------------------ */
/*              Local Type Definitions                          */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Global Variables                                */
/* ------------------------------------------------------------ */

extern BOOL dpmutilfVerbose;

/* ------------------------------------------------------------ */
/*              Local Variables                                 */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Forward Declarations                            */
/* ------------------------------------------------------------ */

int32_t ComputeMultCoefDigitizer(float cg);
int32_t ComputeAddCoefDigitizer(float ca);

/* ------------------------------------------------------------ */
/*              Procedure Definitions                           */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/***    FDisplayZmodDigitizerCal
**
**  Parameters:
**      fdI2cDev        - open file descriptor for underlying I2C device (linux only)
**      addrI2cSlave    - I2C bus address for the slave
**
**  Return Value:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function reads the factory calibration and user calibration
**      areas from the ZmodDigitizer with the specified I2C bus address,
**      computes the multiplicative and additive coefficients, and then
**      displays them using stdout.
*/
BOOL
FDisplayZmodDigitizerCal(int fdI2cDev, BYTE addrI2cSlave) {

    ZMOD_DIGITIZER_CAL    adcal;
    WORD                  cbRead;
    time_t                t;
    struct tm             time;
    char                  szDate[256];
    int                   hz;

    if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrDigitizerFactCalStart, (BYTE*)&adcal, sizeof(ZMOD_DIGITIZER_CAL), &cbRead) ) {
        printf("Error: failed to read ZmodDigitizer factory calibration from 0x%02X\n", addrI2cSlave);
        printf("Error: received %d of %d bytes\n", cbRead, sizeof(ZMOD_DIGITIZER_CAL));
        return fFalse;
    }

    t = (time_t)adcal.date;
    localtime_r(&t, &time);
    if ( 0 != strftime(szDate, sizeof(szDate), "%B %d, %Y at %T", &time) ) {
        printf("\n    Factory Calibration:   %s\n", szDate);
    }

    for (hz = 0; hz < cbDigitizerCalibHzSteps; hz++) {
        float freq = FZmodDigitizerGetFrequencyStepMHz(adcal.hz[hz]);
        printf("    Channel 1 Gain   at %.02f MHz: %f\n", freq, adcal.cal[hz][0][0]);
        printf("    Channel 1 Offset at %.02f MHz: %f\n", freq, adcal.cal[hz][0][1]);
        printf("    Channel 2 Gain   at %.02f MHz: %f\n", freq, adcal.cal[hz][1][0]);
        printf("    Channel 2 Offset at %.02f MHz: %f\n", freq, adcal.cal[hz][1][1]);
    }

    for (hz = 0; hz < cbDigitizerCalibHzSteps; hz++) {
        float freq = FZmodDigitizerGetFrequencyStepMHz(adcal.hz[hz]);
        printf("    Channel 1 Gain   at %.02f MHz (static): 0x%05X\n", freq, (unsigned int)ComputeMultCoefDigitizer(adcal.cal[hz][0][0]));
        printf("    Channel 1 Offset at %.02f MHz (static): 0x%05X\n", freq, (unsigned int)ComputeMultCoefDigitizer(adcal.cal[hz][0][1]));
        printf("    Channel 2 Gain   at %.02f MHz (static): 0x%05X\n", freq, (unsigned int)ComputeMultCoefDigitizer(adcal.cal[hz][1][0]));
        printf("    Channel 2 Offset at %.02f MHz (static): 0x%05X\n", freq, (unsigned int)ComputeMultCoefDigitizer(adcal.cal[hz][1][1]));
    }

    if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrDigitizerUserCalStart, (BYTE*)&adcal, sizeof(ZMOD_DIGITIZER_CAL), &cbRead) ) {
        printf("Error: failed to read ZmodDigitizer user calibration from 0x%02X\n", addrI2cSlave);
        printf("Error: received %d of %d bytes\n", cbRead, sizeof(ZMOD_DIGITIZER_CAL));
        return fFalse;
    }

    t = (time_t)adcal.date;
    localtime_r(&t, &time);
    if ( 0 != strftime(szDate, sizeof(szDate), "%B %d, %Y at %T", &time) ) {
        printf("\n    User Calibration:      %s\n", szDate);
    }

    for (hz = 0; hz < cbDigitizerCalibHzSteps; hz++) {
        float freq = FZmodDigitizerGetFrequencyStepMHz(adcal.hz[hz]);
        printf("    Channel 1 Gain   at %.02f MHz: %f\n", freq, adcal.cal[hz][0][0]);
        printf("    Channel 1 Offset at %.02f MHz: %f\n", freq, adcal.cal[hz][0][1]);
        printf("    Channel 2 Gain   at %.02f MHz: %f\n", freq, adcal.cal[hz][1][0]);
        printf("    Channel 2 Offset at %.02f MHz: %f\n", freq, adcal.cal[hz][1][1]);
    }

    for (hz = 0; hz < cbDigitizerCalibHzSteps; hz++) {
        float freq = FZmodDigitizerGetFrequencyStepMHz(adcal.hz[hz]);
        printf("    Channel 1 Gain   at %.02f MHz (static): 0x%05X\n", freq, (unsigned int)ComputeMultCoefDigitizer(adcal.cal[hz][0][0]));
        printf("    Channel 1 Offset at %.02f MHz (static): 0x%05X\n", freq, (unsigned int)ComputeMultCoefDigitizer(adcal.cal[hz][0][1]));
        printf("    Channel 2 Gain   at %.02f MHz (static): 0x%05X\n", freq, (unsigned int)ComputeMultCoefDigitizer(adcal.cal[hz][1][0]));
        printf("    Channel 2 Offset at %.02f MHz (static): 0x%05X\n", freq, (unsigned int)ComputeMultCoefDigitizer(adcal.cal[hz][1][1]));
    }

    return fTrue;
}

/* ------------------------------------------------------------ */
/***    FGetZmodDigitizerCal
**
**  Parameters:
**      fdI2cDev        - open file descriptor for underlying I2C device (linux only)
**      addrI2cSlave    - I2C bus address for the slave
**      pFactoryCal     - ZMOD_DIGITIZER_CAL object to return factory calibration data through
**      pUserCal        - ZMOD_DIGITIZER_CAL object to return user calibration data through
**
**  Return Value:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function reads the factory calibration and user calibration
**      areas from the ZmodDigitizer with the specified I2C bus address,
**      then returns the calibration data by argument.
*/
BOOL
FGetZmodDigitizerCal(int fdI2cDev, BYTE addrI2cSlave, ZMOD_DIGITIZER_CAL *pFactoryCal, ZMOD_DIGITIZER_CAL *pUserCal) {

    WORD            cbRead;

    if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrDigitizerFactCalStart, (BYTE*)pFactoryCal, sizeof(ZMOD_DIGITIZER_CAL), &cbRead) ) {
        printf("Error: failed to read ZmodDigitizer factory calibration from 0x%02X\n", addrI2cSlave);
        printf("Error: received %d of %d bytes\n", cbRead, sizeof(ZMOD_DIGITIZER_CAL));
        return fFalse;
    }

    if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrDigitizerUserCalStart, (BYTE*)pUserCal, sizeof(ZMOD_DIGITIZER_CAL), &cbRead) ) {
        printf("Error: failed to read ZmodDigitizer user calibration from 0x%02X\n", addrI2cSlave);
        printf("Error: received %d of %d bytes\n", cbRead, sizeof(ZMOD_DIGITIZER_CAL));
        return fFalse;
    }

    return fTrue;
}

/* ------------------------------------------------------------ */
/***    FZmodDigitizerCalConvertToS18
**
**  Parameters:
**      adcal           - ZMOD_DIGITIZER_CAL object to pull calibration coefficients from
**      pReturn         - ZMOD_DIGITIZER_CAL_S18 object to return data by argument
**
**  Return Value:
**      none
**
**  Errors:
**      none
**
**  Description:
**      This function converts calibration coefficients to the 18-bit signed format used in PL hardware.
*/
void
FZmodDigitizerCalConvertToS18(ZMOD_DIGITIZER_CAL adcal, ZMOD_DIGITIZER_CAL_S18 *pReturn) {
    int hz, ch;
    for (hz = 0; hz < cbDigitizerCalibHzSteps; hz++) { // frequency step
        for (ch = 0; ch < 2; ch++) { // channel index
            pReturn->cal[hz][ch][0] = (unsigned int)ComputeMultCoefDigitizer(adcal.cal[hz][ch][0]);
            pReturn->cal[hz][ch][1] = (unsigned int)ComputeAddCoefDigitizer(adcal.cal[hz][ch][1]);
        }
    }
}

/* ------------------------------------------------------------ */
/***    ComputeMultCoefDigitizer
**
**  Parameters:
**      cg              - gain coefficient from ZmodDigitizer DNA memory
**
**  Return Value:
**      signed 32-bit value containing the multiplicative coefficient in the
**      18 least significant bits: bit 17 is the sign, bits 16:0 are the value
**
**  Errors:
**      none
**
**  Description:
**      This function computes a signed 18-bit value corresponding to the
**      multiplicative calibration coefficient of the ZmodDigitizer.
*/
int32_t
ComputeMultCoefDigitizer(float cg) {

    float   fval;
    int32_t ival;

    fval = (DIGITIZER_REAL_RANGE_ADC/DIGITIZER_IDEAL_RANGE_ADC)*(1 + cg)*(float)(1<<16);
    ival = (int32_t)(fval + 0.5);  // round
    ival &= (1<<18) - 1; // keep only 18 bits

    return ival;
}

/* ------------------------------------------------------------ */
/***    ComputeAddCoefDigitizer
**
**  Parameters:
**      ca              - additive coefficient from ZmodDigitizer DNA memory
**
**  Return Value:
**      signed 32-bit value containing the additive coefficient in the 18
**      least significant bits: bit 17 is the sign, bits 16:0 are the value
**
**  Errors:
**      none
**
**  Description:
**      This function computes a signed 18-bit value corresponding to the
**      additive calibration coefficient of the ZmodDigitizer.
*/
int32_t
ComputeAddCoefDigitizer(float ca) {

    float   fval;
    int32_t ival;

    fval = ca / (DIGITIZER_IDEAL_RANGE_ADC)*(float)(1<<17);
    ival = (int32_t)(fval + 0.5);  // round
    ival &= (1<<18) - 1; // keep only 18 bits

    return ival;
}

/* ------------------------------------------------------------ */
/***    FZmodDigitizerGetFrequencyStepMHz
**
**  Parameters:
**      hz              - frequency step value from ZmodDigitizer DNA memory
**
**  Return Value:
**      floating point representation of the actual frequency in MHz represented by the hz byte
**      returns 0.0f if the hz byte is invalid
**
**  Errors:
**      none
**
**  Description:
**      converts the frequency step encoded in Syzygy DNA to its floating point representation
**      7 steps: 0=122.88MHz, 50(MHz), 80(MHz), 100(MHz), 110(MHz), 120(MHz), 125(MHz)
*/
float
FZmodDigitizerGetFrequencyStepMHz(BYTE hz) {
    switch (hz) {
    case 0:   return 122.88f;
    case 50:  return 50.0f;
    case 80:  return 80.0f;
    case 100: return 100.0f;
    case 110: return 110.0f;
    case 120: return 120.0f;
    case 125: return 125.0f;
    default:  return 0.0f;
    }
}

/* ------------------------------------------------------------ */
/***    FZmodIsDigitizer
**
**  Parameters:
**      Pdid            - Product ID read from DNA
**
**  Return Value:
**      Bool, true if the PDID represents a valid Zmod Digitizer ID, false
**      otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function determines whether a given Zmod is a Zmod Digitizer
*/
BOOL
FZmodIsDigitizer(DWORD Pdid) {
	return ((Pdid >> 20) & 0xfff) == prodZmodDigitizer;
}

/* ------------------------------------------------------------ */
/***    FGetZmodDigitizerVariant
**
**  Parameters:
**      Pdid            - Product ID read from DNA
**      pVariant		- ZMOD_DIGITIZER_VARIANT enum to return by argument
**
**  Return Value:
**      Bool, false if unsupported variant, true otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function processes the product ID to determine what variant of the Zmod Digitizer the installed Zmod is
*/
BOOL
FGetZmodDigitizerVariant(DWORD Pdid, ZMOD_DIGITIZER_VARIANT *pVariant) {
	switch ((Pdid >> 8) & 0xfff) {
	case 0x061:
		*pVariant = ZMOD_DIGITIZER_VARIANT_1430_125;
		return fTrue;
	default:
		*pVariant = ZMOD_DIGITIZER_VARIANT_UNSUPPORTED;
		return fFalse;
	}
}

/* ------------------------------------------------------------ */
/***    FGetZmodDigitizerResolution
**
**  Parameters:
**      variant			- Enum identifying the Zmod Digitizer variant, result of FGetZmodDACVariant
**      pResolution		- Pointer to return the ADC resolution by argument
**
**  Return Value:
**      Bool, false if the variant is unsupported, true otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function uses the Zmod Digitizer variant to determine the ADC resolution
*/
BOOL FGetZmodDigitizerResolution(ZMOD_DIGITIZER_VARIANT variant, DWORD *pResolution) {
	switch (variant) {
	case ZMOD_DIGITIZER_VARIANT_1430_125:
		*pResolution = 14;
		return fTrue;
	default:
		return fFalse;
	}
}
