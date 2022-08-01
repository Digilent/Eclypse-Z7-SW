#include "s2mm_transfer.h"
#include "xil_printf.h"

void AllocateBdSpace();

void S2mmInitialize (S2mmTransferHierarchy *InstPtr, const u32 DmaDeviceId) {
	XAxiDma_Config *CfgPtr;

	CfgPtr = XAxiDma_LookupConfig(DmaDeviceId);
	XAxiDma_CfgInitialize(&(InstPtr->Dma), CfgPtr);
	XAxiDma_IntrDisable(&(InstPtr->Dma), XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

	InstPtr->MaxBurstLengthBytes = CfgPtr->S2MmBurstSize * CfgPtr->S2MmDataWidth / (8);
	InstPtr->NumBds = 0;
	InstPtr->BdSpace = NULL;
	InstPtr->BufferBaseAddr = NULL;
}

#define RoundUpDivide(a, b) ((a / b) + (a % b != 0))

void S2mmAttachBuffer (S2mmTransferHierarchy *InstPtr, UINTPTR Buffer, u32 BufferLength) {
	XAxiDma_BdRing *RingPtr = NULL;
	XAxiDma *DmaPtr = NULL;
	int Coalesce = 1;
	int Delay = 0;
	XAxiDma_Bd BdTemplate;
	XAxiDma_Bd *BdPtr = NULL;
	XAxiDma_Bd *BdCurPtr = NULL;
	u32 FreeBdCount = 0;
	u32 *BdSpaceAligned = NULL;
	UINTPTR BdBufferPtr = 0;
	int Index = 0;
	int Status = 0;
	u32 BdSpaceBytes = 0;
	u32 BdLength = 0;

	DmaPtr = &(InstPtr->Dma);
	RingPtr = XAxiDma_GetRxRing(DmaPtr);

	InstPtr->BufferBaseAddr = (u32*)Buffer;

	// Allocate the memory space holding DMA block descriptors
	// Note: Xilinx doesn't support dynamic allocation of aligned memory, so we need to allocate more memory than is actually needed in order to guarantee alignment
	InstPtr->NumBds = RoundUpDivide(BufferLength * sizeof(u32), InstPtr->MaxBurstLengthBytes);
	BdSpaceBytes = (sizeof(XAxiDma_Bd) * (InstPtr->NumBds)) + (XAXIDMA_BD_MINIMUM_ALIGNMENT-sizeof(u32));
	InstPtr->BdSpace = malloc(BdSpaceBytes);
	// Get the address of the actual memory space that the Bds can fit into, following the alignment requirement
	BdSpaceAligned = (u32*)((u32)(InstPtr->BdSpace) + XAXIDMA_BD_MINIMUM_ALIGNMENT - ((u32)(InstPtr->BdSpace) & (XAXIDMA_BD_MINIMUM_ALIGNMENT-1)));

	// Disable interrupts coming from the Bd ring
	XAxiDma_BdRingIntDisable(RingPtr, XAXIDMA_IRQ_ALL_MASK);
	Status = XAxiDma_BdRingSetCoalesce(RingPtr, Coalesce, Delay); // defaults, this design isn't doing anything with interrupts
	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_BdRingSetCoalesce failed for s2mm instance 0x%08x\r\n", InstPtr);
	}

	// Set up the BD ring to use the allocated memory space
	Status = XAxiDma_BdRingCreate(RingPtr, (UINTPTR)(BdSpaceAligned), (UINTPTR)(BdSpaceAligned), XAXIDMA_BD_MINIMUM_ALIGNMENT, InstPtr->NumBds);
	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_BdRingCreate failed for s2mm instance 0x%08x\r\n", InstPtr);
	}

	// use a all-zero bd as template
	XAxiDma_BdClear(&BdTemplate);
	Status = XAxiDma_BdRingClone(RingPtr, &BdTemplate);
	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_BdRingClone failed for s2mm instance 0x%08x\r\n", InstPtr);
	}

	//allocate descriptors to fill the entire space
	FreeBdCount = XAxiDma_BdRingGetFreeCnt(RingPtr);
	Status = XAxiDma_BdRingAlloc(RingPtr, FreeBdCount, &BdPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_BdRingAlloc failed for s2mm instance 0x%08x\r\n", InstPtr);
	}

	BdCurPtr = BdPtr;
	BdBufferPtr = Buffer;

	for (Index = 0; Index < FreeBdCount; Index++) {
		// FIXME: Bd length always being the max length restricts transfers to buffer lengths that are integer multiples of MaxBurstLengthBytes.
		// constraints on the BD size need to be determined, a minimum can be experimentally found by lowering block sizes until beats start
		// getting stalled due to backpressure caused by DMA blocks waiting on scatter gather
		// its possible this doesn't occur due to the dma scatter gather engine buffering two or three transactions ahead
		BdLength = InstPtr->MaxBurstLengthBytes;

		Status = XAxiDma_BdSetBufAddr(BdCurPtr, (UINTPTR)BdBufferPtr);
		if (Status != XST_SUCCESS) {
			xil_printf("XAxiDma_BdSetBufAddr failed for s2mm instance 0x%08x, BD %d\r\n", InstPtr, Index);
		}

		Status = XAxiDma_BdSetLength(BdCurPtr, BdLength, RingPtr->MaxTransferLen);
		if (Status != XST_SUCCESS) {
			xil_printf("XAxiDma_BdSetBufAddr failed for s2mm instance 0x%08x, BD %d\r\n", InstPtr, Index);
		}

		XAxiDma_BdSetCtrl(BdCurPtr, 0);
		XAxiDma_BdSetId(BdCurPtr, BdBufferPtr);

		BdBufferPtr += BdLength;
		BdCurPtr = (XAxiDma_Bd *)XAxiDma_BdRingNext(RingPtr, BdCurPtr);
	}

	Status = XAxiDma_BdRingToHw(RingPtr, FreeBdCount, BdPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_BdRingToHw failed for s2mm instance 0x%08x\r\n", InstPtr);
	}
}

