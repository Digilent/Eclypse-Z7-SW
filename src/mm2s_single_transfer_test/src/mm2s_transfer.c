#include "mm2s_transfer.h"
#include "xil_printf.h"

void AllocateBdSpace (u32 NumBds, u32 **BdSpacePtr, u32 **BdSpaceAlignedPtr);

// FIXME: Determine what the minimum block size is that allows continuous transfers
// 		  Currently, the final block in a cyclic transfer could be as short as one word

void Mm2sInitialize (Mm2sTransferHierarchy *InstPtr, const u32 DmaDeviceId) {
	XAxiDma_Config *CfgPtr = NULL;
	XAxiDma *DmaPtr = NULL;
	XAxiDma_BdRing *RingPtr = NULL;
	const int Coalesce = 1;
	const int Delay = 0;

	DmaPtr = &(InstPtr->Dma);
	RingPtr = XAxiDma_GetTxRing(DmaPtr);

	CfgPtr = XAxiDma_LookupConfig(DmaDeviceId);
	XAxiDma_CfgInitialize(DmaPtr, CfgPtr);
	XAxiDma_IntrDisable(DmaPtr, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

	XAxiDma_BdRingIntDisable(RingPtr, XAXIDMA_IRQ_ALL_MASK);
	XAxiDma_BdRingSetCoalesce(RingPtr, Coalesce, Delay);

	// 8 bits per byte
	InstPtr->BeatWidthBytes = CfgPtr->Mm2SDataWidth / (8);
	InstPtr->MaxBurstLengthBytes = CfgPtr->Mm2SBurstSize * InstPtr->BeatWidthBytes;
	InstPtr->NumBds = 0;
	InstPtr->BdSpace = NULL;
	InstPtr->MaxBlockLengthBytes = 1 << CfgPtr->SgLengthWidth;
}

#define RoundUpDivide(a, b) ((a / b) + (a % b != 0))

void AllocateBdSpace (u32 NumBds, u32 **BdSpacePtr, u32 **BdSpaceAlignedPtr) {
	u32 BdSpaceBytes = (sizeof(XAxiDma_Bd) * (NumBds)) + (XAXIDMA_BD_MINIMUM_ALIGNMENT-sizeof(u32));
	*BdSpacePtr = malloc(BdSpaceBytes);
	// Get the address of the actual memory space that the Bds can fit into, following the alignment requirement
	*BdSpaceAlignedPtr = (u32*)((u32)(*BdSpacePtr) + XAXIDMA_BD_MINIMUM_ALIGNMENT - ((u32)(*BdSpacePtr) & (XAXIDMA_BD_MINIMUM_ALIGNMENT-1)));
}

void Mm2sCreateRing (Mm2sTransferHierarchy *InstPtr, u32 BufferLengthBytes) {
	XAxiDma *DmaPtr = NULL;
	XAxiDma_BdRing *RingPtr = NULL;
	u32 *BdSpaceAligned = NULL;
	XAxiDma_Bd BdTemplate;
	u32 NumBds = 0;

	DmaPtr = &(InstPtr->Dma);
	RingPtr = XAxiDma_GetTxRing(DmaPtr);

	NumBds = RoundUpDivide(BufferLengthBytes, InstPtr->MaxBlockLengthBytes);

	// If a ring has already been created that doesn't fit the desired buffer size, destroy it.
	if (InstPtr->NumBds < NumBds) {
		Mm2sDestroyRing(InstPtr);
		InstPtr->NumBds = NumBds;
		AllocateBdSpace(InstPtr->NumBds, &(InstPtr->BdSpace), &BdSpaceAligned);
	}

	XAxiDma_BdRingCreate(RingPtr, (UINTPTR)(BdSpaceAligned), (UINTPTR)(BdSpaceAligned), XAXIDMA_BD_MINIMUM_ALIGNMENT, InstPtr->NumBds);

	XAxiDma_BdClear(&BdTemplate);
	XAxiDma_BdRingClone(RingPtr, &BdTemplate);
}

// returns the number of bytes enqueued, if it returns 0, then everything was put into the buffer
void Mm2sStartTransfer(Mm2sTransferHierarchy *InstPtr, UINTPTR BufferAddr, u32 BufferLengthBytes, u32 CyclicMode) {
	XAxiDma *DmaPtr = NULL;
	XAxiDma_BdRing *RingPtr = NULL;
	XAxiDma_Bd *BdPtr = NULL;
	XAxiDma_Bd *BdCurPtr = NULL;
	u32 RemainingBytes = 0;
	UINTPTR BdBufAddr = 0;
	u32 BdCtrl = 0;
	u32 BdLength = 0;
	u32 NumBds = 0;
	u32 FreeBds = 0;
	u32 Status = 0;

	DmaPtr = &(InstPtr->Dma);
	RingPtr = XAxiDma_GetTxRing(DmaPtr);

	// Allocate enough Bds to fit all bytes in the buffer
	NumBds = RoundUpDivide(BufferLengthBytes, InstPtr->MaxBlockLengthBytes);
	FreeBds = XAxiDma_BdRingGetFreeCnt(RingPtr);

	if (FreeBds < NumBds) {
		xil_printf("Dma %08x doesn't have enough free block descriptors for buffer %08x\r\n", InstPtr, BufferAddr);
		return;
	}

	XAxiDma_BdRingAlloc(RingPtr, NumBds, &BdPtr);

	// Initialize the Bds to cover the Buffer
	RemainingBytes = BufferLengthBytes;
	BdBufAddr = BufferAddr;
	BdCurPtr = BdPtr;

	for (u32 i = 0; i < NumBds; i++) {
		// Zero out the Bd
		XAxiDma_BdClear(BdCurPtr);

		// Set the block buffer address
		XAxiDma_BdSetBufAddr(BdCurPtr, BdBufAddr);

		// FIXME: This method of making all transfers except the last the maximum length, and making the last potentially very short
		// means that excessively short transfers may occur in some edge cases. This is not a problem for single-shot transfers,
		// however for cyclic transfers, it means that a pending scatter gather transaction could potentially stall the data input stream.
		// This situation has not been tested for. Determining the minimum block length that can successfully be transferred while
		// maintaining full bandwidth would be helpful. Additional buffering in PL may also solve any issues that arise here.
		if (RemainingBytes < InstPtr->MaxBlockLengthBytes) {
			BdLength = RemainingBytes;
		} else {
			BdLength = InstPtr->MaxBlockLengthBytes;
		}

		// Note: BdLength must be equal to one less than the number of bytes actually being transmitted.
		// This field is presumably used as a direct comparison for the top of a counter, meaning that
		// for example a transfer of a 2 KiB block requires a length of 0x1FF, rather than the 0x200 that
		// might reasonably be expected based on the name of the field. Similarly, MaxTransferLength is
		// actually a bit mask.
		XAxiDma_BdSetLength(BdCurPtr, BdLength-1, RingPtr->MaxTransferLen);

//		BdCtrl = 0;
//
//		// Set the start of frame and end of frame flags on teh first and last blocks. These may be the same block.
//		if (i == 0) {
//			BdCtrl |= XAXIDMA_BD_CTRL_TXSOF_MASK;
//		}
//		if (i + 1 == NumBds) {
//			BdCtrl |= XAXIDMA_BD_CTRL_TXEOF_MASK;
//		}

		BdCtrl = XAXIDMA_BD_CTRL_TXSOF_MASK | XAXIDMA_BD_CTRL_TXEOF_MASK;

		XAxiDma_BdSetCtrl(BdCurPtr, BdCtrl);

		// Set the block id to the buffer address. This is potentially useful in debugging with an ILA, as it should appear in the tid field.
		XAxiDma_BdSetId(BdCurPtr, (u32)BdBufAddr);

		// Do counter stuff
		BdBufAddr += BdLength;
		RemainingBytes -= BdLength;

		// Traverse the linked list of blocks
		BdCurPtr = (XAxiDma_Bd*) XAxiDma_BdRingNext(RingPtr, BdCurPtr);
	}

	// Submit the Bds to hardware for processing. If the transfer isn't actively running already,
	// it must be started through a separate call to Mm2sStartTransfer
	Status = XAxiDma_BdRingToHw(RingPtr, NumBds, BdPtr); // must still set the SOF and EOF flags for Tx Cyclic

	// Set up Cyclic mode on both the Dma and Bd Ring
	// FIXME: Check whether a cyclic transfer must use the entire Bd Ring, or if it can consist of only allocated blocks, the latter would save time destroying and creating rings
	// FIXME: Check if setting cyclic mode needs to happen earlier
	InstPtr->CyclicMode = CyclicMode;
	if (InstPtr->CyclicMode) {
		XAxiDma_SelectCyclicMode(&(InstPtr->Dma), XAXIDMA_DMA_TO_DEVICE, TRUE);
		XAxiDma_BdRingEnableCyclicDMA(RingPtr);
	} else {
		XAxiDma_SelectCyclicMode(&(InstPtr->Dma), XAXIDMA_DMA_TO_DEVICE, FALSE);
		RingPtr->Cyclic = 0; // FIXME: No disable counterpart of XAxiDma_BdRingEnableCyclicDMA exists
	}

	Status = XAxiDma_BdRingStart(RingPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Tx BdRingStart failed %d for dma 0x%08x\r\n", Status, InstPtr);
	}
}

u32 Mm2sResubmit(Mm2sTransferHierarchy *InstPtr) {
	XAxiDma *DmaPtr = NULL;
	XAxiDma_BdRing *RingPtr = NULL;
	XAxiDma_Bd *BdPtr = NULL;
	u32 Status = 0;
	u32 NumBds = 0;

	DmaPtr = &(InstPtr->Dma);
	RingPtr = XAxiDma_GetTxRing(DmaPtr);

	NumBds = XAxiDma_BdRingFromHw(RingPtr, 1, &BdPtr);
	// record the length and buffer address of the Bd
	if (NumBds == 0) {
		return 0;
	}

	// Save user info from the processed Bd
	u32 BdBufAddr = XAxiDma_BdGetBufAddr(BdPtr);
	u32 BdLength = XAxiDma_BdGetLength(BdPtr, RingPtr->MaxTransferLen);
//	u32 BdCtrl = XAxiDma_BdGetCtrl(BdPtr) & (XAXIDMA_BD_CTRL_TXSOF_MASK | XAXIDMA_BD_CTRL_TXEOF_MASK);
	u32 BdCtrl = XAXIDMA_BD_CTRL_TXSOF_MASK | XAXIDMA_BD_CTRL_TXEOF_MASK;
	u32 BdId = XAxiDma_BdGetId(BdPtr);

	// Free the Bd
	XAxiDma_BdRingFree(RingPtr, NumBds, BdPtr);
	BdPtr = NULL;

	// Allocate a new Bd
	XAxiDma_BdRingAlloc(RingPtr, NumBds, &BdPtr);

	// Zero out the Bd then reapply all saved info
	XAxiDma_BdClear(BdPtr);
	XAxiDma_BdSetBufAddr(BdPtr, BdBufAddr);
	XAxiDma_BdSetLength(BdPtr, BdLength, RingPtr->MaxTransferLen);
	XAxiDma_BdSetCtrl(BdPtr, BdCtrl);
	XAxiDma_BdSetId(BdPtr, BdId);

	// Put the Bd back into hardware
	XAxiDma_BdRingToHw(RingPtr, NumBds, BdPtr);

	return NumBds;
}

u32 Mm2sCyclicResubmit(Mm2sTransferHierarchy *InstPtr) {
	XAxiDma *DmaPtr = NULL;
	XAxiDma_BdRing *RingPtr = NULL;
	XAxiDma_Bd *BdPtr = NULL;
	u32 NumBds = 0;
	u32 Status = 0;

	DmaPtr = &(InstPtr->Dma);
	RingPtr = XAxiDma_GetTxRing(DmaPtr);

	NumBds = XAxiDma_BdRingFromHw(RingPtr, XAXIDMA_ALL_BDS, &BdPtr);
	if (NumBds > 0) {
		Status = XAxiDma_BdRingToHw(RingPtr, NumBds, BdPtr);
		if (Status) {
			xil_printf("Error: Resubmitting Bds failed\r\n");
		}
	}
	return NumBds;
}

void Mm2sDestroyRing(Mm2sTransferHierarchy *InstPtr) {
	// Clean up Bds and deallocate the BdSpace
	// FIXME: Any pending transfers must be halted prior to calling this.
	InstPtr->NumBds = 0;
	if (InstPtr->BdSpace != NULL) {
		free(InstPtr->BdSpace);
		InstPtr->BdSpace = NULL;
	}
}

u32 Mm2sTransferDone (Mm2sTransferHierarchy *InstPtr) {
	XAxiDma_BdRing *RingPtr = NULL;
	XAxiDma_Bd *BdCurPtr = NULL;
	XAxiDma_Bd *BdPtr = NULL;
	u32 ProcessedBdCount = 0;
	u32 Last = 0;

	if (InstPtr->CyclicMode) {
		xil_printf("Mm2sTransferDone not applicable for Cyclic Mode\r\n");
		return 0;
	}

	RingPtr = XAxiDma_GetTxRing(&(InstPtr->Dma));
	ProcessedBdCount = XAxiDma_BdRingFromHw(RingPtr, XAXIDMA_ALL_BDS, &BdPtr);

	if (ProcessedBdCount == 0) {
		return 0;
	}

	Last = 0;
	BdCurPtr = BdPtr;
	for (u32 i = 0; i < ProcessedBdCount; i++) {
		Last |= XAxiDma_BdGetCtrl(BdCurPtr) & XAXIDMA_BD_CTRL_TXEOF_MASK;
		BdCurPtr = (XAxiDma_Bd*) XAxiDma_BdRingNext(RingPtr, BdCurPtr);
	}

	XAxiDma_BdRingFree(RingPtr, ProcessedBdCount, BdPtr);

	if (Last != 0) {
		return 1;
	} else {
		return 0;
	}
}


void Mm2sCyclicModeCleanup (Mm2sTransferHierarchy *InstPtr) {
	//FIXME: incomplete implementation
	return;
}
