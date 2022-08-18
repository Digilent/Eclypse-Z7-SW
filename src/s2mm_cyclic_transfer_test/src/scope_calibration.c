#include "scope_calibration.h"
#include "dpmutil/dpmutil.h"
#include "dpmutil/ZmodADC.h"

// FIXME define should be in some platform header, not here
#define ECLYPSE_NUM_ZMOD_PORTS 2


/****************************************************************************/
/**
* Checks if a Zmod is actually a Zmod Scope
*
* @param	InstPtr is the device handler instance for the ManualTrigger IP.
* @return	True if the Zmod product model indicates that it is a Zmod ADC, false otherwise
*
*****************************************************************************/
BOOL ZmodIsScope (SzgDnaStrings DnaStrings) {
	if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1410-40") == 0) {
		return fTrue;
	}
	if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1410-105") == 0) {
		return fTrue;
	}
	if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1410-125") == 0) {
		return fTrue;
	}
	if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1210-40") == 0) {
		return fTrue;
	}
	if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1210-125") == 0) {
		return fTrue;
	}
	if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1010-40") == 0) {
		return fTrue;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1010-125") == 0) {
		return fTrue;
	}

	return fFalse;
}

/****************************************************************************/
/**
* Reads calibration coefficients from the Zmod Scope SYZYGY DNA via I2C
*
* @param	ZmodPortVioGroup - 0 for Zmod Port A, 1 for Zmod Port B
* @param	FactoryCoefficients - returns the factory calibration coefficents for the specified Zmod Scope by argument
* @param	FactoryCoefficients - returns the user calibration coefficents for the specified Zmod Scope by argument
*
* @return	If the specified port does not have a Zmod ADC/Scope populated, returns an XST_FAILURE, otherwise returns XST_SUCCESS
*
*****************************************************************************/
XStatus ZmodScope_ReadCoefficientsFromDna(u32 ZmodPortVioGroup, ZmodScope_CalibrationCoefficients *FactoryCoefficients, ZmodScope_CalibrationCoefficients *UserCoefficients) {
	dpmutilPortInfo_t PortInfo[ECLYPSE_NUM_ZMOD_PORTS] = {0};
	XStatus Status;
	u32 iPort;
	SzgDnaHeader DnaHeader;
	SzgDnaStrings DnaStrings = {0};
	ZMOD_ADC_CAL FactoryCal = {0};
	ZMOD_ADC_CAL UserCal = {0};

	const u32 _unused_ = 0; // baremetal dpmutil doesn't use a linux file descriptor

	// enumerate the Syzygy ports to figure out which have Zmods installed
	if (dpmutilFEnum(FALSE, FALSE, PortInfo) == fFalse) {
		return XST_FAILURE;
	}

	for (iPort = 0; iPort < ECLYPSE_NUM_ZMOD_PORTS; iPort++) {
		// Check if a zmod is populated on that port
		if (PortInfo[iPort].portSts.fPresent == 0) {
			continue;
		}

		// The Zmod port was found, break the loop before incrementing the index
		if (PortInfo[iPort].groupVio == ZmodPortVioGroup) {
			break;
		}
	}

	// The port we want was not populated with a Zmod
	if (iPort == ECLYPSE_NUM_ZMOD_PORTS) {
		return XST_FAILURE;
	}

	// Read the standard DNA information
	SyzygyReadDNAHeader(_unused_, PortInfo[iPort].i2cAddr, &DnaHeader, FALSE);
	SyzygyReadDNAStrings(_unused_, PortInfo[iPort].i2cAddr, &DnaHeader, &DnaStrings);

	if (!ZmodIsScope(DnaStrings)) {
		return XST_FAILURE;
	}

	// Read the calibration coefficients
	FGetZmodADCCal(_unused_, PortInfo[iPort].i2cAddr, &FactoryCal, &UserCal);

	ZMOD_ADC_CAL_S18 Coeffs;

	// Compute raw calibration coefficients and move them into the return arguments
	FZmodADCCalConvertToS18(FactoryCal, &Coeffs);
	FactoryCoefficients->Channel1_LowGainMultiplicative		= Coeffs.cal[0][0][0];
	FactoryCoefficients->Channel1_LowGainAdditive			= Coeffs.cal[0][0][1];
	FactoryCoefficients->Channel1_HighGainMultiplicative 	= Coeffs.cal[0][1][0];
	FactoryCoefficients->Channel1_HighGainAdditive 			= Coeffs.cal[0][1][1];
	FactoryCoefficients->Channel2_LowGainMultiplicative		= Coeffs.cal[1][0][0];
	FactoryCoefficients->Channel2_LowGainAdditive		 	= Coeffs.cal[1][0][1];
	FactoryCoefficients->Channel2_HighGainMultiplicative 	= Coeffs.cal[1][1][0];
	FactoryCoefficients->Channel2_HighGainAdditive		 	= Coeffs.cal[1][1][1];

	FZmodADCCalConvertToS18(UserCal, &Coeffs);
	UserCoefficients->Channel1_LowGainMultiplicative	= Coeffs.cal[0][0][0];
	UserCoefficients->Channel1_LowGainAdditive			= Coeffs.cal[0][0][1];
	UserCoefficients->Channel1_HighGainMultiplicative 	= Coeffs.cal[0][1][0];
	UserCoefficients->Channel1_HighGainAdditive 		= Coeffs.cal[0][1][1];
	UserCoefficients->Channel2_LowGainMultiplicative	= Coeffs.cal[1][0][0];
	UserCoefficients->Channel2_LowGainAdditive		 	= Coeffs.cal[1][0][1];
	UserCoefficients->Channel2_HighGainMultiplicative 	= Coeffs.cal[1][1][0];
	UserCoefficients->Channel2_HighGainAdditive		 	= Coeffs.cal[1][1][1];

	return XST_SUCCESS;
}
