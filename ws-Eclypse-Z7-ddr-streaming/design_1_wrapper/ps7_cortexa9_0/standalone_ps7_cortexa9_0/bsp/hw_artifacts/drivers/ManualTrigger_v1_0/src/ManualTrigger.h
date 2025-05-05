#ifndef MANUAL_TRIGGER_H_   /* prevent circular inclusions */
#define MANUAL_TRIGGER_H_

#include "xil_types.h"
#include "xil_io.h"
#include "ManualTrigger_hw.h"

typedef struct {
	u32 BaseAddr;
} ManualTrigger;

#define ManualTrigger_In32     Xil_In32
#define ManualTrigger_Out32	Xil_Out32

#define ManualTrigger_ReadReg(BaseAddress, RegOffset)          ManualTrigger_In32((BaseAddress) + (RegOffset))
#define ManualTrigger_WriteReg(BaseAddress, RegOffset, Data)   ManualTrigger_Out32((BaseAddress) + (RegOffset), (Data))

void ManualTrigger_Initialize (ManualTrigger *InstPtr, u32 BaseAddr);
void ManualTrigger_IssueApStart (ManualTrigger *InstPtr);

#endif /* end of protection macro */

