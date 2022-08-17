#include "trigger.h"

/****************************************************************************/
/**
* Gets the trigger idle bit
*
* @param	InstPtr - Driver handler instance to operate on
*
* @return	Trigger idle bit, if high, the trigger state machine is ready to start
*
*****************************************************************************/
u32 TriggerGetIdle (TriggerControl *InstPtr) {
	u32 Data = 0;
	TriggerControl_IssueApStart(InstPtr);
	Data = TriggerControl_ReadReg(InstPtr->BaseAddr, TRIGGER_CONTROL_STATUS_REG_OFFSET);
	Data &= TRIGGER_CONTROL_STATUS_IDLE_MASK;
	return Data;
}

/****************************************************************************/
/**
* Sets the trigger to last beats and prebuffer beats registers of the trigger detector module
*
* @param	InstPtr - Driver handler instance to operate on
* @param	BufferLength - The length of the buffer used to capture data
* @param	TriggerPosition - Where in the acquisition the trigger event will be placed. Must fall in the range [0:BufferLength)
*
* @return	none
*
* @note		Buffer length is used to calculate the number of data beats which must be present in the buffer before a trigger event can
* 			occur, to ensure that the entire buffer is full of data. Trigger events which occur before enough data is prebuffered are ignored.
* 			The actual trigger event (and thus the first and last samples with respect to time) in the buffer can fall at any index in the final buffer.
*
* 			The trigger to last beats and prebuffer beats registers are used to count out the number of beats in each stage of an acquisition.
* 			Prebuffering happens first, and is used to partially fill the buffer.
* 			After prebuffering, an await state is entered where the trigger detector waits for a , forwarding any amount of data, which is loaded into a
* 				circle buffer, overwriting the first data received once it rolls over.
* 			After a trigger occurs, TriggerPosition - BufferLength beats are counted out, and the trigger detector returns to idle.
*
* 			Additional adjustment of these counts is performed in hardware to account for mismatched data and control signal latency through the module.
*
*****************************************************************************/
void TriggerSetPosition (TriggerControl *InstPtr, u32 BufferLength, u32 TriggerPosition) {
	const u32 PrebufferLength = TriggerPosition;
	const u32 TrigToLastLength = BufferLength - TriggerPosition;
	TriggerControl_WriteReg(InstPtr->BaseAddr, TRIGGER_CONTROL_TRIGGERTOLASTBEATS_REG_OFFSET, TrigToLastLength);
	TriggerControl_WriteReg(InstPtr->BaseAddr, TRIGGER_CONTROL_PREBUFFERBEATS_REG_OFFSET, PrebufferLength);
	TriggerControl_IssueApStart(InstPtr);
}

/****************************************************************************/
/**
* Sets the enable register of the trigger detector module
*
* @param	InstPtr - Driver handler instance to operate on
* @param	EnableMask - 32 enable bits which are used to mask out undesired triggers
*
* @return	none
*
* @note		Which enable bit maps to which trigger functionality depends on hardware implementation. Check the `trigger` input port of the trigger
* 			detector module. Each bit of the trigger enable enables the corresponding trigger input port bit.
*
*****************************************************************************/
void TriggerSetEnable (TriggerControl *InstPtr, u32 EnableMask) {
	TriggerControl_WriteReg(InstPtr->BaseAddr, TRIGGER_CONTROL_TRIGGERENABLE_REG_OFFSET, EnableMask);
	TriggerControl_IssueApStart(InstPtr);
}

/****************************************************************************/
/**
* Starts the trigger detector's state machine, immediately entering prebuffering state
*
* @param	InstPtr - Driver handler instance to operate on
*
* @return	none
*
* @note		In order to avoid sample loss, hardware upstream and downstream of the trigger detector must be ready to transmit and receive data
* 			when the detector is started.
*
*****************************************************************************/
void TriggerStart (TriggerControl *InstPtr) {
	TriggerControl_WriteReg(InstPtr->BaseAddr, TRIGGER_CONTROL_CONTROL_REG_OFFSET, TRIGGER_CONTROL_CONTROL_START_MASK);
	TriggerControl_IssueApStart(InstPtr);
	TriggerControl_WriteReg(InstPtr->BaseAddr, TRIGGER_CONTROL_CONTROL_REG_OFFSET, 0);
	TriggerControl_IssueApStart(InstPtr);
}

/****************************************************************************/
/**
* Gets the state of the trigger detector's "detected" register
*
* @param	InstPtr - Driver handler instance to operate on
*
* @return	The mask of the trigger that caused the trigger detector to exit the await state. Used to determine which trigger was received if multiple were enabled.
*
*****************************************************************************/
u32 TriggerGetDetected (TriggerControl *InstPtr) {
	TriggerControl_IssueApStart(InstPtr);
	return TriggerControl_ReadReg(InstPtr->BaseAddr, TRIGGER_CONTROL_TRIGGERDETECTED_REG_OFFSET) & TRIGGER_CONTROL_TRIGGERDETECTED_TRIGGERDETECTED_MASK;
}
