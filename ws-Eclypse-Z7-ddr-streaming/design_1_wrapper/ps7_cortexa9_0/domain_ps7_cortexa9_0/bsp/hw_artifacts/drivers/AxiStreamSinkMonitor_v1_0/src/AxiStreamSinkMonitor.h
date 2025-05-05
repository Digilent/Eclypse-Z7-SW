#ifndef AXI_STREAM_SINK_MONITOR_H_   /* prevent circular inclusions */
#define AXI_STREAM_SINK_MONITOR_H_

#include "xil_types.h"
#include "xil_io.h"
#include "AxiStreamSinkMonitor_hw.h"

typedef struct {
	u32 BaseAddr;
} AxiStreamSinkMonitor;

#define AxiStreamSinkMonitor_In32     Xil_In32
#define AxiStreamSinkMonitor_Out32	Xil_Out32

#define AxiStreamSinkMonitor_ReadReg(BaseAddress, RegOffset)          AxiStreamSinkMonitor_In32((BaseAddress) + (RegOffset))
#define AxiStreamSinkMonitor_WriteReg(BaseAddress, RegOffset, Data)   AxiStreamSinkMonitor_Out32((BaseAddress) + (RegOffset), (Data))

void AxiStreamSinkMonitor_Initialize (AxiStreamSinkMonitor *InstPtr, u32 BaseAddr);
void AxiStreamSinkMonitor_IssueApStart (AxiStreamSinkMonitor *InstPtr);

#endif /* end of protection macro */

