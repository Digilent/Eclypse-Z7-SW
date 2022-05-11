#include "xparameters.h"
#include "xgpio.h"
#include "xaxidma.h"
#include "xil_types.h"
#include "xil_cache.h"
#include "xil_printf.h"

#define NUM_DMA XPAR_XAXIDMA_NUM_INSTANCES
#define NUM_GPIO XPAR_XGPIO_NUM_INSTANCES

#define DDR_BASE_ADDR       XPAR_PS7_DDR_0_S_AXI_BASEADDR
#define MEM_BASE_ADDR		(DDR_BASE_ADDR + 0x1000000)
#define RX0_BD_SPACE_BASE	(MEM_BASE_ADDR)
#define RX0_BD_SPACE_HIGH	(MEM_BASE_ADDR + 0x0000FFFF)
#define RX1_BD_SPACE_BASE	(MEM_BASE_ADDR + 0x00010000)
#define RX1_BD_SPACE_HIGH	(MEM_BASE_ADDR + 0x0001FFFF)
#define RX2_BD_SPACE_BASE	(MEM_BASE_ADDR + 0x00020000)
#define RX2_BD_SPACE_HIGH	(MEM_BASE_ADDR + 0x0002FFFF)
#define RX3_BD_SPACE_BASE	(MEM_BASE_ADDR + 0x00030000)
#define RX3_BD_SPACE_HIGH	(MEM_BASE_ADDR + 0x0003FFFF)
#define RX01_BUFFER_BASE    (MEM_BASE_ADDR + 0x00040000)
#define RX23_BUFFER_BASE    (MEM_BASE_ADDR + 0x00140000)

#define RX0_DEVICEID XPAR_DOUBLE_BUFFER_INST0_AXI_DMA_0_DEVICE_ID
#define RX1_DEVICEID XPAR_DOUBLE_BUFFER_INST0_AXI_DMA_1_DEVICE_ID
#define RX2_DEVICEID XPAR_DOUBLE_BUFFER_INST1_AXI_DMA_0_DEVICE_ID
#define RX3_DEVICEID XPAR_DOUBLE_BUFFER_INST1_AXI_DMA_1_DEVICE_ID
#define RX01_SOURCE_DEVICEID XPAR_STREAM_SOURCE_GPIO_AXI_GPIO_0_DEVICE_ID
#define RX23_SOURCE_DEVICEID XPAR_STREAM_SOURCE1_GPIO_AXI_GPIO_0_DEVICE_ID

#define TRANSFER_LENGTH_WORDS (256)
#define TRANSFER_LENGTH (TRANSFER_LENGTH_WORDS * sizeof(u32))
#define FRAMES (1024)
//#define FRAMES (16)
#define BUFFER_SIZE_WORDS (TRANSFER_LENGTH_WORDS * FRAMES)
#define BUFFER_SIZE (TRANSFER_LENGTH * FRAMES)
// buffer_size = 0x40000

#define ENABLE_BIT 1
#define FREERUN_BIT 2
#define SetGpio(instptr, data) XGpio_DiscreteWrite(instptr, 1, data)

//XAXIDMA_BD_MINIMUM_ALIGNMENT

void GpioInitialize (XGpio gpios[NUM_GPIO]) {
	XGpio_Config *cfgptr;
	for (u32 device_id = 0; device_id < NUM_GPIO; device_id++) {
		cfgptr = XGpio_LookupConfig(device_id);
		XGpio_CfgInitialize(&(gpios[device_id]), cfgptr, cfgptr->BaseAddress);
		SetGpio(&(gpios[device_id]), 0);
	}
}

