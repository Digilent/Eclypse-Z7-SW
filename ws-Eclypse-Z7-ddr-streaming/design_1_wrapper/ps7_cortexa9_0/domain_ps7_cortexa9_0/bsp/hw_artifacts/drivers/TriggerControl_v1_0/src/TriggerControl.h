#ifndef TRIGGER_CONTROL_H_   /* prevent circular inclusions */
#define TRIGGER_CONTROL_H_

#include "xil_types.h"
#include "xil_io.h"
#include "TriggerControl_hw.h"

typedef struct {
	u32 BaseAddr;
} TriggerControl;

#define TriggerControl_In32     Xil_In32
#define TriggerControl_Out32	Xil_Out32

#define TriggerControl_ReadReg(BaseAddress, RegOffset)          TriggerControl_In32((BaseAddress) + (RegOffset))
#define TriggerControl_WriteReg(BaseAddress, RegOffset, Data)   TriggerControl_Out32((BaseAddress) + (RegOffset), (Data))

void TriggerControl_Initialize (TriggerControl *InstPtr, u32 BaseAddr);
void TriggerControl_IssueApStart (TriggerControl *InstPtr);

#endif /* end of protection macro */

