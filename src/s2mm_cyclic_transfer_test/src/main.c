#include "xparameters.h"
#include "xaxidma.h"
#include "xil_printf.h"
#include "sleep.h"
#include "trigger.h"
#include "test_stream_source.h"
#include "s2mm_transfer.h"
#include "zmod_scope_axi_configuration.h"
#include "manual_trigger.h"
#include "xstatus.h"
#include "userregisters.h"

#define DMA_ID XPAR_ZMODSCOPE_PORTA_S2MMDMATRANSFER_0_AXI_DMA_0_DEVICE_ID
#define DMA_BURST_SIZE XPAR_ZMODSCOPE_PORTA_S2MMDMATRANSFER_0_AXI_DMA_0_S2MM_BURST_SIZE
#define DMA_DATA_WIDTH XPAR_ZMODSCOPE_PORTA_S2MMDMATRANSFER_0_AXI_DMA_0_M_AXI_S2MM_DATA_WIDTH

#define TRIGGER_CTRL_BASEADDR XPAR_ZMODSCOPE_PORTA_TRIGGERDETECTOR_0_TRIGGERCONTROL_0_S_AXI_CONTROL_BASEADDR
#define SOURCE_MONITOR_BASEADDR XPAR_ZMODSCOPE_PORTA_AXISTREAMSOURCEMONITOR_0_AXISTREAMSOURCEMONIT_0_S_AXI_CONTROL_BASEADDR
#define MANUAL_TRIGGER_BASEADDR XPAR_ZMODSCOPE_PORTA_TRIGGERGENERATOR_MANUALTRIGGER_0_S_AXI_CONTROL_BASEADDR
#define SCOPE_BASEADDR XPAR_ZMODSCOPE_PORTA_ZMODSCOPEFRONTEND_0_ZMODSCOPEAXICONFIGUR_0_BASEADDR
#define LEVELTRIGGER_BASEADDR XPAR_ZMODSCOPE_PORTA_TRIGGERGENERATOR_USERREGISTERS_0_S_AXI_CONTROL_BASEADDR

#define ZMOD_SCOPE_RESOLUTION 10
#define ZMOD_SCOPE_SAMPLE_RATE 125000000

// Function definitions

u16 ChannelData(u8 channel, u32 data, u8 resolution)
{
	//	Channel1 -> cDataAxisTdata[31:16]
	//	Channel2 -> cDataAxisTdata[15:0]
	return (channel ? (data >> (16 - resolution)) : (data >> (32 - resolution))) & ((1 << resolution) - 1);
}

u32 ToSigned(u32 value, u8 noBits) {
	// align value to bit 31 (left justify), to preserve sign
	value <<= 32 - noBits;

	int32_t sValue = (int32_t)value;

	// align value to bit 0 (right justify)
	sValue >>= (32 - noBits);

	return sValue;
}

float GetVoltFromSignedRaw(s32 raw, u8 gain, u8 resolution) {
	#define IDEAL_RANGE_ADC_LOW 25.0f
	#define IDEAL_RANGE_ADC_HIGH 1.0f
	float vMax = gain ? IDEAL_RANGE_ADC_HIGH : IDEAL_RANGE_ADC_LOW;
	float fval = (float)raw * vMax / (float)(1<<(resolution - 1));
	return fval;
}

float RawDataToVolts (u32 data, u8 channel, u8 resolution, u8 gain) {
	u16 channel_data = ChannelData(channel, data, resolution);
	int SignedData = ToSigned(channel_data, resolution);
	return GetVoltFromSignedRaw(SignedData, gain, resolution);
}

typedef struct {
	u8 Ch1Range; // 0 = Low Gain; 1 = High Gain
	u8 Ch2Range; // 0 = Low Gain; 1 = High Gain
	u8 Ch1Coupling; // 0 = AC; 1 = DC
	u8 Ch2Coupling; // 0 = AC; 1 = DC
} ZmodScopeRelayConfig;

void WriteZmodScopeRelayConfig(ZmodScope *InstPtr, ZmodScopeRelayConfig Config) {
	u32 ScopeConfig = 0;
	ScopeConfig |= Config.Ch1Range * AXI_ZMOD_SCOPE_CONFIG_CHANNEL_1_GAIN_SELECT_MASK;
	ScopeConfig |= Config.Ch2Range * AXI_ZMOD_SCOPE_CONFIG_CHANNEL_2_GAIN_SELECT_MASK;
	ScopeConfig |= Config.Ch1Coupling * AXI_ZMOD_SCOPE_CONFIG_CHANNEL_1_COUPLING_SELECT_MASK;
	ScopeConfig |= Config.Ch2Coupling * AXI_ZMOD_SCOPE_CONFIG_CHANNEL_2_COUPLING_SELECT_MASK;
//	ScopeConfig |= TestMode * AXI_ZMOD_SCOPE_CONFIG_TEST_MODE_MASK;
	ZmodScope_SetConfig(InstPtr, ScopeConfig);
}

