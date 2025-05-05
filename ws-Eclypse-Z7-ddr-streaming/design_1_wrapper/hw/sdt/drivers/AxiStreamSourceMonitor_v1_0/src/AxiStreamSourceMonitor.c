#include "AxiStreamSourceMonitor.h"
void AxiStreamSourceMonitor_IssueApStart (AxiStreamSourceMonitor *InstPtr) {
	// Send the new stuff to hardware
	AxiStreamSourceMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SOURCE_MONITOR_AP_CTRL_REG_OFFSET, AXI_STREAM_SOURCE_MONITOR_AP_CTRL_START_MASK);

	// Wait until ap_done interrupt goes high
	u32 Ctrl;
	do {
		Ctrl = AxiStreamSourceMonitor_ReadReg(InstPtr->BaseAddr, AXI_STREAM_SOURCE_MONITOR_IP_INTR_STS_REG_OFFSET);
	} while (!(AXI_STREAM_SOURCE_MONITOR_IP_INTR_STS_AP_DONE_MASK & Ctrl));

    // write to the interrupt bit to clear it
	AxiStreamSourceMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SOURCE_MONITOR_IP_INTR_STS_REG_OFFSET, AXI_STREAM_SOURCE_MONITOR_IP_INTR_STS_AP_DONE_MASK);
}

void AxiStreamSourceMonitor_Initialize (AxiStreamSourceMonitor *InstPtr, u32 BaseAddr) {
	InstPtr->BaseAddr = BaseAddr;
	
	AxiStreamSourceMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SOURCE_MONITOR_GIE_REG_OFFSET, AXI_STREAM_SOURCE_MONITOR_GIE_ENABLE_MASK);
	AxiStreamSourceMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SOURCE_MONITOR_IP_INTR_EN_REG_OFFSET, AXI_STREAM_SOURCE_MONITOR_IP_INTR_EN_AP_DONE_MASK);
}

