#include "TriggerControl.h"
void TriggerControl_IssueApStart (TriggerControl *InstPtr) {
	// Send the new stuff to hardware
	TriggerControl_WriteReg(InstPtr->BaseAddr, TRIGGER_CONTROL_AP_CTRL_REG_OFFSET, TRIGGER_CONTROL_AP_CTRL_START_MASK);

	// Wait until ap_done interrupt goes high
	u32 Ctrl;
	do {
		Ctrl = TriggerControl_ReadReg(InstPtr->BaseAddr, TRIGGER_CONTROL_IP_INTR_STS_REG_OFFSET);
	} while (!(TRIGGER_CONTROL_IP_INTR_STS_AP_DONE_MASK & Ctrl));

    // write to the interrupt bit to clear it
	TriggerControl_WriteReg(InstPtr->BaseAddr, TRIGGER_CONTROL_IP_INTR_STS_REG_OFFSET, TRIGGER_CONTROL_IP_INTR_STS_AP_DONE_MASK);
}

void TriggerControl_Initialize (TriggerControl *InstPtr, u32 BaseAddr) {
	InstPtr->BaseAddr = BaseAddr;
	
	TriggerControl_WriteReg(InstPtr->BaseAddr, TRIGGER_CONTROL_GIE_REG_OFFSET, TRIGGER_CONTROL_GIE_ENABLE_MASK);
	TriggerControl_WriteReg(InstPtr->BaseAddr, TRIGGER_CONTROL_IP_INTR_EN_REG_OFFSET, TRIGGER_CONTROL_IP_INTR_EN_AP_DONE_MASK);
}

