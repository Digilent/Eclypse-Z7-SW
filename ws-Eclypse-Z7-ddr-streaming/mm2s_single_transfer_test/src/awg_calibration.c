#include "awg_calibration.h"
#include "xstatus.h"
#include "dpmutil/dpmutil.h"
#include "dpmutil/ZmodDAC.h"

// FIXME define should be in some platform header, not here
#define ECLYPSE_NUM_ZMOD_PORTS 2

/****************************************************************************/
/**
* Reads calibration coefficients from the Zmod AWG SYZYGY DNA via I2C
*
* @param	ZmodPortVioGroup - 0 for Zmod Port A, 1 for Zmod Port B
* @param	FactoryCoefficients - returns the factory calibration coefficents for the specified Zmod AWG by argument
* @param	FactoryCoefficients - returns the user calibration coefficents for the specified Zmod AWG by argument
*
* @return	If the specified port does not have a Zmod DAC/AWG populated, returns an error
*
*****************************************************************************/
XStatus ZmodAwg_ReadCoefficientsFromDna(u32 ZmodPortVioGroup, ZmodAwg_CalibrationCoefficients *FactoryCoefficients, ZmodAwg_CalibrationCoefficients *UserCoefficients) {
	dpmutilPortInfo_t PortInfo[ECLYPSE_NUM_ZMOD_PORTS] = {0};
	XStatus Status;
	u32 iPort;
	DWORD Pdid;
	ZMOD_DAC_CAL FactoryCal = {0};
	ZMOD_DAC_CAL UserCal = {0};

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

	// Read the Pdid and use it to determine whether the Zmod is a Zmod AWG
	FZmodReadPdid(_unused_, PortInfo[iPort].i2cAddr, &Pdid);

	if (!FZmodIsDAC(Pdid)) {
		return XST_FAILURE;
	}

	// Read the calibration coefficients
	FGetZmodDACCal(_unused_, PortInfo[iPort].i2cAddr, &FactoryCal, &UserCal);

	ZMOD_DAC_CAL_S18 Coeffs;

	// Compute raw calibration coefficients and move them into the return arguments
	FZmodDACCalConvertToS18(FactoryCal, &Coeffs);
	FactoryCoefficients->Channel1_LowGainMultiplicative		= Coeffs.cal[0][0][0];
	FactoryCoefficients->Channel1_LowGainAdditive			= Coeffs.cal[0][0][1];
	FactoryCoefficients->Channel1_HighGainMultiplicative 	= Coeffs.cal[0][1][0];
	FactoryCoefficients->Channel1_HighGainAdditive 			= Coeffs.cal[0][1][1];
	FactoryCoefficients->Channel2_LowGainMultiplicative		= Coeffs.cal[1][0][0];
	FactoryCoefficients->Channel2_LowGainAdditive		 	= Coeffs.cal[1][0][1];
	FactoryCoefficients->Channel2_HighGainMultiplicative 	= Coeffs.cal[1][1][0];
	FactoryCoefficients->Channel2_HighGainAdditive		 	= Coeffs.cal[1][1][1];

	FZmodDACCalConvertToS18(UserCal, &Coeffs);
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
