#ifndef MM2S_TRANSFER_H_   /* prevent circular inclusions */
#define MM2S_TRANSFER_H_

#include "xaxidma.h"
#include "xil_types.h"

typedef struct {
	XAxiDma Dma;
	u32 BeatWidthBytes;
	u32 MaxBurstLengthBytes; // based on max burst size configured in hardware
	u32 MaxBlockLengthBytes; // based on buffer length register width configured in hardware
	u32 *BdSpace;
	u32 NumBds; // number of Bds that can fit in the BdSpace. MaxBlockLengthBytes * NumBds determines the maximum buffer size, however, NumBds is calculated automatically from the provided buffer size when creating a Bd ring
	u32 CyclicMode;
//	u32 MinimumAlignment;
} Mm2sTransferHierarchy;

// Note: It's not viable to do dynamic-length transfers using a software FIFO. Something weird happens potentially when the BD ring contains a SOF but not and EOF
// FIXME: determine if its possible to reuse the same buffer with modified addresses - attach a new buffer without destroying an existing BD space
// partial answer: Bds must be reallocated regardless. The Bd space need not be destroyed outright unless the number of blocks needs to be changed

void Mm2sInitialize (Mm2sTransferHierarchy *InstPtr, const u32 DmaDeviceId);

void Mm2sCreateRing (Mm2sTransferHierarchy *InstPtr, u32 BufferLengthBytes);
void Mm2sDestroyRing (Mm2sTransferHierarchy *InstPtr);

void Mm2sStartTransfer(Mm2sTransferHierarchy *InstPtr, UINTPTR BufferAddr, u32 BufferLengthBytes, u32 CyclicMode);
u32 Mm2sCyclicResubmit(Mm2sTransferHierarchy *InstPtr);
u32 Mm2sTransferDone (Mm2sTransferHierarchy *InstPtr);

u32 Mm2sResubmit(Mm2sTransferHierarchy *InstPtr);

void Mm2sCyclicModeCleanup (Mm2sTransferHierarchy *InstPtr);

#endif /* end of protection macro */
