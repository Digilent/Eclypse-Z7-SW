#include "zmod_awg_axi_configuration.h"

void ZmodAwgAxiConfiguration_SetControl(ZmodAwgAxiConfiguration *InstPtr, u32 Control) {
	ZmodAwgAxiConfiguration_WriteReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_CONTROL_REG_OFFSET, Control);
	ZmodAwgAxiConfiguration_IssueApStart(InstPtr);
}

u32 ZmodAwgAxiConfiguration_GetControl(ZmodAwgAxiConfiguration *InstPtr) {
	u32 Control;
	ZmodAwgAxiConfiguration_IssueApStart(InstPtr);
	Control = ZmodAwgAxiConfiguration_ReadReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_CONTROL_REG_OFFSET);
	return Control;
}

u32 ZmodAwgAxiConfiguration_GetStatus(ZmodAwgAxiConfiguration *InstPtr) {
	u32 Status;
	ZmodAwgAxiConfiguration_IssueApStart(InstPtr);
	Status = ZmodAwgAxiConfiguration_ReadReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_STATUS_REG_OFFSET);
	return Status;
}

void ZmodAwgAxiConfiguration_SetCalibration(ZmodAwgAxiConfiguration *InstPtr, ZmodAwg_CalibrationCoefficients Coeff) {
	ZmodAwgAxiConfiguration_WriteReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_EXTCH1HGMULTCOEF_REG_OFFSET, Coeff.Channel1_HighGainMultiplicative);
	ZmodAwgAxiConfiguration_WriteReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_EXTCH1HGADDCOEF_REG_OFFSET,  Coeff.Channel1_HighGainAdditive);
	ZmodAwgAxiConfiguration_WriteReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_EXTCH1LGMULTCOEF_REG_OFFSET, Coeff.Channel1_LowGainMultiplicative);
	ZmodAwgAxiConfiguration_WriteReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_EXTCH1LGADDCOEF_REG_OFFSET,  Coeff.Channel1_LowGainAdditive);
	ZmodAwgAxiConfiguration_WriteReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_EXTCH2HGMULTCOEF_REG_OFFSET, Coeff.Channel2_HighGainMultiplicative);
	ZmodAwgAxiConfiguration_WriteReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_EXTCH2HGADDCOEF_REG_OFFSET,  Coeff.Channel2_HighGainAdditive);
	ZmodAwgAxiConfiguration_WriteReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_EXTCH2LGMULTCOEF_REG_OFFSET, Coeff.Channel2_LowGainMultiplicative);
	ZmodAwgAxiConfiguration_WriteReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_EXTCH2LGADDCOEF_REG_OFFSET,  Coeff.Channel2_LowGainAdditive);
	ZmodAwgAxiConfiguration_IssueApStart(InstPtr);
}
