/************************************************************************/
/*                                                                      */
/*  ZmodADC.c - ZmodADC functions implementation                        */
/*                                                                      */
/************************************************************************/
/*  Author: Michael T. Alexander                                        */
/*  Copyright 2020, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  This source file contains the implementation of functions that can  */
/*  be used to calculate and display the calibration constants          */
/*  associated with a Zmod ADC.                                         */
/*                                                                      */
/*  Note: the code for calculating the coefficients was provided by     */
/*  Tudor Gherman.                                                      */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  01/13/2020 (MichaelA): created                                      */
/*  01/14/2020 (MichaelA): modified the way that coefficients are       */
/*      displayed based on feedback                                     */
/*  01/15/2020 (MichaelA): modified FDisplayZmodADCCal to display the   */
/*      raw calibration constants as retrieved from flash               */
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
#include "../dpmutil/ZmodADC.h"

/* ------------------------------------------------------------ */
/*              Miscellaneous Declarations                      */
/* ------------------------------------------------------------ */

#define ADC1410_IDEAL_RANGE_ADC_HIGH    1.0
#define ADC1410_IDEAL_RANGE_ADC_LOW     25.0
#define ADC1410_REAL_RANGE_ADC_HIGH     1.086
#define ADC1410_REAL_RANGE_ADC_LOW      26.25

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

int32_t ComputeMultCoefADC1410(float cg, BOOL fHighGain);
int32_t ComputeAddCoefADC1410(float ca, BOOL fHighGain);

