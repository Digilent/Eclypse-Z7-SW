#include "test_stream_source.h"

void AxiStreamSourceMonitorSetControl(AxiStreamSourceMonitor *InstPtr, u8 Enable, u8 Freerun, u8 GeneratorSelect) {
	u32 Data = 0;

	if (Enable) Data |= AXI_STREAM_SOURCE_MONITOR_CONTROL_ENABLE_MASK;
	if (Freerun) Data |= AXI_STREAM_SOURCE_MONITOR_CONTROL_FREERUN_MASK;
	if (GeneratorSelect) Data |= AXI_STREAM_SOURCE_MONITOR_CONTROL_GENERATORSELECT_MASK;

	AxiStreamSourceMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SOURCE_MONITOR_CONTROL_REG_OFFSET, Data);
	AxiStreamSourceMonitor_IssueApStart(InstPtr);
}

void AxiStreamSourceMonitorSetEnable(AxiStreamSourceMonitor *InstPtr, u8 Enable) {
	u32 Data = 0;
	Data = AxiStreamSourceMonitor_ReadReg(InstPtr->BaseAddr, AXI_STREAM_SOURCE_MONITOR_CONTROL_REG_OFFSET);

	if (Enable) {
		Data |= AXI_STREAM_SOURCE_MONITOR_CONTROL_ENABLE_MASK;
	} else {
		Data &= ~AXI_STREAM_SOURCE_MONITOR_CONTROL_ENABLE_MASK;
	}

	AxiStreamSourceMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SOURCE_MONITOR_CONTROL_REG_OFFSET, Data);
	AxiStreamSourceMonitor_IssueApStart(InstPtr);
}

void AxiStreamSourceMonitorSetSelect(AxiStreamSourceMonitor *InstPtr, u8 GeneratorSelect) {
	u32 Data = 0;
	Data = AxiStreamSourceMonitor_ReadReg(InstPtr->BaseAddr, AXI_STREAM_SOURCE_MONITOR_CONTROL_REG_OFFSET);

	if (GeneratorSelect) {
		Data |= AXI_STREAM_SOURCE_MONITOR_CONTROL_GENERATORSELECT_MASK;
	} else {
		Data &= ~AXI_STREAM_SOURCE_MONITOR_CONTROL_GENERATORSELECT_MASK;
	}

	AxiStreamSourceMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SOURCE_MONITOR_CONTROL_REG_OFFSET, Data);
	AxiStreamSourceMonitor_IssueApStart(InstPtr);
}
