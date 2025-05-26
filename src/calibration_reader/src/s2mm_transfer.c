#include "s2mm_transfer.h"
#include "xil_printf.h"
#include <stdlib.h>

void AllocateBdSpace();

#define RoundUpDivide(a, b) ((a / b) + (a % b != 0))

/****************************************************************************/
/**
* Initializes the stream to memory-mapped transfer engine
*
* @param	InstPtr is the device handler instance to operate on.
* @param	DmaDeviceId is the device ID for the AXI DMA IP used to perform the transfers, pulled from xparameters
*
* @note Max burst size is stored to later be used in computing DMA transfer block sizes. No buffer is attached yet and the descriptor space is not yet allocated.
*
*****************************************************************************/
void S2mmInitialize (S2mmTransferHierarchy *InstPtr, const u32 DmaDeviceId) {
	XAxiDma_Config *CfgPtr;

	CfgPtr = XAxiDma_LookupConfig(DmaDeviceId);
	XAxiDma_CfgInitialize(&(InstPtr->Dma), CfgPtr);
	XAxiDma_IntrDisable(&(InstPtr->Dma), XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

	InstPtr->MaxBurstLengthBytes = CfgPtr->S2MmBurstSize * CfgPtr->S2MmDataWidth / (8);
	InstPtr->NumBds = 0;
	InstPtr->BdSpace = NULL;
	InstPtr->BufferBaseAddr = NULL;
	InstPtr->BufferLength = 0;
}

/****************************************************************************/
/**
* Allocates memory for block descriptors and creates and initializes the block descriptors for a specific buffer address and size
*
* @param	InstPtr is the device handler instance to operate on.
* @param	DmaDeviceId is the device ID for the AXI DMA IP used to perform the transfers, pulled from xparameters
*
* @return	none
*
* @note The same buffer can be reused, however if either the size or address is changed, new block descriptors need to be allocated.
*       Approximately 16 * BufferLength * sizeof(u32) / MaxBurstLengthBytes words are allocated, with additional padding space to ensure that
*       the block descriptor alignment requirement is satisfied.
*       Blocks are used to describe a single burst, and their length is set to the max burst size of the AXI DMA. This is not an inherent
*       requirement imposed by the AXI DMA, and theoretically any block size could be used. However, some minimum burst size exists such that
*       AXI4-full protocol overhead significantly affects the bandwidth of the transfer. It may be possible to use short bursts as long as the
*       following burst is long enough for the scatter gather interface to keep up.
*       FIXME: It may be possible to use only a single block descriptor with cyclic mode. It should be determined whether the DMA engine will
*              repeatedly fetch the same block.
*		FIXME: The minimum block size that can be consistently supported should be experimentally determined.
*		FIXME: Currently, the DMA core must be reset in order to cancel any blocks still pending after the final block in an arbitrarily-long
*		       transfer has been received. These blocks must be submitted to hardware in order since it is unknown while the final block is in flight
*		       whether it is actually final or not, it is unknown whether the next acquisition will reuse the same buffer, and the blocks must be
*		       available to hardware to prefetch before the block before them is complete.
*		FIXME: At time of writing, buffer length is restricted to values of N * MaxBurstLengthBytes, where N is an integer and MaxBurstLengthBytes
*		       is 256x64/8=1024 bytes (BurstSize x DataWidth bits).
*		       If buffer length doesn't meet this requirement, S2mmFindStartOfBuffer may not return the correct address in some edge cases.
*		FIXME: It should be explored whether multiple buffers can easily be attached and switched out (perhaps by taking advantage of multiple Rx rings)
*			   in order to allow an acquisition to occur at the same time as a previous acquisition is being transferred to the host.
*		FIXME: Errors should be returned in several bad-status cases (eg if malloc or some XAxiDma API call fail).
*****************************************************************************/
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
	InstPtr->BufferLength = BufferLength;

	if ((InstPtr->BufferLength & 0x7ff) != 0) {
		xil_printf("ERROR: Buffer length must be an integer multiple of 0x800\r\n");
	}

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
		xil_printf("ERROR: XAxiDma_BdRingSetCoalesce failed for s2mm instance 0x%08x\r\n", InstPtr);
	}

	// Set up the BD ring to use the allocated memory space
	Status = XAxiDma_BdRingCreate(RingPtr, (UINTPTR)(BdSpaceAligned), (UINTPTR)(BdSpaceAligned), XAXIDMA_BD_MINIMUM_ALIGNMENT, InstPtr->NumBds);
	if (Status != XST_SUCCESS) {
		xil_printf("ERROR: XAxiDma_BdRingCreate failed for s2mm instance 0x%08x\r\n", InstPtr);
	}

	// use a all-zero bd as template
	XAxiDma_BdClear(&BdTemplate);
	Status = XAxiDma_BdRingClone(RingPtr, &BdTemplate);
	if (Status != XST_SUCCESS) {
		xil_printf("ERROR: XAxiDma_BdRingClone failed for s2mm instance 0x%08x\r\n", InstPtr);
	}

	//allocate descriptors to fill the entire space
	FreeBdCount = XAxiDma_BdRingGetFreeCnt(RingPtr);
	Status = XAxiDma_BdRingAlloc(RingPtr, FreeBdCount, &BdPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("ERROR: XAxiDma_BdRingAlloc failed for s2mm instance 0x%08x\r\n", InstPtr);
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
			xil_printf("ERROR: XAxiDma_BdSetBufAddr failed for s2mm instance 0x%08x, BD %d\r\n", InstPtr, Index);
		}

		Status = XAxiDma_BdSetLength(BdCurPtr, BdLength, RingPtr->MaxTransferLen);
		if (Status != XST_SUCCESS) {
			xil_printf("ERROR: XAxiDma_BdSetBufAddr failed for s2mm instance 0x%08x, BD %d\r\n", InstPtr, Index);
		}

		XAxiDma_BdSetCtrl(BdCurPtr, 0);
		XAxiDma_BdSetId(BdCurPtr, BdBufferPtr);

		BdBufferPtr += BdLength;
		BdCurPtr = (XAxiDma_Bd *)XAxiDma_BdRingNext(RingPtr, BdCurPtr);
	}

	Status = XAxiDma_BdRingToHw(RingPtr, FreeBdCount, BdPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("ERROR: XAxiDma_BdRingToHw failed for s2mm instance 0x%08x\r\n", InstPtr);
	}
}

