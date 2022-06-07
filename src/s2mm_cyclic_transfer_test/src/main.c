#include "xparameters.h"
#include "xgpio.h"
#include "xaxidma.h"
#include "xil_printf.h"
#include "sleep.h"
#include "trigger/trigger.h"
#include "test_stream_source/test_stream_source.h"
#include "s2mm_transfer/s2mm_transfer.h"

#define DMA_ID XPAR_S2MMDMATRANSFER_0_AXI_DMA_0_DEVICE_ID
#define DMA_BURST_SIZE XPAR_S2MMDMATRANSFER_0_AXI_DMA_0_S2MM_BURST_SIZE
#define DMA_DATA_WIDTH XPAR_S2MMDMATRANSFER_0_AXI_DMA_0_M_AXI_S2MM_DATA_WIDTH

#define TRIGGER_CONFIG_ID XPAR_TRIGGERDETECTOR_0_CFG_0_DEVICE_ID
#define TRIGGER_CTRL_ID XPAR_TRIGGERDETECTOR_0_CTRL_0_DEVICE_ID
#define TRIGGER_COUNTER_CONFIG_ID XPAR_TRIGGERDETECTOR_0_COUNTER_CFG_0_DEVICE_ID

#define TRAFFIC_CTRL_ID XPAR_TESTAXISTREAMSOURCE_0_CTRL_0_DEVICE_ID

#define MANUAL_TRIGGER_ID XPAR_TRIGGERDETECTOR_0_MANUAL_TRIGGER_0_DEVICE_ID
#define MANUAL_TRIGGER_CHANNEL 1

// Function definitions

void InitializeGpio (XGpio *InstPtr, const u32 DeviceId) {
	XGpio_Config *GpioCfgPtr;

	GpioCfgPtr = XGpio_LookupConfig(DeviceId);
	XGpio_CfgInitialize(InstPtr, GpioCfgPtr, GpioCfgPtr->BaseAddress);
}

int main () {
	// Initialize device drivers
	S2mmTransferHierarchy S2mm;
	XGpio ManualTriggerGpio;
	TriggerController Trig;
	StreamSource TrafficGen;

	TriggerControllerInitialize(&Trig, TRIGGER_CONFIG_ID, TRIGGER_CTRL_ID, TRIGGER_COUNTER_CONFIG_ID);
	S2mmInitialize(&S2mm, DMA_ID);
	StreamSourceInitialize(&TrafficGen, TRAFFIC_CTRL_ID);

	InitializeGpio(&ManualTriggerGpio, MANUAL_TRIGGER_ID);

	// Define the acquisition window
	const u32 BufferLength = 1024;//65536;
	const u32 TriggerPosition = 200;

	// Initialize the buffer for receiving data from PL
	u32 *RxBuffer = NULL;

	RxBuffer = malloc(BufferLength * sizeof(u32));
	memset(RxBuffer, 0, BufferLength * sizeof(u32));

	// Create a Dma Bd Ring and map the buffer to it
	S2mmAttachBuffer(&S2mm, (UINTPTR)RxBuffer, BufferLength);

	// Flush the cache before any transfer
	Xil_DCacheFlushRange((UINTPTR)RxBuffer, BufferLength * sizeof(u32));

	// Configure the trigger
	TriggerSetPosition (&Trig, BufferLength, TriggerPosition);
	TriggerSetEnable (&Trig, 0xFFFFFFFF);

	xil_printf("Initialization done\r\n");

	// Start up the input pipeline from back to front
	// Start the DMA receive
	S2mmStartCyclicTransfer(&S2mm);

	// Start the trigger hardware
	TriggerStart(&Trig);

	// Enable the traffic generator
	StreamSourceSetEnable(&TrafficGen, 1);

	// Wait for the receive transfer to complete

	// Apply a manual trigger
	usleep(1);
//	u32 trigtime = 0;
//	while (trigtime++ < 1200);
	XGpio_DiscreteWrite(&ManualTriggerGpio, MANUAL_TRIGGER_CHANNEL, 0x1);

	u32 iter = 0;
	// wait for trigger hardware to go idle
	while (!TriggerGetIdle(&Trig)) iter++; // FIXME polarity seems wrong?

	u32 *BufferHeadPtr = FindStartOfBuffer(&S2mm);
	if (BufferHeadPtr == NULL) {
		xil_printf("ERROR: No buffer head detected\r\n");
	}

	u32 BufferHeadIndex = (((u32)BufferHeadPtr - (u32)RxBuffer) / sizeof(u32)) % BufferLength;

	u32 TriggerDetected = TriggerGetDetected(&Trig);

	xil_printf("Buffer base address: %08x\r\n", RxBuffer);
	xil_printf("Buffer high address: %08x\r\n", ((u32)RxBuffer) + ((BufferLength-1) * sizeof(u32)));
	xil_printf("Length of buffer (words): %d\r\n", BufferLength);
	xil_printf("Index of buffer head: %d\r\n", BufferHeadIndex);
	xil_printf("Trigger position: %d\r\n", TriggerPosition);
	xil_printf("Index of trigger position: %d\r\n", (BufferHeadIndex + TriggerPosition) % BufferLength);
	xil_printf("Detected trigger condition: %08x\r\n", TriggerDetected);

	// Invalidate the cache to ensure acquired data can be read
	Xil_DCacheInvalidateRange((UINTPTR)RxBuffer, BufferLength * sizeof(u32));

	xil_printf("Transfer done\r\n");

	// The traffic generator counters reset when enable is deasserted
	StreamSourceSetEnable(&TrafficGen, 0);

	// Check the buffer to see if data increments like expected
	u32 errors = 0;
	u32 FirstValue = RxBuffer[BufferHeadIndex];
	for (u32 i = 0; i < BufferLength; i++) {
		u32 index = (i + BufferHeadIndex) % BufferLength;
		if (RxBuffer[index] != FirstValue + i) {
			xil_printf("RxBuffer at %08x: %d; expected %d\r\n", (u32)RxBuffer + index*sizeof(u32), RxBuffer[index], FirstValue + i);
			errors++;
		}
	}
	if (errors == 0) {
		xil_printf("All RxBuffer data matched!\r\n");
	}

	// Clean up allocated memory
	S2mmDetachBuffer(&S2mm);
	free(RxBuffer);

	u32 *testptr = malloc(4);
	xil_printf("heap head = %08x?\r\n", (u32)testptr);
	free(testptr);

	xil_printf("done\r\n\r\n");
}
