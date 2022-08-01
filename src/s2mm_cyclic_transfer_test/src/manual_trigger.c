#include "manual_trigger.h"

void ManualTriggerIssueTrigger(ManualTrigger *InstPtr) {
	ManualTrigger_WriteReg(InstPtr->BaseAddr, MANUAL_TRIGGER_TRIGGER_REG_OFFSET, MANUAL_TRIGGER_TRIGGER_TRIGGER_MASK);
	ManualTrigger_IssueApStart(InstPtr);
	ManualTrigger_WriteReg(InstPtr->BaseAddr, MANUAL_TRIGGER_TRIGGER_REG_OFFSET, 0);
	ManualTrigger_IssueApStart(InstPtr);
}
