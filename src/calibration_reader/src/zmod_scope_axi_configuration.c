#include "zmod_scope_axi_configuration.h"
#include "xil_printf.h"

/****************************************************************************/
/**
* Send an AP start transaction to the HLS engine responsible for transferring data between AXI configuration module ports and the AXI4-lite interface,
* then wait for the HLS engine to return to Idle state to ensure that the transaction has completed
*
* @param	InstPtr - Driver handler instance to operate on
*
* @return	none
*
*****************************************************************************/
void IssueApStart (ZmodScope *InstPtr) {
	// Send the new stuff to hardware
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CONTROL_REG_OFFSET, AXI_ZMOD_SCOPE_CONTROL_AP_START_MASK);

	u32 Ctrl;
	do {
		Ctrl = ZmodScope_ReadReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CONTROL_REG_OFFSET);
	} while (!(AXI_ZMOD_SCOPE_CONTROL_AP_IDLE_MASK & Ctrl));
}

/****************************************************************************/
/**
* Initialize the HLS engine and configure the Zmod Scope Lowlevel IP with default calibration coefficients such that even
* if not in TestMode, the calibration DSP will act as a passthrough.
*
* @param	InstPtr - Driver handler instance to operate on
* @param	BaseAddr - The base address of the Zmod Scope AXI Configuration IP's register space
*
* @return	none
*
*****************************************************************************/
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

/****************************************************************************/
/**
* Controls the Zmod Scope's gain and coupling select relays by way of the config register of the Zmod Scope AXI Configuration IP
*
* @param	InstPtr - Driver handler instance to operate on
* @param	Config - The new value of the config register. Any of these, or-ed together:
*						AXI_ZMOD_SCOPE_CONFIG_CHANNEL_1_GAIN_SELECT_MASK (0 - low gain; 1 - high gain)
*  						AXI_ZMOD_SCOPE_CONFIG_CHANNEL_2_GAIN_SELECT_MASK (0 - low gain; 1 - high gain)
*  						AXI_ZMOD_SCOPE_CONFIG_CHANNEL_1_COUPLING_SELECT_MASK (0 - AC coupling; 1 - DC coupling)
*  						AXI_ZMOD_SCOPE_CONFIG_CHANNEL_2_COUPLING_SELECT_MASK (0 - AC coupling; 1 - DC coupling)
*  						AXI_ZMOD_SCOPE_CONFIG_TEST_MODE_MASK (0 - calibration DSP is performed on acquired data; 1 - raw data from the ADC is acquired)
*
* @return	none
*
*****************************************************************************/
void ZmodScope_SetConfig(ZmodScope *InstPtr, u32 Config) {
	u32 OldConfig;
	OldConfig = ZmodScope_ReadReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CONFIG_REG_OFFSET);
	OldConfig &= ~AXI_ZMOD_SCOPE_CONFIG_ALL_SOFTWARE_BITS;
	Config &= AXI_ZMOD_SCOPE_CONFIG_ALL_SOFTWARE_BITS;
	Config |= OldConfig;
	ZmodScope_WriteReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CONFIG_REG_OFFSET, Config);
	IssueApStart(InstPtr);
}

/****************************************************************************/
/**
* Gets the current state of the config register
*
* @param	InstPtr - Driver handler instance to operate on
*
* @return	The contents of the config register: Any of these, or-ed together:
*						AXI_ZMOD_SCOPE_CONFIG_CHANNEL_1_GAIN_SELECT_MASK (0 - low gain; 1 - high gain)
*  						AXI_ZMOD_SCOPE_CONFIG_CHANNEL_2_GAIN_SELECT_MASK (0 - low gain; 1 - high gain)
*  						AXI_ZMOD_SCOPE_CONFIG_CHANNEL_1_COUPLING_SELECT_MASK (0 - AC coupling; 1 - DC coupling)
*  						AXI_ZMOD_SCOPE_CONFIG_CHANNEL_2_COUPLING_SELECT_MASK (0 - AC coupling; 1 - DC coupling)
*  						AXI_ZMOD_SCOPE_CONFIG_TEST_MODE_MASK (0 - calibration DSP is performed on acquired data; 1 - raw data from the ADC is acquired)
*
*****************************************************************************/
u32 ZmodScope_GetConfig(ZmodScope *InstPtr) {
	u32 Config;
	IssueApStart(InstPtr);
	Config = ZmodScope_ReadReg(InstPtr->BaseAddr, AXI_ZMOD_SCOPE_CONFIG_REG_OFFSET);
	return Config & AXI_ZMOD_SCOPE_CONFIG_ALL_SOFTWARE_BITS;
}

/****************************************************************************/
/**
* Clears the Lowlevel IP reset bit, dhecks the status of the Zmod Scope lowlevel IP, then enables the Zmod Scope's AXI4-stream interface.
*
* @param	InstPtr - Driver handler instance to operate on
*
* @return	none
*
* @note: FIXME: remove print statements and return error codes
*
*****************************************************************************/
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

/****************************************************************************/
/**
* Sets the calibration coefficient inputs to the Zmod Scope Lowlevel IP.
*
* @param	InstPtr - Driver handler instance to operate on
* @param	CalibrationCoefficients - Calibration coefficients to apply
*
* @return	none
*
*****************************************************************************/
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
