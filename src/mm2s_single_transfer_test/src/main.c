#include "xparameters.h"
#include "xil_printf.h"
#include"xil_cache.h"
#include "sleep.h"
#include "test_stream_sink/test_stream_sink.h"
#include "mm2s_transfer/mm2s_transfer.h"
#include "xuartps.h"

#define UART_ID XPAR_PS7_UART_0_DEVICE_ID

#define DMA_ID XPAR_MM2SDMATRANSFER_0_AXI_DMA_0_DEVICE_ID
#define DMA_BURST_SIZE XPAR_S2MMDMATRANSFER_0_AXI_DMA_0_S2MM_BURST_SIZE
#define DMA_DATA_WIDTH XPAR_S2MMDMATRANSFER_0_AXI_DMA_0_M_AXI_S2MM_DATA_WIDTH

#define SINK_GPIO_0_ID XPAR_TESTAXISTREAMSINK_0_AXI_GPIO_0_DEVICE_ID
#define SINK_GPIO_1_ID XPAR_TESTAXISTREAMSINK_0_AXI_GPIO_1_DEVICE_ID
#define SINK_GPIO_2_ID XPAR_TESTAXISTREAMSINK_0_AXI_GPIO_2_DEVICE_ID

void UartInitialize (XUartPs *InstPtr, u32 DeviceId) {
	XUartPs_Config *CfgPtr;
	CfgPtr = XUartPs_LookupConfig(DeviceId);
	XUartPs_CfgInitialize(InstPtr, CfgPtr, CfgPtr->BaseAddress);
}

u32 UartGetChar (XUartPs *InstPtr, u8 *CharPtr) {
	return XUartPs_Recv(InstPtr, CharPtr, 1);
}

int main () {
	XUartPs Uart;
	UartInitialize(&Uart, UART_ID);

	Mm2sTransferHierarchy Mm2s;
	Mm2sInitialize(&Mm2s, DMA_ID);

	StreamSink Sink;
	StreamSinkInitialize(&Sink, SINK_GPIO_0_ID, SINK_GPIO_1_ID, SINK_GPIO_2_ID);

	const u32 Alignment = 0x2; // buffer must be aligned to 64-bit boundaries since the DMA as instantiated in hardware does not support dynamic realignment
	const u32 BufferLength = 0x4000000;
	const u32 BufferLengthBytes = BufferLength * sizeof(u32);
	u32 *TxBufferBase = malloc ((BufferLength + Alignment - 1) * sizeof(u32));
	u32 *TxBuffer; // aligned to 64-bit boundary

	if (TxBufferBase == NULL) {
		xil_printf("Error: not enough memory, check lscript.ld to increase the heap size.\r\n");
	}

	if ((((UINTPTR)TxBufferBase) & (Alignment - 1)) == 0) {
		TxBuffer = TxBufferBase;
	} else {
		TxBuffer = (u32*)((((UINTPTR)TxBufferBase) & (Alignment - 1)) + Alignment);
	}

	for (u32 i = 0; i < BufferLength; i++) {
		TxBuffer[i] = i;
	}

	Xil_DCacheFlushRange((UINTPTR)TxBuffer, BufferLengthBytes);

	Mm2sCreateRing(&Mm2s, BufferLengthBytes);

//#define SINGLE_SHOT
//#define CYCLIC
#define REPEATED

#if defined(SINGLE_SHOT)

	StreamSinkSetLastCount(&Sink, Mm2s.NumBds);
	StreamSinkStart(&Sink);

	u32 CyclicMode = 0;
	Mm2sStartTransfer(&Mm2s, (UINTPTR)TxBuffer, BufferLengthBytes, CyclicMode);

	// Wait until all of the bytes have been transferred
	while (!Mm2sTransferDone(&Mm2s));
	while (!StreamSinkGetIdle(&Sink)); // make sure the sink has received a tlast beat

	u32 Beats = StreamSinkGetBeatCount(&Sink);
	u32 Misses = StreamSinkGetMissCount(&Sink);
	u32 Errors = StreamSinkGetErrorCount(&Sink);

	xil_printf("%08x beats counted, %08x expected\r\n", Beats, BufferLengthBytes / sizeof(u32));
	xil_printf("%08x misses counted, 0 expected\r\n", Misses);
	xil_printf("%08x errors counted, 0 expected\r\n", Errors);

#elif defined(CYCLIC)
	// FIXME: Has various problems.

	// let the sink counters continue running, resetting only on successive starts
	StreamSinkSetLastCount(&Sink, 0);
	StreamSinkStart(&Sink);

	u32 CyclicMode = 1;

	Mm2sStartTransfer(&Mm2s, (UINTPTR)TxBuffer, BufferLengthBytes, CyclicMode);
	// Keep resubmitting the same buffer until the user presses any key, then halt the transfer
	u8 Char = 0;
	u32 Done = 0;
	u32 Misses = 0;
	u32 Beats = 0;
	u32 Acc = 0;
	u32 Most = 0;
	u32 Blocks = 0;
	do {
		Beats = StreamSinkGetBeatCount(&Sink); // only ever captures the first frame sent
		Misses = StreamSinkGetMissCount(&Sink);
		Blocks = Mm2sCyclicResubmit(&Mm2s);
		Acc += Blocks;
		if (Blocks > Most) Most = Blocks;
		Done = XUartPs_Recv(&Uart, &Char, 1);
	} while (!Done);

	Mm2sCyclicModeCleanup(&Mm2s);

	xil_printf("%08x / %08x stream uptime maintained (integer rollover may have occurred)\r\n", Misses, Beats);
	// stream sink currently doesn't support testing cyclic mode data coherency, but we can check bandwidth usage

#elif defined(REPEATED)
	u32 Repeats = 10;

	StreamSinkSetLastCount(&Sink, Repeats * Mm2s.NumBds);
	StreamSinkStart(&Sink);
	u32 CyclicMode = 0;

	Mm2sStartTransfer(&Mm2s, (UINTPTR)TxBuffer, BufferLengthBytes, CyclicMode);
	u32 BlocksResubmitted = 0;

	do {
		BlocksResubmitted += Mm2sResubmit(&Mm2s);
	} while (BlocksResubmitted < (Repeats - 1) * Mm2s.NumBds);

	// Wait until all of the bytes have been transferred
	while (!Mm2sTransferDone(&Mm2s));
	while (!StreamSinkGetIdle(&Sink)); // make sure the sink has received a tlast beat

	u32 Beats = StreamSinkGetBeatCount(&Sink);
	u32 Misses = StreamSinkGetMissCount(&Sink);
	u32 Errors = StreamSinkGetErrorCount(&Sink);

	xil_printf("%08x beats counted, %08x expected\r\n", Beats, Repeats * BufferLengthBytes / sizeof(u32));
	xil_printf("%08x misses counted, 0 expected\r\n", Misses);
	// expect one discontinuity when the buffer starts being replayed
	xil_printf("%08x errors counted, %d expected\r\n", Errors, BlocksResubmitted / Mm2s.NumBds);

#endif


	Mm2sDestroyRing(&Mm2s);
	free(TxBufferBase);
}