void S2mmDetachBuffer(S2mmTransferHierarchy *InstPtr) {
	// Clean up Bds and deallocate the BdSpace
	InstPtr->BufferBaseAddr = NULL;
	InstPtr->NumBds = 0;
	free(InstPtr->BdSpace);
	InstPtr->BdSpace = NULL;
}

void S2mmStartCyclicTransfer (S2mmTransferHierarchy *InstPtr) {
	XAxiDma_BdRing *RingPtr;
	RingPtr = XAxiDma_GetRxRing(&(InstPtr->Dma));
	int Status;

	XAxiDma_SelectCyclicMode(&(InstPtr->Dma), XAXIDMA_DEVICE_TO_DMA, TRUE);
	XAxiDma_BdRingEnableCyclicDMA(RingPtr);

	Status = XAxiDma_BdRingStart(RingPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("RX start hw failed %d for dma 0x%08x\r\n", Status, InstPtr);
	}
}

u32 *FindStartOfBuffer (S2mmTransferHierarchy *InstPtr) {
	XAxiDma_BdRing *RingPtr = XAxiDma_GetRxRing(&(InstPtr->Dma));
	XAxiDma_Bd *BdPtr;
	u32 ActualLength;

	BdPtr = (XAxiDma_Bd*)RingPtr->FirstBdAddr; // this is kind of weird. FIXME find out why this value isn't the one getting returned from BdRingFree

	for (u32 i = 0; i < InstPtr->NumBds; i++) {
		Xil_DCacheInvalidateRange((UINTPTR)BdPtr, XAXIDMA_BD_NUM_WORDS * sizeof(u32));

		u32 Status = XAxiDma_BdGetSts(BdPtr);
		// RXEOF bit high indicates that tlast occured in that block
		if (Status & XAXIDMA_BD_STS_RXEOF_MASK) {
			ActualLength = XAxiDma_BdGetActualLength(BdPtr, ((InstPtr->MaxBurstLengthBytes*2)-1));
			xil_printf("Last beat found:\r\n");
			xil_printf("  BD base address: %08x\r\n", XAxiDma_BdGetBufAddr(BdPtr));
			xil_printf("  BD actual length: %08x\r\n", ActualLength);
			return (u32*)(XAxiDma_BdGetBufAddr(BdPtr) + ActualLength); // FIXME: could return a value past the end of the buffer if ActualLength == BdLength
		}

		// Advance the pointer to the next descriptor
		BdPtr = (XAxiDma_Bd *)XAxiDma_BdRingNext(RingPtr, BdPtr);
	}
	return 0;
}
