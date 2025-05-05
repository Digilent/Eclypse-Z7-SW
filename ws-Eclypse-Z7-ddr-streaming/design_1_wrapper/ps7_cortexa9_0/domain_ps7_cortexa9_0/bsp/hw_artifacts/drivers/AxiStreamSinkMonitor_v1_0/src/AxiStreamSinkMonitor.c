#include "AxiStreamSinkMonitor.h"
void AxiStreamSinkMonitor_IssueApStart (AxiStreamSinkMonitor *InstPtr) {
	// Send the new stuff to hardware
	AxiStreamSinkMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_AP_CTRL_REG_OFFSET, AXI_STREAM_SINK_MONITOR_AP_CTRL_START_MASK);

	// Wait until ap_done interrupt goes high
	u32 Ctrl;
	do {
		Ctrl = AxiStreamSinkMonitor_ReadReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_IP_INTR_STS_REG_OFFSET);
	} while (!(AXI_STREAM_SINK_MONITOR_IP_INTR_STS_AP_DONE_MASK & Ctrl));

    // write to the interrupt bit to clear it
	AxiStreamSinkMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_IP_INTR_STS_REG_OFFSET, AXI_STREAM_SINK_MONITOR_IP_INTR_STS_AP_DONE_MASK);
}

void AxiStreamSinkMonitor_Initialize (AxiStreamSinkMonitor *InstPtr, u32 BaseAddr) {
	InstPtr->BaseAddr = BaseAddr;
	
	AxiStreamSinkMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_GIE_REG_OFFSET, AXI_STREAM_SINK_MONITOR_GIE_ENABLE_MASK);
	AxiStreamSinkMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SINK_MONITOR_IP_INTR_EN_REG_OFFSET, AXI_STREAM_SINK_MONITOR_IP_INTR_EN_AP_DONE_MASK);
}