/****************************************************************************/
/**
* Cleans up information stored in the device handler about the attached buffer and frees memory allocated for the block descriptors
*
* @param	InstPtr is the device handler instance to operate on.
*
* @return	none
*
* @note Function currently does not work due to a data abort exception being thrown when freeing the BdSpace. Cause is unknown at this time.
*
*****************************************************************************/
void S2mmCleanup(S2mmTransferHierarchy *InstPtr) {
	// Spin down the hardware.
	XAxiDma_Pause(&(InstPtr->Dma));
	XAxiDma_Reset(&(InstPtr->Dma));
	while (!XAxiDma_ResetIsDone(&(InstPtr->Dma)));

	// Clean up Bds and deallocate the BdSpace
	InstPtr->BufferBaseAddr = NULL;
	InstPtr->BufferLength = 0;
	InstPtr->NumBds = 0;
	// FIXME: throws data abort exception
	// Oddly, issue seems to not occur in 2024.1 vitis classic
	free((void*)(InstPtr->BdSpace));
	InstPtr->BdSpace = NULL;
}

/****************************************************************************/
/**
* Initiates the acquisition by submitting the already-configured BdRing to hardware.
*
* @param	InstPtr is the device handler instance to operate on.
*
* @return	none
*
* @note This function should be called prior to enabling all upstream IP, as ready can remain high while the transfer is inactive,
* 	    allowing data to flow into buffers, corrupting the first few samples of an acquisition.
* 	    FIXME: Consider whether cyclic mode should be set when the ring is first being configured.
* 	    FIXME: Return error instead of printing here.
*
*****************************************************************************/
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