typedef struct InputPipeline {
	S2mmTransferHierarchy S2mm;
	ManualTrigger Man;
	TriggerControl Trig;
	AxiStreamSourceMonitor TrafficGen;
	ZmodScope Scope;
	UserRegisters LevelTrigger;
} InputPipeline;

XStatus MinMaxAcquisition (InputPipeline *InstPtr, const ZmodScopeRelayConfig Relays) {
	// push the same signal into both ports.
	// a manual trigger acquisition is performed.
	// min and max values are calculated for each channel and printed to console.

	// Initialize device drivers
	S2mmTransferHierarchy *S2mmPtr = &(InstPtr->S2mm);
	ManualTrigger *ManPtr = &(InstPtr->Man);
	TriggerControl *TrigPtr = &(InstPtr->Trig);
	AxiStreamSourceMonitor *TrafficGenPtr = &(InstPtr->TrafficGen);
	ZmodScope *ScopePtr = &(InstPtr->Scope);

	// Define the acquisition window
	const u32 BufferLength = 0x1000;
	const u32 TriggerPosition = BufferLength / 4;
	float SamplePeriod_ns = 8.0f;
	float BufferPeriod_ns = SamplePeriod_ns * BufferLength;

	// Initialize the buffer for receiving data from PL
	u32 *RxBuffer = NULL;

	RxBuffer = malloc(BufferLength * sizeof(u32));
	if (RxBuffer == NULL) {
		xil_printf("ERROR: Buffer allocation failed, check heap size in lscript.ld\r\n");
		return XST_FAILURE;
	} else {
		xil_printf("Buffer for %d ns of samples allocated\r\n", (int)BufferPeriod_ns);
	}

	memset(RxBuffer, 0, BufferLength * sizeof(u32));
	Xil_DCacheFlushRange((UINTPTR)RxBuffer, BufferLength * sizeof(u32));

	// Create a Dma Bd Ring and map the buffer to it
	S2mmAttachBuffer(S2mmPtr, (UINTPTR)RxBuffer, BufferLength);

	// Write to the Scope relay pins
	WriteZmodScopeRelayConfig(ScopePtr, Relays);

	// Make sure data will be coming from the scope, not the monitor
	AxiStreamSourceMonitorSetSelect(TrafficGenPtr, SWITCH_SOURCE_SCOPE);

	// Configure the trigger
	TriggerSetPosition (TrigPtr, BufferLength, TriggerPosition);
	TriggerSetEnable (TrigPtr, 0xFFFFFFFF);

	xil_printf("Done initializing PL hardware\r\n");

	// Start up the input pipeline from back to front
	// Start the DMA receive
	S2mmStartCyclicTransfer(S2mmPtr);

	// Start the trigger hardware
	TriggerStart(TrigPtr);

	// Start the Zmod data stream
	ZmodScope_StartStream(ScopePtr);

	xil_printf("Input stream started\r\n");

	// Apply a manual trigger
	sleep(1);
	ManualTriggerIssueTrigger(ManPtr);

	// Wait for trigger hardware to go idle
	while (!TriggerGetIdle(TrigPtr));

	xil_printf("Trigger detected\r\n");

	u32 *BufferHeadPtr = S2mmFindStartOfBuffer(S2mmPtr);
	if (BufferHeadPtr == NULL) {
		xil_printf("ERROR: No buffer head detected\r\n");
	}

	u32 BufferHeadIndex = (((u32)BufferHeadPtr - (u32)RxBuffer) / sizeof(u32)) % BufferLength;

	u32 TriggerDetected = TriggerGetDetected(TrigPtr);

	xil_printf("Info about the acquisition buffer\r\n");
	xil_printf("  Buffer base address: %08x\r\n", RxBuffer);
	xil_printf("  Buffer high address: %08x\r\n", ((u32)RxBuffer) + ((BufferLength-1) * sizeof(u32)));
	xil_printf("  Length of buffer (words): %d\r\n", BufferLength);
	xil_printf("  Index of buffer head: %d\r\n", BufferHeadIndex);
	xil_printf("  Trigger position: %d\r\n", TriggerPosition);
	xil_printf("  Index of trigger position: %d\r\n", (BufferHeadIndex + TriggerPosition) % BufferLength);
	xil_printf("  Detected trigger condition: %08x\r\n", TriggerDetected);

	// Invalidate the cache to ensure acquired data can be read
	Xil_DCacheInvalidateRange((UINTPTR)RxBuffer, BufferLength * sizeof(u32));

	xil_printf("Transfer done\r\n");

	// Find the minimum and maximum samples in the acquisition for each channel and print them
	float ch1Max_mV = 0.0;
	float ch1Min_mV = 0.0;
	float ch2Max_mV = 0.0;
	float ch2Min_mV = 0.0;
	u32 ch1RawMax = 0;
	u32 ch1RawMin = 0;
	u32 ch2RawMax = 0;
	u32 ch2RawMin = 0;

	for (u32 i = 0; i < BufferLength; i++) {
		u32 index = (i + BufferHeadIndex) % BufferLength;
		float ch1_mV = 1000.0f * RawDataToVolts(RxBuffer[index], 0, ZMOD_SCOPE_RESOLUTION, Relays.Ch1Range);
		float ch2_mV = 1000.0f * RawDataToVolts(RxBuffer[index], 1, ZMOD_SCOPE_RESOLUTION, Relays.Ch2Range);
		const u16 ch1_raw = ChannelData(0, RxBuffer[index], ZMOD_SCOPE_RESOLUTION);
		const u16 ch2_raw = ChannelData(1, RxBuffer[index], ZMOD_SCOPE_RESOLUTION);
		if (ch1Max_mV < ch1_mV) { ch1Max_mV = ch1_mV; ch1RawMax = ch1_raw; }
		if (ch1Min_mV > ch1_mV) { ch1Min_mV = ch1_mV; ch1RawMin = ch1_raw; }
		if (ch2Max_mV < ch2_mV) { ch2Max_mV = ch2_mV; ch2RawMax = ch2_raw; }
		if (ch2Min_mV > ch2_mV) { ch2Min_mV = ch2_mV; ch2RawMin = ch2_raw; }
	}

	xil_printf("Relay Settings:\r\n");
	xil_printf("  Ch1Coupling: %s\r\n", Relays.Ch1Coupling ? "DC" : "AC");
	xil_printf("  Ch2Coupling: %s\r\n", Relays.Ch2Coupling ? "DC" : "AC");
	xil_printf("  Ch1Range: %s\r\n", Relays.Ch1Range ? "High" : "Low");
	xil_printf("  Ch2Range: %s\r\n", Relays.Ch2Range ? "High" : "Low");

	xil_printf("Measurements:\r\n");
	xil_printf("  ch1Max_mV: %d mV (0x%04x)\r\n", (int)ch1Max_mV, (int)ch1RawMax);
	xil_printf("  ch1Min_mV: %d mV (0x%04x)\r\n", (int)ch1Min_mV, (int)ch1RawMin);
	xil_printf("  ch2Max_mV: %d mV (0x%04x)\r\n", (int)ch2Max_mV, (int)ch2RawMax);
	xil_printf("  ch2Min_mV: %d mV (0x%04x)\r\n", (int)ch2Min_mV, (int)ch2RawMin);

	// Clean up allocated memory
	S2mmCleanup(S2mmPtr);
	free(RxBuffer);

	xil_printf("Exit\r\n\r\n");
	return XST_SUCCESS;
}

