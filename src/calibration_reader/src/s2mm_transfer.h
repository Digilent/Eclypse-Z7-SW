#ifndef S2MM_TRANSFER_H_   /* prevent circular inclusions */
#define S2MM_TRANSFER_H_

#include "xaxidma.h"
#include "xil_types.h"

typedef struct {
	XAxiDma Dma;
	u32 NumBds;
	u32 MaxBurstLengthBytes;
	u32 BufferLength;
	u32 *BdSpace;
	u32 *BufferBaseAddr;
} S2mmTransferHierarchy;

void S2mmInitialize (S2mmTransferHierarchy *InstPtr, const u32 DmaDeviceId);
void S2mmAttachBuffer (S2mmTransferHierarchy *InstPtr, UINTPTR Buffer, u32 BufferLength);
void S2mmStartCyclicTransfer (S2mmTransferHierarchy *InstPtr);
u32 *S2mmFindStartOfBuffer (S2mmTransferHierarchy *InstPtr);
void S2mmCleanup(S2mmTransferHierarchy *InstPtr);

#endif /* end of protection macro */
