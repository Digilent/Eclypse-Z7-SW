#ifndef USER_REGISTERS_H_   /* prevent circular inclusions */
#define USER_REGISTERS_H_

#include "xil_types.h"
#include "xil_io.h"
#include "UserRegisters_hw.h"

typedef struct {
	u32 BaseAddr;
} UserRegisters;

#define UserRegisters_In32     Xil_In32
#define UserRegisters_Out32	Xil_Out32

#define UserRegisters_ReadReg(BaseAddress, RegOffset)          UserRegisters_In32((BaseAddress) + (RegOffset))
#define UserRegisters_WriteReg(BaseAddress, RegOffset, Data)   UserRegisters_Out32((BaseAddress) + (RegOffset), (Data))

void UserRegisters_Initialize (UserRegisters *InstPtr, u32 BaseAddr);
void UserRegisters_IssueApStart (UserRegisters *InstPtr);

#endif /* end of protection macro */

