#include "trigger.h"

u32 TriggerGetIdle (TriggerControl *InstPtr) {
	u32 Data = 0;
	TriggerControl_IssueApStart(InstPtr);
	Data = TriggerControl_ReadReg(InstPtr->BaseAddr, TRIGGER_CONTROL_STATUS_REG_OFFSET);
	Data &= TRIGGER_CONTROL_STATUS_IDLE_MASK;
	return Data;
}

void TriggerSetPosition (TriggerControl *InstPtr, u32 BufferLength, u32 TriggerPosition) {
	const u32 PrebufferLength = TriggerPosition;
	const u32 TrigToLastLength = BufferLength - TriggerPosition;
	TriggerControl_WriteReg(InstPtr->BaseAddr, TRIGGER_CONTROL_TRIGGERTOLASTBEATS_REG_OFFSET, TrigToLastLength);
	TriggerControl_WriteReg(InstPtr->BaseAddr, TRIGGER_CONTROL_PREBUFFERBEATS_REG_OFFSET, PrebufferLength);
	TriggerControl_IssueApStart(InstPtr);
}

void TriggerSetEnable (TriggerControl *InstPtr, u32 EnableMask) {
	TriggerControl_WriteReg(InstPtr->BaseAddr, TRIGGER_CONTROL_TRIGGERENABLE_REG_OFFSET, EnableMask);
	TriggerControl_IssueApStart(InstPtr);
}

void TriggerStart (TriggerControl *InstPtr) {
	TriggerControl_WriteReg(InstPtr->BaseAddr, TRIGGER_CONTROL_CONTROL_REG_OFFSET, TRIGGER_CONTROL_CONTROL_START_MASK);
	TriggerControl_IssueApStart(InstPtr);
	TriggerControl_WriteReg(InstPtr->BaseAddr, TRIGGER_CONTROL_CONTROL_REG_OFFSET, 0);
	TriggerControl_IssueApStart(InstPtr);
}

u32 TriggerGetDetected (TriggerControl *InstPtr) {
	TriggerControl_IssueApStart(InstPtr);
	return TriggerControl_ReadReg(InstPtr->BaseAddr, TRIGGER_CONTROL_TRIGGERDETECTED_REG_OFFSET) & TRIGGER_CONTROL_TRIGGERDETECTED_TRIGGERDETECTED_MASK;
}
