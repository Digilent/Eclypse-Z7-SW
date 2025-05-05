#include "UserRegisters.h"
void UserRegisters_IssueApStart (UserRegisters *InstPtr) {
	// Send the new stuff to hardware
	UserRegisters_WriteReg(InstPtr->BaseAddr, USER_REGISTERS_AP_CTRL_REG_OFFSET, USER_REGISTERS_AP_CTRL_START_MASK);

	// Wait until ap_done interrupt goes high
	u32 Ctrl;
	do {
		Ctrl = UserRegisters_ReadReg(InstPtr->BaseAddr, USER_REGISTERS_IP_INTR_STS_REG_OFFSET);
	} while (!(USER_REGISTERS_IP_INTR_STS_AP_DONE_MASK & Ctrl));

    // write to the interrupt bit to clear it
	UserRegisters_WriteReg(InstPtr->BaseAddr, USER_REGISTERS_IP_INTR_STS_REG_OFFSET, USER_REGISTERS_IP_INTR_STS_AP_DONE_MASK);
}

void UserRegisters_Initialize (UserRegisters *InstPtr, u32 BaseAddr) {
	InstPtr->BaseAddr = BaseAddr;
	
	UserRegisters_WriteReg(InstPtr->BaseAddr, USER_REGISTERS_GIE_REG_OFFSET, USER_REGISTERS_GIE_ENABLE_MASK);
	UserRegisters_WriteReg(InstPtr->BaseAddr, USER_REGISTERS_IP_INTR_EN_REG_OFFSET, USER_REGISTERS_IP_INTR_EN_AP_DONE_MASK);
}

