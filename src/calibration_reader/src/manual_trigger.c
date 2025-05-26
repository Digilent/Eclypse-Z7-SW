#include "manual_trigger.h"

/****************************************************************************/
/**
* Toggles the manual trigger bit high then low. This applies a pulse to a connected trigger line.
*
* @param	InstPtr is the device handler instance for the ManualTrigger IP.
* @return	none
*
*****************************************************************************/
void ManualTriggerIssueTrigger(ManualTrigger *InstPtr) {
	ManualTrigger_WriteReg(InstPtr->BaseAddr, MANUAL_TRIGGER_TRIGGER_REG_OFFSET, MANUAL_TRIGGER_TRIGGER_TRIGGER_MASK);
	ManualTrigger_IssueApStart(InstPtr);
	ManualTrigger_WriteReg(InstPtr->BaseAddr, MANUAL_TRIGGER_TRIGGER_REG_OFFSET, 0);
	ManualTrigger_IssueApStart(InstPtr);
}