void DmaInitialize (XAxiDma dmas[NUM_DMA]) {
	XAxiDma_Config *cfgptr;
	int Status;
	for (u32 device_id = 0; device_id < NUM_DMA; device_id++) {
		cfgptr = XAxiDma_LookupConfig(device_id);
		Status = XAxiDma_CfgInitialize(&(dmas[device_id]), cfgptr);
		if (Status != XST_SUCCESS) {
			xil_printf("XAxiDma_CfgInitialize failed for dma %08x\r\n", &(dmas[device_id]));
		}
		XAxiDma_IntrDisable(&(dmas[device_id]), XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	}
}

void SgInitialize (XAxiDma *instptr, u32 num_bds, u32 bd_space_base, u32 bd_space_high, UINTPTR buffer_base, u32 stride) {
	XAxiDma_BdRing *ringptr;
	int coalesce = 1;
	int delay = 0;
	XAxiDma_Bd template;
	XAxiDma_Bd *BdPtr;
	XAxiDma_Bd *BdCurPtr;
	u32 BdCount;
	u32 FreeBdCount;
	UINTPTR RxBufferPtr;
	int Index;
	int Status;

	ringptr = XAxiDma_GetRxRing(instptr);
	XAxiDma_BdRingIntDisable(ringptr, XAXIDMA_IRQ_ALL_MASK);
	Status = XAxiDma_BdRingSetCoalesce(ringptr, coalesce, delay);
	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_BdRingSetCoalesce failed for dma 0x%08x\r\n", instptr);
	}

//	BdCount = XAxiDma_BdRingCntCalc(XAXIDMA_BD_MINIMUM_ALIGNMENT, RX_BD_SPACE_HIGH - RX_BD_SPACE_BASE + 1);
	BdCount = num_bds;
	int MaxBdCount = XAxiDma_BdRingCntCalc(XAXIDMA_BD_MINIMUM_ALIGNMENT, bd_space_high - bd_space_base + 1);
	if (BdCount > MaxBdCount) {
		xil_printf("Error: memory allocated to Rx BD storage too small\r\n");
	}
	Status = XAxiDma_BdRingCreate(ringptr, bd_space_base, bd_space_base, XAXIDMA_BD_MINIMUM_ALIGNMENT, BdCount);
	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_BdRingCreate failed for dma 0x%08x\r\n", instptr);
	}

	// use a all-zero bd as template
	XAxiDma_BdClear(&template);
	Status = XAxiDma_BdRingClone(ringptr, &template);
	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_BdRingClone failed for dma 0x%08x\r\n", instptr);
	}

	FreeBdCount = XAxiDma_BdRingGetFreeCnt(ringptr);
	Status = XAxiDma_BdRingAlloc(ringptr, FreeBdCount, &BdPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_BdRingAlloc failed for dma 0x%08x\r\n", instptr);
	}

	BdCurPtr = BdPtr;
	RxBufferPtr = buffer_base;

	for (Index = 0; Index < FreeBdCount; Index++) {
		u32 BdId = RxBufferPtr;
		u32 BdLength = TRANSFER_LENGTH;
		u32 BdCtrl = 0;

		Status = XAxiDma_BdSetBufAddr(BdCurPtr, RxBufferPtr);
		if (Status != XST_SUCCESS) {
			xil_printf("XAxiDma_BdSetBufAddr failed for dma 0x%08x, BD %d\r\n", instptr, Index);
		}
		Status = XAxiDma_BdSetLength(BdCurPtr, BdLength, ringptr->MaxTransferLen);
		if (Status != XST_SUCCESS) {
			xil_printf("XAxiDma_BdSetBufAddr failed for dma 0x%08x, BD %d\r\n", instptr, Index);
		}

		XAxiDma_BdSetCtrl(BdCurPtr, BdCtrl);
		XAxiDma_BdSetId(BdCurPtr, BdId);

//		xil_printf("BD 0x%08x set up with length %d and address 0x%08x\r\n", BdId, BdLength, RxBufferPtr);

		RxBufferPtr += BdLength + stride;
		BdCurPtr = (XAxiDma_Bd *)XAxiDma_BdRingNext(ringptr, BdCurPtr);
	}

	Status = XAxiDma_BdRingToHw(ringptr, FreeBdCount, BdPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_BdRingToHw failed for dma 0x%08x\r\n", instptr);
	}

	xil_printf("Initialized %d BDs for dma %08x\r\n", FreeBdCount, instptr);
}

void SgStartReceiveTransfer (XAxiDma *InstPtr) {
	XAxiDma_BdRing *RingPtr;
	RingPtr = XAxiDma_GetRxRing(InstPtr);
	int Status;

	Status = XAxiDma_BdRingStart(RingPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_BdRingStart failed %d for dma 0x%08x\r\n", Status, InstPtr);
	}
}

#define TRIGGER_LEVEL 0x200000

u32 CheckLast(XAxiDma_Bd *BdPtr) {
	// placeholder; compare data at the front of the buffer to a specified level
	u32 *DataBuffer = XAxiDma_BdGetBufAddr(BdPtr);
	if (*DataBuffer >= TRIGGER_LEVEL) {
		// test this by placing it after the end of a transfer?
		return 1;
	}
	return 0;
}