XStatus TestInputWithMonitor (InputPipeline *InstPtr) {
	// Initialize device drivers
	S2mmTransferHierarchy *S2mmPtr = &(InstPtr->S2mm);
	ManualTrigger *ManPtr = &(InstPtr->Man);
	TriggerControl *TrigPtr = &(InstPtr->Trig);
	AxiStreamSourceMonitor *TrafficGenPtr = &(InstPtr->TrafficGen);

	// Define the acquisition window
	//	const u32 SampleRateMegaSamplesPerSecond = 40;
	//	const u32 SamplePeriodNanoseconds = 25;
	// fails at bufferlength=0x100
	const u32 BufferLength = 0x1000;//0x400000; // 0x400000 / 125 MS/s = 3.3 ms
	const u32 TriggerPosition = BufferLength / 4;

	// Initialize the buffer for receiving data from PL
	u32 *RxBuffer = NULL;

	RxBuffer = malloc(BufferLength * sizeof(u32));
	if (RxBuffer == NULL) {
		xil_printf("ERROR: Buffer allocation failed, check heap size in lscript.ld\r\n");
		return XST_FAILURE;
	}

	memset(RxBuffer, 0, BufferLength * sizeof(u32));

	// Create a Dma Bd Ring and map the buffer to it
	S2mmAttachBuffer(S2mmPtr, (UINTPTR)RxBuffer, BufferLength);

	// Flush the cache before any transfer
	Xil_DCacheFlushRange((UINTPTR)RxBuffer, BufferLength * sizeof(u32));

	// Configure the trigger
	TriggerSetPosition (TrigPtr, BufferLength, TriggerPosition);
	TriggerSetEnable (TrigPtr, 0xFFFFFFFF);

	// Route data from the traffic generator
	// Has to occur before trigger and DMA start in order to ensure that the new source of the switch is flushed?
	AxiStreamSourceMonitorSetSelect(TrafficGenPtr, SWITCH_SOURCE_GENERATOR);

	xil_printf("Initialization done\r\n");

	// Start up the input pipeline from back to front
	// Start the DMA receive
	S2mmStartCyclicTransfer(S2mmPtr);

	// Start the trigger hardware
	TriggerStart(TrigPtr);

	// FIXME: Start the data source first to ensure that the pipeline is flushed into an idle trigger module?
	// Enable the traffic generator
	AxiStreamSourceMonitorSetEnable(TrafficGenPtr, 1);

	// Apply a manual trigger
	sleep(1);
	ManualTriggerIssueTrigger(ManPtr);

	// Wait for trigger hardware to go idle
	while (!TriggerGetIdle(TrigPtr));

	u32 *BufferHeadPtr = S2mmFindStartOfBuffer(S2mmPtr);
	if (BufferHeadPtr == NULL) {
		xil_printf("ERROR: No buffer head detected\r\n");
	}

	u32 BufferHeadIndex = (((u32)BufferHeadPtr - (u32)RxBuffer) / sizeof(u32)) % BufferLength;

	u32 TriggerDetected = TriggerGetDetected(TrigPtr);

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
	AxiStreamSourceMonitorSetEnable(TrafficGenPtr, 0);

	// Check the buffer to see if data increments like expected
	u32 errors = 0;
	u32 previous = RxBuffer[BufferHeadIndex];
	for (u32 i = 1; i < BufferLength; i++) {
		u32 index = (i + BufferHeadIndex) % BufferLength;
		if (RxBuffer[index] != previous + 1) {
			xil_printf("RxBuffer at %08x: %d; expected %d\r\n", (u32)RxBuffer + index*sizeof(u32), RxBuffer[index], previous + 1);
			errors++;
		}
		previous = RxBuffer[index];
	}
	if (errors == 0) {
		xil_printf("All RxBuffer data matched!\r\n");
	}

	// Clean up allocated memory
	S2mmCleanup(S2mmPtr);
	free(RxBuffer);

	xil_printf("Exit\r\n\r\n");
	return XST_SUCCESS;
}