/****************************************************************************/
/**
* After an acquisition has completed, iterates over the block descriptors to find the one where the EOF status bit is set, indicating
* 	the position of the stream beat where tlast was asserted, in order to find the index in the buffer for the first sample.
*
* @param	InstPtr is the device handler instance to operate on.
* @return	Address of the start of the buffer.
*
* @note Samples are laid out sequentially in memory. In order to iterate across the buffer, a loop should start at StartOfBuffer, iterate until it hits
* 		the top of the buffer address space as determined by InstPtr->BufferBaseAddr + InstPtr->BufferLength, roll over to InstPtr->BufferBaseAddr, then
* 		iterate back up to StartOfBuffer.
* 		For example, a 10-sample buffer where the last beat fell on index 3 would look as follows: 6789012345
*		Scatter gather descriptors are further described here: https://docs.xilinx.com/r/en-US/pg021_axi_dma/Scatter-Gather-Descriptor
*		S2mm status is of particular interest here: https://docs.xilinx.com/r/en-US/pg021_axi_dma/S2MM_STATUS-S2MM-Status
*
*****************************************************************************/
u32 *S2mmFindStartOfBuffer (S2mmTransferHierarchy *InstPtr) {
	XAxiDma_BdRing *RingPtr = XAxiDma_GetRxRing(&(InstPtr->Dma));
	XAxiDma_Bd *BdPtr;
	u32 ActualLength;
	u32 *StartOfBuffer = NULL;

	BdPtr = (XAxiDma_Bd*)RingPtr->FirstBdAddr; // this is kind of weird. FIXME find out why this value isn't the one getting returned from BdRingFree

//	NumBd = XAxiDma_BdRingFromHw(RingPtr, XAXIDMA_ALL_BDS, &BdPtr); // potentially use this to clear the processed status fields, if NumBd > 0 then RXEOF exists somewhere, maybe in the HwTail?

	for (u32 i = 0; i < InstPtr->NumBds; i++) {
		Xil_DCacheInvalidateRange((UINTPTR)BdPtr, XAXIDMA_BD_NUM_WORDS * sizeof(u32));

		u32 Status = XAxiDma_BdGetSts(BdPtr);
		// RXEOF bit high indicates that tlast occured in that block, ActualLength indicates the tlast position
		// FIXME: determine whether using the DMA with 64-bit wide AXI master interfaces obscures the position of tlast.
		// Check if ActualLength shows up as both even and odd. Using the source monitor's traffic generator with
		// even- and odd-valued level triggers should indicate this.
		if (Status & XAXIDMA_BD_STS_RXEOF_MASK) {
			ActualLength = XAxiDma_BdGetActualLength(BdPtr, ((InstPtr->MaxBurstLengthBytes*2)-1));
			xil_printf("Last beat found:\r\n");
			xil_printf("  BD base address: %08x\r\n", XAxiDma_BdGetBufAddr(BdPtr));
			xil_printf("  BD actual length: %08x\r\n", ActualLength);
			StartOfBuffer = (u32*)(XAxiDma_BdGetBufAddr(BdPtr) + ActualLength);

			// wrap around to the start of the buffer if the calculated address of start goes past the end.
			// only expected if the final block is completely full, eg ActualLength == BdLength
			// FIXME: this will fail if bufferLength does not meet the requirement of being an integer multiple of the max burst length
			if (StartOfBuffer >= InstPtr->BufferBaseAddr + InstPtr->BufferLength) {
				StartOfBuffer -= InstPtr->BufferLength;
			}

			return StartOfBuffer;
		}

		// Advance the pointer to the next descriptor
		BdPtr = (XAxiDma_Bd *)XAxiDma_BdRingNext(RingPtr, BdPtr);
	}
	return NULL;
}
