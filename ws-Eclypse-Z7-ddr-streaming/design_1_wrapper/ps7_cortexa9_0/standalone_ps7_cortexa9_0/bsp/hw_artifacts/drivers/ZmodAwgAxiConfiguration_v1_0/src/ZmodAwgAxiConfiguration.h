#ifndef ZMOD_AWG_AXI_CONFIGURATION_H_   /* prevent circular inclusions */
#define ZMOD_AWG_AXI_CONFIGURATION_H_

#include "xil_types.h"
#include "xil_io.h"
#include "ZmodAwgAxiConfiguration_hw.h"

typedef struct {
	u32 BaseAddr;
} ZmodAwgAxiConfiguration;

#define ZmodAwgAxiConfiguration_In32     Xil_In32
#define ZmodAwgAxiConfiguration_Out32	Xil_Out32

#define ZmodAwgAxiConfiguration_ReadReg(BaseAddress, RegOffset)          ZmodAwgAxiConfiguration_In32((BaseAddress) + (RegOffset))
#define ZmodAwgAxiConfiguration_WriteReg(BaseAddress, RegOffset, Data)   ZmodAwgAxiConfiguration_Out32((BaseAddress) + (RegOffset), (Data))

void ZmodAwgAxiConfiguration_Initialize (ZmodAwgAxiConfiguration *InstPtr, u32 BaseAddr);
void ZmodAwgAxiConfiguration_IssueApStart (ZmodAwgAxiConfiguration *InstPtr);

#endif /* end of protection macro */