u32 SgProcessBlocks (XAxiDma *InstPtr, u32 Resubmit) {
	XAxiDma_BdRing *RingPtr;
	XAxiDma_Bd *BdPtr;
	int ProcessedBdCount;
	int Status;

	RingPtr = XAxiDma_GetRxRing(InstPtr);

	ProcessedBdCount = XAxiDma_BdRingFromHw(RingPtr, XAXIDMA_ALL_BDS, &BdPtr);

	if (ProcessedBdCount > 0) {
		Status = XAxiDma_BdRingFree(RingPtr, ProcessedBdCount, BdPtr);
		if (Status != XST_SUCCESS) {
			xil_printf("XAxiDma_BdRingFree failed %d for dma 0x%08x\r\n", Status, InstPtr);
		}

		// reallocate the same number of BDs
		if (Resubmit && !CheckLast(BdPtr)) {
			Status = XAxiDma_BdRingAlloc(RingPtr, ProcessedBdCount, &BdPtr);
			if (Status != XST_SUCCESS) {
				xil_printf("XAxiDma_BdRingAlloc failed for dma 0x%08x\r\n", InstPtr);
			}
		}
	}

	return ProcessedBdCount;
}



u32 SgProcessBlocks_UntilLast (XAxiDma *InstPtr, u32 Resubmit, u32 *ProcessedBdCountPtr, u32 **LastBlockPtr) {
	XAxiDma_BdRing *RingPtr;
	XAxiDma_Bd *BdPtr;
//	int ProcessedBdCount;
	int Status;

	RingPtr = XAxiDma_GetRxRing(InstPtr);

	*ProcessedBdCountPtr = XAxiDma_BdRingFromHw(RingPtr, XAXIDMA_ALL_BDS, &BdPtr);

	if (*ProcessedBdCountPtr > 0) {
		Status = XAxiDma_BdRingFree(RingPtr, *ProcessedBdCountPtr, BdPtr);
		if (Status != XST_SUCCESS) {
			xil_printf("XAxiDma_BdRingFree failed %d for dma 0x%08x\r\n", Status, InstPtr);
		}

		if (CheckLast(BdPtr)) {
			*LastBlockPtr = XAxiDma_BdGetBufAddr(BdPtr);
			return 1;
		} else if (Resubmit) {
			// reallocate the same number of BDs
			Status = XAxiDma_BdRingAlloc(RingPtr, *ProcessedBdCountPtr, &BdPtr);
			if (Status != XST_SUCCESS) {
				xil_printf("XAxiDma_BdRingAlloc failed for dma 0x%08x\r\n", InstPtr);
			}
//			XAxiDma_DumpBd(BdPtr);
			Status = XAxiDma_BdRingToHw(RingPtr, *ProcessedBdCountPtr, BdPtr);
			if (Status != XST_SUCCESS) {
				xil_printf("XAxiDma_BdRingToHw failed for dma 0x%08x\r\n", InstPtr);
			}
		}
	}

	return 0;
}

void SgWaitUntilDone (XAxiDma *instptr, u32 frames) {
	XAxiDma_BdRing *ringptr;
	XAxiDma_Bd *BdPtr;

	int Count = 0;
	int ProcessedBdCount;
	int Status;

	ringptr = XAxiDma_GetRxRing(instptr);

	while (Count < frames) {
		ProcessedBdCount = XAxiDma_BdRingFromHw(ringptr, XAXIDMA_ALL_BDS, &BdPtr);

		if (ProcessedBdCount > 0) {
			xil_printf("processed %d bds from dma 0x%08x:\r\n", ProcessedBdCount, instptr);
			XAxiDma_DumpBd(BdPtr);

			Status = XAxiDma_BdRingFree(ringptr, ProcessedBdCount, BdPtr);
			if (Status != XST_SUCCESS) {
				xil_printf("XAxiDma_BdRingFree failed %d for dma 0x%08x\r\n", Status, instptr);
			}

			Count += ProcessedBdCount;
		}

	}
}

void BufferInitialize (u32 *receive_buffer) {
	memset(receive_buffer, 0, BUFFER_SIZE);
	Xil_DCacheFlushRange((UINTPTR)receive_buffer, BUFFER_SIZE);
}

