#include "ManualTrigger.h"
void ManualTrigger_IssueApStart (ManualTrigger *InstPtr) {
	// Send the new stuff to hardware
	ManualTrigger_WriteReg(InstPtr->BaseAddr, MANUAL_TRIGGER_AP_CTRL_REG_OFFSET, MANUAL_TRIGGER_AP_CTRL_START_MASK);

	// Wait until ap_done interrupt goes high
	u32 Ctrl;
	do {
		Ctrl = ManualTrigger_ReadReg(InstPtr->BaseAddr, MANUAL_TRIGGER_IP_INTR_STS_REG_OFFSET);
	} while (!(MANUAL_TRIGGER_IP_INTR_STS_AP_DONE_MASK & Ctrl));

    // write to the interrupt bit to clear it
	ManualTrigger_WriteReg(InstPtr->BaseAddr, MANUAL_TRIGGER_IP_INTR_STS_REG_OFFSET, MANUAL_TRIGGER_IP_INTR_STS_AP_DONE_MASK);
}

void ManualTrigger_Initialize (ManualTrigger *InstPtr, u32 BaseAddr) {
	InstPtr->BaseAddr = BaseAddr;
	
	ManualTrigger_WriteReg(InstPtr->BaseAddr, MANUAL_TRIGGER_GIE_REG_OFFSET, MANUAL_TRIGGER_GIE_ENABLE_MASK);
	ManualTrigger_WriteReg(InstPtr->BaseAddr, MANUAL_TRIGGER_IP_INTR_EN_REG_OFFSET, MANUAL_TRIGGER_IP_INTR_EN_AP_DONE_MASK);
}

