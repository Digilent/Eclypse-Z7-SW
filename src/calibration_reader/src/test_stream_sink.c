#include "test_stream_sink.h"

void AxiStreamSinkMonitor_SetLastCount(AxiStreamSinkMonitor *InstPtr, u32 LastCount) {
	AxiStreamSinkMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_NUMFRAMES_REG_OFFSET, LastCount);
	AxiStreamSinkMonitor_IssueApStart(InstPtr);
}

void AxiStreamSinkMonitor_Start(AxiStreamSinkMonitor *InstPtr) {
	u32 Data;

	AxiStreamSinkMonitor_IssueApStart(InstPtr);
	Data = AxiStreamSinkMonitor_ReadReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_CONTROL_REG_OFFSET);

	AxiStreamSinkMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_CONTROL_REG_OFFSET, Data | AXI_STREAM_SINK_MONITOR_CONTROL_START_MASK);
	AxiStreamSinkMonitor_IssueApStart(InstPtr);

	AxiStreamSinkMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_CONTROL_REG_OFFSET, Data);
	AxiStreamSinkMonitor_IssueApStart(InstPtr);
}

void AxiStreamSinkMonitor_SetSelect(AxiStreamSinkMonitor *InstPtr, u8 SelectBit) {
	u32 Data;

	AxiStreamSinkMonitor_IssueApStart(InstPtr);
	Data = AxiStreamSinkMonitor_ReadReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_CONTROL_REG_OFFSET);

	Data |= (AXI_STREAM_SINK_MONITOR_CONTROL_SELECTVOID_MASK * SelectBit); /* set bit to 0 or 1 depending on SelectBit */

	AxiStreamSinkMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_CONTROL_REG_OFFSET, Data);
	AxiStreamSinkMonitor_IssueApStart(InstPtr);
}

u32 AxiStreamSinkMonitor_GetIdle(AxiStreamSinkMonitor *InstPtr) {
	AxiStreamSinkMonitor_IssueApStart(InstPtr);
	return AxiStreamSinkMonitor_ReadReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_STATUS_REG_OFFSET) & AXI_STREAM_SINK_MONITOR_STATUS_IDLE_MASK;
}

u32 AxiStreamSinkMonitor_GetBeatCount(AxiStreamSinkMonitor *InstPtr) {
	AxiStreamSinkMonitor_IssueApStart(InstPtr);
	return AxiStreamSinkMonitor_ReadReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_BEATCOUNT_REG_OFFSET);
}

u32 AxiStreamSinkMonitor_GetMissCount(AxiStreamSinkMonitor *InstPtr) {
	AxiStreamSinkMonitor_IssueApStart(InstPtr);
	return AxiStreamSinkMonitor_ReadReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_MISSCOUNT_REG_OFFSET);
}

u32 AxiStreamSinkMonitor_GetErrorCount(AxiStreamSinkMonitor *InstPtr) {
	AxiStreamSinkMonitor_IssueApStart(InstPtr);
	return AxiStreamSinkMonitor_ReadReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_ERRORCOUNT_REG_OFFSET);
}