//#define TEST_TWO

int main () {
	XGpio gpio[NUM_GPIO];
	XAxiDma dma[NUM_DMA];

	GpioInitialize(gpio);
	DmaInitialize(dma);

	xil_printf("start\r\n");

	BufferInitialize(RX01_BUFFER_BASE);
	xil_printf("rx01 buffer address: %08x\r\n", RX01_BUFFER_BASE);
	SgInitialize (&(dma[0]), FRAMES / 2, RX0_BD_SPACE_BASE, RX0_BD_SPACE_HIGH, RX01_BUFFER_BASE, TRANSFER_LENGTH);
	SgInitialize (&(dma[1]), FRAMES / 2, RX1_BD_SPACE_BASE, RX1_BD_SPACE_HIGH, RX01_BUFFER_BASE + TRANSFER_LENGTH, TRANSFER_LENGTH);

	#ifdef TEST_TWO
	BufferInitialize(RX23_BUFFER_BASE);
	xil_printf("rx23 buffer address: %08x\r\n", RX23_BUFFER_BASE);
	SgInitialize (&(dma[2]), FRAMES / 2, RX2_BD_SPACE_BASE, RX2_BD_SPACE_HIGH, RX23_BUFFER_BASE, TRANSFER_LENGTH);
	SgInitialize (&(dma[3]), FRAMES / 2, RX3_BD_SPACE_BASE, RX3_BD_SPACE_HIGH, RX23_BUFFER_BASE + TRANSFER_LENGTH, TRANSFER_LENGTH);
#endif

	SgStartReceiveTransfer(&(dma[RX0_DEVICEID]));
	SgStartReceiveTransfer(&(dma[RX1_DEVICEID]));

#ifdef TEST_TWO
	SgStartReceiveTransfer(&(dma[RX2_DEVICEID]));
	SgStartReceiveTransfer(&(dma[RX3_DEVICEID]));
#endif

	// start the traffic generators
	SetGpio(&(gpio[RX01_SOURCE_DEVICEID]), FREERUN_BIT | ENABLE_BIT);
#ifdef TEST_TWO
	SetGpio(&(gpio[RX23_SOURCE_DEVICEID]), FREERUN_BIT | ENABLE_BIT);
#endif

	// handle a receive operation on the DMA0/DMA1 pair.
	u32 count = 0;
	u32 done = 0;
	u32 highest = 0;
	u32 *LastBlockPtr = 0;
	u32 ids[2] = {RX0_DEVICEID, RX1_DEVICEID};
	do {
		done = 0;
		for (u32 i = 0; i < 2; i++) {
//			u32 BlocksProcessed = SgProcessBlocks(&dma[ids[i]], 1);
			u32 BlocksProcessed;
			if (SgProcessBlocks_UntilLast(&dma[ids[i]], 1, &BlocksProcessed, &LastBlockPtr)) {
				done = 1;
			}
			count += BlocksProcessed;
			if (BlocksProcessed > highest) {
				highest = BlocksProcessed;
			}
		}
	} while (!done);

	xil_printf("Most blocks received at once: %d\r\n", highest);

	Xil_DCacheInvalidateRange((UINTPTR)RX01_BUFFER_BASE, BUFFER_SIZE);
#ifdef TEST_TWO
	Xil_DCacheInvalidateRange((UINTPTR)RX23_BUFFER_BASE, BUFFER_SIZE);
#endif

	for (int i = 0; i < BUFFER_SIZE_WORDS; i++) {
		if (((u32*)RX01_BUFFER_BASE)[i] != TRIGGER_LEVEL + i) {
			xil_printf("first difference at rx01[%d] = %d\r\n", i, ((u32*)RX01_BUFFER_BASE)[i]);
			break;
		}
	}
	xil_printf("%d words received in rx01\r\n", BUFFER_SIZE_WORDS);

#ifdef TEST_TWO
	for (int i = 0; i < BUFFER_SIZE_WORDS; i++) {
		if (((u32*)RX23_BUFFER_BASE)[i] != TRIGGER_LEVEL + i) {
			xil_printf("first difference at rx23[%d] = %d\r\n", i, ((u32*)RX23_BUFFER_BASE)[i]);
			break;
		}
	}
	xil_printf("%d words received in rx23\r\n", BUFFER_SIZE_WORDS);
#endif

	xil_printf("done\r\n");
}
