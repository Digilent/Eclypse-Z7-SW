#include "zmod_scope_axi_configuration.h"
#include "xil_printf.h"

void IssueApStart (ZmodScope *InstPtr) {
	// Send the new stuff to hardware
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CONTROL_REG_OFFSET, AXI_ZMOD_SCOPE_CONTROL_AP_START_MASK);

	u32 Ctrl;
	do {
		Ctrl = ZmodScope_ReadReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CONTROL_REG_OFFSET);
	} while (!(AXI_ZMOD_SCOPE_CONTROL_AP_IDLE_MASK & Ctrl));
}

void ZmodScope_Initialize (ZmodScope *InstPtr, u32 BaseAddr) {
	const u32 DefaultMultCoeff = 1 << AXI_ZMOD_SCOPE_MULT_COEFF_FIXED_POINT_ONES_BIT;

	InstPtr->BaseAddr = BaseAddr;

	// Enable interrupts
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_GIE_REG_OFFSET, AXI_ZMOD_SCOPE_GIE_ENABLE_MASK);
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_IP_INTR_EN_REG_OFFSET, AXI_ZMOD_SCOPE_IP_INTR_EN_ALL_MASK);

	IssueApStart(InstPtr);

	// Set default coefficients
	ZmodScope_CalibrationCoefficients DefaultCoeffs;
	DefaultCoeffs.Channel1_HighGainMultiplicative = DefaultMultCoeff;
	DefaultCoeffs.Channel1_LowGainMultiplicative = DefaultMultCoeff;
	DefaultCoeffs.Channel1_HighGainAdditive = 0;
	DefaultCoeffs.Channel1_LowGainAdditive = 0;
	DefaultCoeffs.Channel2_HighGainMultiplicative = DefaultMultCoeff;
	DefaultCoeffs.Channel2_LowGainMultiplicative = DefaultMultCoeff;
	DefaultCoeffs.Channel2_HighGainAdditive = 0;
	DefaultCoeffs.Channel2_LowGainAdditive = 0;
	ZmodScope_SetCalibrationCoefficients(InstPtr, DefaultCoeffs);
}

// provide software access to only these:
//  AXI_ZMOD_SCOPE_CONFIG_CHANNEL_1_GAIN_SELECT_MASK
//  AXI_ZMOD_SCOPE_CONFIG_CHANNEL_2_GAIN_SELECT_MASK
//  AXI_ZMOD_SCOPE_CONFIG_CHANNEL_1_COUPLING_SELECT_MASK
//  AXI_ZMOD_SCOPE_CONFIG_CHANNEL_2_COUPLING_SELECT_MASK
//  AXI_ZMOD_SCOPE_CONFIG_TEST_MODE_MASK
// preserve state of all others
void ZmodScope_SetConfig(ZmodScope *InstPtr, u32 Config) {
	u32 OldConfig;
	OldConfig = ZmodScope_ReadReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CONFIG_REG_OFFSET);
	OldConfig &= ~AXI_ZMOD_SCOPE_CONFIG_ALL_SOFTWARE_BITS;
	Config &= AXI_ZMOD_SCOPE_CONFIG_ALL_SOFTWARE_BITS;
	Config |= OldConfig;
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CONFIG_REG_OFFSET, Config);
	IssueApStart(InstPtr);
}

u32 ZmodScope_GetConfig(ZmodScope *InstPtr) {
	u32 Config;
	Config = ZmodScope_ReadReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CONFIG_REG_OFFSET);
	return Config & AXI_ZMOD_SCOPE_CONFIG_ALL_SOFTWARE_BITS;
}