XStatus AcquireDataToConsole (InputPipeline *InstPtr, ZmodScopeRelayConfig Relays) {
	// Initialize device drivers
	S2mmTransferHierarchy *S2mmPtr = &(InstPtr->S2mm);
	ManualTrigger *ManPtr = &(InstPtr->Man);
	TriggerControl *TrigPtr = &(InstPtr->Trig);
	AxiStreamSourceMonitor *TrafficGenPtr = &(InstPtr->TrafficGen);
	ZmodScope *ScopePtr = &(InstPtr->Scope);

	// Define the acquisition window
	//	const u32 SampleRateMegaSamplesPerSecond = 40;
	//	const u32 SamplePeriodNanoseconds = 25;
	// fails at bufferlength=0x100
	const u32 BufferLength = 0x1000;//0x400000; // 0x400000 / 125 MS/s = 3.3 ms
	const u32 TriggerPosition = BufferLength / 4;

	// Initialize the buffer for receiving data from PL
	u32 *RxBuffer = NULL;

	RxBuffer = malloc(BufferLength * sizeof(u32));
	if (RxBuffer == NULL) {
		xil_printf("ERROR: Buffer allocation failed, check heap size in lscript.ld\r\n");
		return XST_FAILURE;
	}

	memset(RxBuffer, 0, BufferLength * sizeof(u32));

	// Create a Dma Bd Ring and map the buffer to it
	S2mmAttachBuffer(S2mmPtr, (UINTPTR)RxBuffer, BufferLength);

	// Flush the cache before any transfer
	Xil_DCacheFlushRange((UINTPTR)RxBuffer, BufferLength * sizeof(u32));

	// Configure the trigger
	TriggerSetPosition (TrigPtr, BufferLength, TriggerPosition);
	TriggerSetEnable (TrigPtr, 0xFFFFFFFF);


	AxiStreamSourceMonitorSetSelect(TrafficGenPtr, SWITCH_SOURCE_SCOPE);

	xil_printf("Initialization done\r\n");

	// Start up the input pipeline from back to front
	// Start the DMA receive
	S2mmStartCyclicTransfer(S2mmPtr);

	// Start the trigger hardware
	TriggerStart(TrigPtr);

	// FIXME: Start the data source first to ensure that the pipeline is flushed into an idle trigger module?

	WriteZmodScopeRelayConfig(ScopePtr, Relays);

	// Start the Zmod data stream
	ZmodScope_StartStream(ScopePtr);

	// Apply a manual trigger
	sleep(1);
	ManualTriggerIssueTrigger(ManPtr);

	// Wait for trigger hardware to go idle
	while (!TriggerGetIdle(TrigPtr));

	u32 *BufferHeadPtr = S2mmFindStartOfBuffer(S2mmPtr);
	if (BufferHeadPtr == NULL) {
	xil_printf("ERROR: No buffer head detected\r\n");
	}

	u32 BufferHeadIndex = (((u32)BufferHeadPtr - (u32)RxBuffer) / sizeof(u32)) % BufferLength;

	u32 TriggerDetected = TriggerGetDetected(TrigPtr);

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

	for (u32 i = 0; i < BufferLength; i++) {
		u32 index = (i + BufferHeadIndex) % BufferLength;
		float ch1_mV = 1000.0f * RawDataToVolts(RxBuffer[index], 0, ZMOD_SCOPE_RESOLUTION, Relays.Ch1Range);
		float ch2_mV = 1000.0f * RawDataToVolts(RxBuffer[index], 1, ZMOD_SCOPE_RESOLUTION, Relays.Ch2Range);
		const u16 ch1_raw = RxBuffer[index] & 0xffff;
		const u16 ch2_raw = (RxBuffer[index] >> 16) & 0xffff;
		xil_printf("@%08x\t%04x\t%04x\t%d\t%d\r\n", (u32)RxBuffer + index*sizeof(u32), ch1_raw, ch2_raw, (int)ch1_mV, (int)ch2_mV);
	}

	// Clean up allocated memory
	S2mmCleanup(S2mmPtr);
	free(RxBuffer);

	xil_printf("Exit\r\n\r\n");
	return XST_SUCCESS;
}


