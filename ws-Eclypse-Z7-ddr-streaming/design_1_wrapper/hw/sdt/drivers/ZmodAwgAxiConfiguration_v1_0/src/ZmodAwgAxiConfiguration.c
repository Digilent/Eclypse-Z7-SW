#include "ZmodAwgAxiConfiguration.h"
void ZmodAwgAxiConfiguration_IssueApStart (ZmodAwgAxiConfiguration *InstPtr) {
	// Send the new stuff to hardware
	ZmodAwgAxiConfiguration_WriteReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_AP_CTRL_REG_OFFSET, ZMOD_AWG_AXI_CONFIGURATION_AP_CTRL_START_MASK);

	// Wait until ap_done interrupt goes high
	u32 Ctrl;
	do {
		Ctrl = ZmodAwgAxiConfiguration_ReadReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_IP_INTR_STS_REG_OFFSET);
	} while (!(ZMOD_AWG_AXI_CONFIGURATION_IP_INTR_STS_AP_DONE_MASK & Ctrl));

    // write to the interrupt bit to clear it
	ZmodAwgAxiConfiguration_WriteReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_IP_INTR_STS_REG_OFFSET, ZMOD_AWG_AXI_CONFIGURATION_IP_INTR_STS_AP_DONE_MASK);
}

void ZmodAwgAxiConfiguration_Initialize (ZmodAwgAxiConfiguration *InstPtr, u32 BaseAddr) {
	InstPtr->BaseAddr = BaseAddr;
	
	ZmodAwgAxiConfiguration_WriteReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_GIE_REG_OFFSET, ZMOD_AWG_AXI_CONFIGURATION_GIE_ENABLE_MASK);
	ZmodAwgAxiConfiguration_WriteReg(InstPtr->BaseAddr, ZMOD_AWG_AXI_CONFIGURATION_IP_INTR_EN_REG_OFFSET, ZMOD_AWG_AXI_CONFIGURATION_IP_INTR_EN_AP_DONE_MASK);
}

