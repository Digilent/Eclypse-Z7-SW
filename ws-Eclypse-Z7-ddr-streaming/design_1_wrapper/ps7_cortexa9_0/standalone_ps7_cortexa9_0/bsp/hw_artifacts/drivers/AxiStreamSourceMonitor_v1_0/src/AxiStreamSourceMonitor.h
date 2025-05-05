#ifndef AXI_STREAM_SOURCE_MONITOR_H_   /* prevent circular inclusions */
#define AXI_STREAM_SOURCE_MONITOR_H_

#include "xil_types.h"
#include "xil_io.h"
#include "AxiStreamSourceMonitor_hw.h"

typedef struct {
	u32 BaseAddr;
} AxiStreamSourceMonitor;

#define AxiStreamSourceMonitor_In32     Xil_In32
#define AxiStreamSourceMonitor_Out32	Xil_Out32

#define AxiStreamSourceMonitor_ReadReg(BaseAddress, RegOffset)          AxiStreamSourceMonitor_In32((BaseAddress) + (RegOffset))
#define AxiStreamSourceMonitor_WriteReg(BaseAddress, RegOffset, Data)   AxiStreamSourceMonitor_Out32((BaseAddress) + (RegOffset), (Data))

void AxiStreamSourceMonitor_Initialize (AxiStreamSourceMonitor *InstPtr, u32 BaseAddr);
void AxiStreamSourceMonitor_IssueApStart (AxiStreamSourceMonitor *InstPtr);

#endif /* end of protection macro */