XStatus LevelTriggerAcquisition (InputPipeline *InstPtr, ZmodScopeRelayConfig Relays, u32 TrigEnable, u16 Ch1Level, u16 Ch2Level) {
	// Initialize device drivers
	S2mmTransferHierarchy *S2mmPtr = &(InstPtr->S2mm);
	ManualTrigger *ManPtr = &(InstPtr->Man);
	TriggerControl *TrigPtr = &(InstPtr->Trig);
	AxiStreamSourceMonitor *TrafficGenPtr = &(InstPtr->TrafficGen);
	ZmodScope *ScopePtr = &(InstPtr->Scope);
	UserRegisters *LevelTriggerPtr = &(InstPtr->LevelTrigger);

	// Define the acquisition window
	//	const u32 SampleRateMegaSamplesPerSecond = 40;
	//	const u32 SamplePeriodNanoseconds = 25;
	// fails at bufferlength=0x100
	const u32 BufferLength = 0x1000;//0x400000; // 0x400000 / 125 MS/s = 3.3 ms
	const u32 TriggerPosition = 0;//BufferLength / 4;

	// Initialize the buffer for receiving data from PL
	u32 *RxBuffer = NULL;

	RxBuffer = malloc(BufferLength * sizeof(u32));
	if (RxBuffer == NULL) {
		xil_printf("ERROR: Buffer allocation failed, check heap size in lscript.ld\r\n");
		return XST_FAILURE;
	}

	memset(RxBuffer, 0, BufferLength * sizeof(u32));

	// Create a Dma Bd Ring and map the buffer to it
	S2mmAttachBuffer(S2mmPtr, (UINTPTR)RxBuffer, BufferLength);

	// Flush the cache before any transfer
	Xil_DCacheFlushRange((UINTPTR)RxBuffer, BufferLength * sizeof(u32));

	// Configure the trigger
	TriggerSetPosition (TrigPtr, BufferLength, TriggerPosition);
	TriggerSetEnable (TrigPtr, TrigEnable);

	u32 Levels = ((u32)(Ch1Level) << 16) | (Ch2Level);
//	u32 Levels = ((u32)(Ch2Level) << 16) | (Ch1Level);
	UserRegisters_WriteReg(LevelTriggerPtr, USER_REGISTERS_OUTPUT0_REG_OFFSET, Levels);

	AxiStreamSourceMonitorSetSelect(TrafficGenPtr, SWITCH_SOURCE_SCOPE);

	xil_printf("Initialization done\r\n");

	// Start up the input pipeline from back to front
	// Start the DMA receive
	S2mmStartCyclicTransfer(S2mmPtr);

	// Start the trigger hardware
	TriggerStart(TrigPtr);

	// FIXME: Start the data source first to ensure that the pipeline is flushed into an idle trigger module?

	WriteZmodScopeRelayConfig(ScopePtr, Relays);

	// Start the Zmod data stream
	ZmodScope_StartStream(ScopePtr);

	// Apply a manual trigger
	sleep(1);
	ManualTriggerIssueTrigger(ManPtr);

	// Wait for trigger hardware to go idle
	xil_printf("Waiting for trigger...\r\n");
	while (!TriggerGetIdle(TrigPtr));

	// FIXME: maybe wait a bit to ensure that the RXEOF frame transfer has completed
	u32 *BufferHeadPtr = S2mmFindStartOfBuffer(S2mmPtr);
	if (BufferHeadPtr == NULL) {
		xil_printf("ERROR: No buffer head detected\r\n");
	}

	u32 BufferHeadIndex = (((u32)BufferHeadPtr - (u32)RxBuffer) / sizeof(u32)) % BufferLength;

	u32 TriggerDetected = TriggerGetDetected(TrigPtr);

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

	for (u32 i = 0; i < BufferLength; i++) {
		u32 index = (i + BufferHeadIndex) % BufferLength;
		float ch1_mV = 1000.0f * RawDataToVolts(RxBuffer[index], 0, ZMOD_SCOPE_RESOLUTION, Relays.Ch1Range);
		float ch2_mV = 1000.0f * RawDataToVolts(RxBuffer[index], 1, ZMOD_SCOPE_RESOLUTION, Relays.Ch2Range);
		const u16 ch1_raw = ChannelData(0, RxBuffer[index], ZMOD_SCOPE_RESOLUTION);
		const u16 ch2_raw = ChannelData(1, RxBuffer[index], ZMOD_SCOPE_RESOLUTION);
		xil_printf("@%08x\t%04x\t%04x\t%d\t%d\r\n", (u32)RxBuffer + index*sizeof(u32), ch1_raw, ch2_raw, (int)ch1_mV, (int)ch2_mV);
	}

	// Clean up allocated memory
	S2mmCleanup(S2mmPtr);
	free(RxBuffer);

	xil_printf("Exit\r\n\r\n");
	return XST_SUCCESS;
}