void ZmodScope_DebugStatus(ZmodScope *InstPtr) {
	u32 Status = 0;
	Status = ZmodScope_ReadReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_STATUS_REG_OFFSET);
	if (Status & AXI_ZMOD_SCOPE_STATUS_RESET_BUSY_MASK) {
		xil_printf("Reset busy\r\n");
	}

	if (Status & AXI_ZMOD_SCOPE_STATUS_INIT_DONE_ADC_MASK) {
		xil_printf("ADC initialization done\r\n");
	}

	if (Status & AXI_ZMOD_SCOPE_STATUS_INIT_DONE_RELAY_MASK) {
		xil_printf("Relay initialization done\r\n");
	}

	if (Status & AXI_ZMOD_SCOPE_STATUS_CONFIG_ERROR_MASK) {
		xil_printf("IP configuration error\r\n");
	}

	if (Status & AXI_ZMOD_SCOPE_STATUS_DATA_OVERFLOW_MASK) {
		xil_printf("Data overflow error\r\n");
	}
}

void ZmodScope_StartStream (ZmodScope *InstPtr) {
	u32 Status = 0;
	u32 Config = 0;

	// Deassert the reset bit
	Config = ZmodScope_ReadReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CONFIG_REG_OFFSET);
	Config |= AXI_ZMOD_SCOPE_CONFIG_RESETN_MASK;
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CONFIG_REG_OFFSET, Config);

	// Send the update to hardware, (and sync the inputs?)
	IssueApStart(InstPtr);

	Status = ZmodScope_ReadReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_STATUS_REG_OFFSET);
	if (Status & AXI_ZMOD_SCOPE_STATUS_RESET_BUSY_MASK) {
		xil_printf("Reset busy\r\n");
	}

	if (Status & AXI_ZMOD_SCOPE_STATUS_INIT_DONE_ADC_MASK) {
		xil_printf("ADC initialization done\r\n");
	}

	if (Status & AXI_ZMOD_SCOPE_STATUS_INIT_DONE_RELAY_MASK) {
		xil_printf("Relay initialization done\r\n");
	}

	if (Status & AXI_ZMOD_SCOPE_STATUS_CONFIG_ERROR_MASK) {
		xil_printf("IP configuration error\r\n"); // check that the right zmod is plugged in
	}

	if (Status & AXI_ZMOD_SCOPE_STATUS_DATA_OVERFLOW_MASK) {
		xil_printf("Data overflow error\r\n");
	}

	// Set the enable acquisition bit
	Config = ZmodScope_ReadReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CONFIG_REG_OFFSET);
	Config |= AXI_ZMOD_SCOPE_CONFIG_ENABLE_ACQUISITION_MASK;
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CONFIG_REG_OFFSET, Config);

	/* Send the new reg values to hardware */
	IssueApStart(InstPtr);
}


void ZmodScope_SetCalibrationCoefficients(ZmodScope *InstPtr, ZmodScope_CalibrationCoefficients CalibrationCoefficients) {
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CHANNEL_1_HIGH_GAIN_MULT_COEFF_REG_OFFSET, CalibrationCoefficients.Channel1_HighGainMultiplicative);
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CHANNEL_1_HIGH_GAIN_ADD_COEFF_REG_OFFSET,  CalibrationCoefficients.Channel1_HighGainAdditive);
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CHANNEL_1_LOW_GAIN_MULT_COEFF_REG_OFFSET,  CalibrationCoefficients.Channel1_LowGainMultiplicative);
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CHANNEL_1_LOW_GAIN_ADD_COEFF_REG_OFFSET,   CalibrationCoefficients.Channel1_LowGainAdditive);
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CHANNEL_2_HIGH_GAIN_MULT_COEFF_REG_OFFSET, CalibrationCoefficients.Channel2_HighGainMultiplicative);
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CHANNEL_2_HIGH_GAIN_ADD_COEFF_REG_OFFSET,  CalibrationCoefficients.Channel2_HighGainAdditive);
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CHANNEL_2_LOW_GAIN_MULT_COEFF_REG_OFFSET,  CalibrationCoefficients.Channel2_LowGainMultiplicative);
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CHANNEL_2_LOW_GAIN_ADD_COEFF_REG_OFFSET,   CalibrationCoefficients.Channel2_LowGainAdditive);

	/* Send the new reg values to hardware */
	IssueApStart(InstPtr);
}
