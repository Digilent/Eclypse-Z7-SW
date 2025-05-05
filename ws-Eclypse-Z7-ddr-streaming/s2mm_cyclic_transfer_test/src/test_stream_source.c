#include "test_stream_source.h"

/****************************************************************************/
/**
* Sets the source monitor's control register
*
* @param	InstPtr - Driver handler instance to operate on
* @param	Enable - Enables traffic generator hardware in the source monitor
* @param	Freerun - Determines traffic generator behavior:
* 				0: The AXI4-stream tdata only increments on successful data beats (valid & ready).
* 				1: The AXI4-stream tdata tdata increments every clock cycle, so that acquired data
* 					can be checked for gaps to determine whether any samples are lost due to backpressure on the stream.
* @param	GeneratorSelect - 0 or 1
* 				0: Default, the monitor acts as a passthrough, such that AXI4-stream data is passed from further upstream. The traffic generator hardware is ignored.
* 				1: The monitor discards AXI4-stream data from upstream and ties the master interface to the traffic generator.
*
* @return	none
*
*****************************************************************************/
void AxiStreamSourceMonitorSetControl(AxiStreamSourceMonitor *InstPtr, u8 Enable, u8 Freerun, u8 GeneratorSelect) {
	u32 Data = 0;

	if (Enable) Data |= AXI_STREAM_SOURCE_MONITOR_CONTROL_ENABLE_MASK;
	if (Freerun) Data |= AXI_STREAM_SOURCE_MONITOR_CONTROL_FREERUN_MASK;
	if (GeneratorSelect) Data |= AXI_STREAM_SOURCE_MONITOR_CONTROL_GENERATORSELECT_MASK;

	AxiStreamSourceMonitor_WriteReg(InstPtr->BaseAddr, AXI_STREAM_SOURCE_MONITOR_CONTROL_REG_OFFSET, Data);
	AxiStreamSourceMonitor_IssueApStart(InstPtr);
}

/****************************************************************************/
/**
* Sets only the enable bit in the source monitor's control register
*
* @param	InstPtr - Driver handler instance to operate on
* @param	Enable - 0 or 1, enables traffic generator hardware in the source monitor
*
* @return	none
*
* @note		Other bits are left in their original state
*
*****************************************************************************/
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

/****************************************************************************/
/**
* Sets only the generator select bit in the source monitor's control register
*
* @param	InstPtr - Driver handler instance to operate on
* @param	GeneratorSelect - 0 or 1
* 				0: Default, the monitor acts as a passthrough, such that AXI4-stream data is passed from further upstream. The traffic generator hardware is ignored.
* 				1: The monitor discards AXI4-stream data from upstream and ties the master interface to the traffic generator.
*
* @return	none
*
* @note		Other bits are left in their original state
*
*****************************************************************************/
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