int main () {
	// Initialize device drivers
	InputPipeline Pipe;

	// Initialize IP driver devices
	S2mmInitialize(&(Pipe.S2mm), DMA_ID);
	TriggerControl_Initialize(&(Pipe.Trig), TRIGGER_CTRL_BASEADDR);
	ManualTrigger_Initialize(&(Pipe.Man), MANUAL_TRIGGER_BASEADDR);
	AxiStreamSourceMonitor_Initialize(&(Pipe.TrafficGen), SOURCE_MONITOR_BASEADDR);
	ZmodScope_Initialize(&(Pipe.Scope), SCOPE_BASEADDR);
	UserRegisters_Initialize(&(Pipe.LevelTrigger), LEVELTRIGGER_BASEADDR);

	xil_printf("Done initializing device drivers\r\n");

	ZmodScopeRelayConfig CouplingTestRelays = {0, 0, 1, 0};
	ZmodScopeRelayConfig GainTestRelays = {1, 0, 0, 0};
	ZmodScopeRelayConfig LowRangeDcCoupling = {0, 0, 1, 1};

	LevelTriggerAcquisition (&Pipe, LowRangeDcCoupling, 0b00011, 0x0000, 0x01F0);
//	MinMaxAcquisition(&Pipe, CouplingTestRelays);
//	MinMaxAcquisition(&Pipe, GainTestRelays);

}