/* ------------------------------------------------------------ */
/*              Procedure Definitions                           */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/***    FDisplayZmodADCCal
**
**  Parameters:
**  	fdI2cDev        - open file descriptor for underlying I2C device (linux only)
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
**      areas from the ZmodADC with the specified I2C bus address,
**      computes the multiplicative and additive coefficients, and then
**      displays them using stdout.
*/
BOOL
FDisplayZmodADCCal(int fdI2cDev, BYTE addrI2cSlave) {

    ZMOD_ADC_CAL    adcal;
    WORD            cbRead;
    time_t          t;
    struct tm       time;
    char            szDate[256];

    if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrAdcFactCalStart, (BYTE*)&adcal, sizeof(ZMOD_ADC_CAL), &cbRead) ) {
        printf("Error: failed to read ZmodADC factory calibration from 0x%02X\n", addrI2cSlave);
        printf("Error: received %d of %d bytes\n", cbRead, sizeof(ZMOD_ADC_CAL));
        return fFalse;
    }

    t = (time_t)adcal.date;
    localtime_r(&t, &time);
    if ( 0 != strftime(szDate, sizeof(szDate), "%B %d, %Y at %T", &time) ) {
        printf("\n    Factory Calibration:   %s\n", szDate);
    }

    printf("    CHAN_1_LG_GAIN:        %f\n", adcal.cal[0][0][0]);
    printf("    CHAN_1_LG_OFFSET:      %f\n", adcal.cal[0][0][1]);
    printf("    CHAN_1_HG_GAIN:        %f\n", adcal.cal[0][1][0]);
    printf("    CHAN_1_HG_OFFSET:      %f\n", adcal.cal[0][1][1]);
    printf("    CHAN_2_LG_GAIN:        %f\n", adcal.cal[1][0][0]);
    printf("    CHAN_2_LG_OFFSET:      %f\n", adcal.cal[1][0][1]);
    printf("    CHAN_2_HG_GAIN:        %f\n", adcal.cal[1][1][0]);
    printf("    CHAN_2_HG_OFFSET:      %f\n", adcal.cal[1][1][1]);

    printf("    Ch1LgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[0][0][0], fFalse));
    printf("    Ch1LgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[0][0][1], fFalse));
    printf("    Ch1HgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[0][1][0], fTrue));
    printf("    Ch1HgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[0][1][1], fTrue));
    printf("    Ch2LgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[1][0][0], fFalse));
    printf("    Ch2LgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[1][0][1], fFalse));
    printf("    Ch2HgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[1][1][0], fTrue));
    printf("    Ch2HgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[1][1][1], fTrue));

    if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrAdcUserCalStart, (BYTE*)&adcal, sizeof(ZMOD_ADC_CAL), &cbRead) ) {
        printf("Error: failed to read ZmodADC user calibration from 0x%02X\n", addrI2cSlave);
        printf("Error: received %d of %d bytes\n", cbRead, sizeof(ZMOD_ADC_CAL));
        return fFalse;
    }

    t = (time_t)adcal.date;
    localtime_r(&t, &time);
    if ( 0 != strftime(szDate, sizeof(szDate), "%B %d, %Y at %T", &time) ) {
        printf("\n    User Calibration:      %s\n", szDate);
    }

    printf("    CHAN_1_LG_GAIN:        %f\n", adcal.cal[0][0][0]);
    printf("    CHAN_1_LG_OFFSET:      %f\n", adcal.cal[0][0][1]);
    printf("    CHAN_1_HG_GAIN:        %f\n", adcal.cal[0][1][0]);
    printf("    CHAN_1_HG_OFFSET:      %f\n", adcal.cal[0][1][1]);
    printf("    CHAN_2_LG_GAIN:        %f\n", adcal.cal[1][0][0]);
    printf("    CHAN_2_LG_OFFSET:      %f\n", adcal.cal[1][0][1]);
    printf("    CHAN_2_HG_GAIN:        %f\n", adcal.cal[1][1][0]);
    printf("    CHAN_2_HG_OFFSET:      %f\n", adcal.cal[1][1][1]);

    printf("    Ch1LgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[0][0][0], fFalse));
    printf("    Ch1LgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[0][0][1], fFalse));
    printf("    Ch1HgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[0][1][0], fTrue));
    printf("    Ch1HgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[0][1][1], fTrue));
    printf("    Ch2LgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[1][0][0], fFalse));
    printf("    Ch2LgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[1][0][1], fFalse));
    printf("    Ch2HgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[1][1][0], fTrue));
    printf("    Ch2HgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[1][1][1], fTrue));

    return fTrue;
}

/* ------------------------------------------------------------ */
/***    FGetZmodADCCal
**
**  Parameters:
**  	fdI2cDev        - open file descriptor for underlying I2C device (linux only)
**      addrI2cSlave    - I2C bus address for the slave
**      pFactoryCal			- ZMOD_ADC_CAL object to return factory calibration data through
**      pUserCal			- ZMOD_ADC_CAL object to return user calibration data through
**
**  Return Value:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function reads the factory calibration and user calibration
**      areas from the ZmodADC with the specified I2C bus address,
**      then returns the calibration data by argument.
*/
BOOL
FGetZmodADCCal(int fdI2cDev, BYTE addrI2cSlave, ZMOD_ADC_CAL *pFactoryCal, ZMOD_ADC_CAL *pUserCal) {

    WORD            cbRead;

    if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrAdcFactCalStart, (BYTE*)pFactoryCal, sizeof(ZMOD_ADC_CAL), &cbRead) ) {
        printf("Error: failed to read ZmodADC factory calibration from 0x%02X\n", addrI2cSlave);
        printf("Error: received %d of %d bytes\n", cbRead, sizeof(ZMOD_ADC_CAL));
        return fFalse;
    }

    if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrAdcUserCalStart, (BYTE*)pUserCal, sizeof(ZMOD_ADC_CAL), &cbRead) ) {
        printf("Error: failed to read ZmodADC user calibration from 0x%02X\n", addrI2cSlave);
        printf("Error: received %d of %d bytes\n", cbRead, sizeof(ZMOD_ADC_CAL));
        return fFalse;
    }

    return fTrue;
}

/* ------------------------------------------------------------ */
/***    FZmodADCCalConvertToS18
**
**  Parameters:
**      adcal			- ZMOD_ADC_CAL object to pull calibration coefficients from
**      pReturn			- ZMOD_ADC_CAL_S18 object to return data by argument
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
FZmodADCCalConvertToS18(ZMOD_ADC_CAL adcal, ZMOD_ADC_CAL_S18 *pReturn) {
	pReturn->cal[0][0][0] = (unsigned int)ComputeMultCoefADC1410(adcal.cal[0][0][0], fFalse);
	pReturn->cal[0][0][1] = (unsigned int)ComputeAddCoefADC1410(adcal.cal[0][0][1], fFalse);
	pReturn->cal[0][1][0] = (unsigned int)ComputeMultCoefADC1410(adcal.cal[0][1][0], fTrue);
	pReturn->cal[0][1][1] = (unsigned int)ComputeAddCoefADC1410(adcal.cal[0][1][1], fTrue);
	pReturn->cal[1][0][0] = (unsigned int)ComputeMultCoefADC1410(adcal.cal[1][0][0], fFalse);
	pReturn->cal[1][0][1] = (unsigned int)ComputeAddCoefADC1410(adcal.cal[1][0][1], fFalse);
	pReturn->cal[1][1][0] = (unsigned int)ComputeMultCoefADC1410(adcal.cal[1][1][0], fTrue);
	pReturn->cal[1][1][1] = (unsigned int)ComputeAddCoefADC1410(adcal.cal[1][1][1], fTrue);
}

/* ------------------------------------------------------------ */
/***    ComputeMultCoefADC1410
**
**  Parameters:
**      cg              - gain coefficient from ZmodADC flash memory
**      fHighGain       - fTrue for high gain setting, fFalse for low gain
**                        read
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
**      multiplicative calibration coefficient of the ZmodADC.
*/
int32_t
ComputeMultCoefADC1410(float cg, BOOL fHighGain) {

    float   fval;
    int32_t ival;

    fval = (fHighGain ? (ADC1410_REAL_RANGE_ADC_HIGH/ADC1410_IDEAL_RANGE_ADC_HIGH):(ADC1410_REAL_RANGE_ADC_LOW/ADC1410_IDEAL_RANGE_ADC_LOW))*(1 + cg)*(float)(1<<16);
    ival = (int32_t)(fval + 0.5);  // round
    ival &= (1<<18) - 1; // keep only 18 bits

    return ival;
}

/* ------------------------------------------------------------ */
/***    ComputeAddCoefADC1410
**
**  Parameters:
**      ca              - additive coefficient from ZmodADC flash memory
**      fHighGain       - fTrue for high gain setting, fFalse for low gain
**                        read
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
**      additive calibration coefficient of the ZmodADC.
*/
int32_t
ComputeAddCoefADC1410(float ca, BOOL fHighGain) {

    float   fval;
    int32_t ival;

    fval = ca / (fHighGain ? ADC1410_IDEAL_RANGE_ADC_HIGH:ADC1410_IDEAL_RANGE_ADC_LOW)*(float)(1<<17);
    ival = (int32_t)(fval + 0.5);  // round
    ival &= (1<<18) - 1; // keep only 18 bits

    return ival;
}

/* ------------------------------------------------------------ */
/***    FZmodIsADC
**
**  Parameters:
**      Pdid            - Product ID read from DNA
**
**  Return Value:
**      Bool, true if the PDID represents a valid Zmod ADC ID, false
**      otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function determines whether a given Zmod is a Zmod ADC
*/
BOOL
FZmodIsADC(DWORD Pdid) {
	return ((Pdid >> 20) & 0xfff) == prodZmodADC;
}

/* ------------------------------------------------------------ */
/***    FGetZmodADCVariant
**
**  Parameters:
**      Pdid            - Product ID read from DNA
**      pVariant		- ZMOD_ADC_VARIANT enum to return by argument
**
**  Return Value:
**      Vool, false if unsupported variant, true otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function processes the product ID to determine what variant of the Zmod ADC the installed Zmod is
*/
BOOL
FGetZmodADCVariant(DWORD Pdid, ZMOD_ADC_VARIANT *pVariant) {
	switch ((Pdid >> 8) & 0xfff) {
	case 0x002:
		*pVariant = ZMOD_ADC_VARIANT_1410_105;
		return fTrue;
	case 0x012:
		*pVariant = ZMOD_ADC_VARIANT_1010_40;
		return fTrue;
	case 0x022:
		*pVariant = ZMOD_ADC_VARIANT_1210_40;
		return fTrue;
	case 0x032:
		*pVariant = ZMOD_ADC_VARIANT_1410_40;
		return fTrue;
	case 0x042:
		*pVariant = ZMOD_ADC_VARIANT_1010_125;
		return fTrue;
	case 0x052:
		*pVariant = ZMOD_ADC_VARIANT_1210_125;
		return fTrue;
	case 0x062:
		*pVariant = ZMOD_ADC_VARIANT_1410_125;
		return fTrue;
	default:
		*pVariant = ZMOD_ADC_VARIANT_UNSUPPORTED;
		return fFalse;
	}
}

/* ------------------------------------------------------------ */
/***    FGetZmodADCResolution
**
**  Parameters:
**      variant			- Enum identifying the Zmod ADC variant, result of FGetZmodADCVariant
**      pResolution		- Pointer to return the ADC resolution by argument
**
**  Return Value:
**      Bool, false if the variant is unsupported, true otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function uses the Zmod ADC variant to determine the ADC resolution
*/
BOOL FGetZmodADCResolution(ZMOD_ADC_VARIANT variant, DWORD *pResolution) {
	switch (variant) {
	case ZMOD_ADC_VARIANT_1410_105:
	case ZMOD_ADC_VARIANT_1410_40:
	case ZMOD_ADC_VARIANT_1410_125:
		*pResolution = 14;
		return fTrue;
	case ZMOD_ADC_VARIANT_1210_40:
	case ZMOD_ADC_VARIANT_1210_125:
		*pResolution = 12;
		return fTrue;
	case ZMOD_ADC_VARIANT_1010_40:
	case ZMOD_ADC_VARIANT_1010_125:
		*pResolution = 10;
		return fTrue;
	case ZMOD_ADC_VARIANT_UNSUPPORTED:
		return fFalse;
    default:
        return fFalse;
	}
}
