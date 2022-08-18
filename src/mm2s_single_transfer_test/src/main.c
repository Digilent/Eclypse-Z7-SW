#include "xparameters.h"
#include "xil_printf.h"
#include "xil_cache.h"
#include "sleep.h"
#include "test_stream_sink.h"
#include "mm2s_transfer.h"
#include "xuartps.h"
#include "zmod_awg_axi_configuration.h"
#include "awg_calibration.h"

#define UART_ID XPAR_PS7_UART_0_DEVICE_ID

#define DMA_ID XPAR_ZMODAWG_PORTB_MM2SDMATRANSFER_0_AXI_DMA_0_DEVICE_ID
#define DMA_BURST_SIZE XPAR_ZMODAWG_PORTB_MM2SDMATRANSFER_0_AXI_DMA_0_MM2S_BURST_SIZE
#define DMA_DATA_WIDTH XPAR_ZMODAWG_PORTB_MM2SDMATRANSFER_0_AXI_DMA_0_M_AXI_MM2S_DATA_WIDTH

#define SINK_BASEADDR XPAR_ZMODAWG_PORTB_AXISTREAMSINKMONITOR_0_AXISTREAMSINKMONITOR_0_S_AXI_CONTROL_BASEADDR

#define SWITCH_SINK_VOID 1
#define SWITCH_SINK_ZMODAWG 0

#define AWG_BASEADDR XPAR_ZMODAWG_PORTB_ZMODAWGFRONTEND_0_ZMODAWGAXICONFIGURAT_0_S_AXI_CONTROL_BASEADDR

#define AWG_PORT ZMODAWG_ZMOD_PORT_B_VIO_GROUP

void UartInitialize (XUartPs *InstPtr, u32 DeviceId) {
	XUartPs_Config *CfgPtr;
	CfgPtr = XUartPs_LookupConfig(DeviceId);
	XUartPs_CfgInitialize(InstPtr, CfgPtr, CfgPtr->BaseAddress);
}

u32 UartGetChar (XUartPs *InstPtr, u8 *CharPtr) {
	return XUartPs_Recv(InstPtr, CharPtr, 1);
}

#define TO_ZMOD

u16 VoltageToRaw(float Voltage, u8 Scale) {
#define ZMOD_DAC_IDEAL_LOW_RANGE_VOLTAGE 1.33
#define ZMOD_DAC_IDEAL_HIGH_RANGE_VOLTAGE 5.32
#define RESOLUTION 14
	const float ScaleVoltage = (Scale) ? ZMOD_DAC_IDEAL_HIGH_RANGE_VOLTAGE : ZMOD_DAC_IDEAL_LOW_RANGE_VOLTAGE;
	u16 RawData = (u16)((Voltage / ScaleVoltage) * (1 << (RESOLUTION - 1)));
	return RawData;
}

int main () {
	XUartPs Uart;
	UartInitialize(&Uart, UART_ID);

	Mm2sTransferHierarchy Mm2s;
	Mm2sInitialize(&Mm2s, DMA_ID);

	AxiStreamSinkMonitor Sink;
	AxiStreamSinkMonitor_Initialize(&Sink, SINK_BASEADDR);

	ZmodAwgAxiConfiguration Awg;
//	ZmodAwg_CalibrationCoefficients Coeff = ZmodAwg_DefaultCoeff;


#if defined(TO_ZMOD)
	ZmodAwg_CalibrationCoefficients factory = {0}, user = {0};
	if (ZmodAwg_ReadCoefficientsFromDna(AWG_PORT, &factory, &user) != XST_SUCCESS) {
		xil_printf("Failed to read calibration coefficients from Zmod AWG DNA\r\n");
	}

	ZmodAwgAxiConfiguration_Initialize(&Awg, AWG_BASEADDR);
	ZmodAwgAxiConfiguration_SetCalibration(&Awg, factory);

	const u32 Ch1Scale = 0; // 1 = High, 0 = Low
	const u32 Ch2Scale = 1; //
	const u32 TestMode = 0;
	u32 Control = 0;
	Control |= ZMOD_AWG_AXI_CONFIGURATION_CONTROL_DACENABLE_MASK; // enable the dac when we set control
	Control |= Ch1Scale * ZMOD_AWG_AXI_CONFIGURATION_CONTROL_EXTCH1SCALE_MASK;
	Control |= Ch2Scale * ZMOD_AWG_AXI_CONFIGURATION_CONTROL_EXTCH2SCALE_MASK;
	Control |= TestMode * ZMOD_AWG_AXI_CONFIGURATION_CONTROL_TESTMODE_MASK;
	ZmodAwgAxiConfiguration_SetControl(&Awg, Control);

	u32 Status = ZmodAwgAxiConfiguration_GetStatus(&Awg);
	xil_printf("ZmodAwg status: %08x\r\n", Status);

	AxiStreamSinkMonitor_SetSelect(&Sink, SWITCH_SINK_ZMODAWG);
#elif defined(TO_VOID)
	AxiStreamSinkMonitor_SetSelect(&Sink, SWITCH_SINK_VOID);
#endif

	const u32 Alignment = 0x2; // buffer must be aligned to 64-bit boundaries since the DMA as instantiated in hardware does not support dynamic realignment
	const u32 BufferLength = 0x4000000; // 67,108,864 MS @ 100 MS/s = 0.671 ms buffer length
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

#if defined(TO_ZMOD)
	float voltage;
	u16 Ch1RawData;
	u16 Ch2RawData;

	u32 count = 0;
	for (u32 i = 0; i < BufferLength; i++) {
		voltage = 1.0f * ((float)i / (float)BufferLength);
		Ch1RawData = VoltageToRaw(voltage, Ch1Scale);
		Ch2RawData = VoltageToRaw(voltage, Ch2Scale);
		TxBuffer[i] = (Ch1RawData << 2) | (Ch2RawData << 18);
		if (i % 1000 == 0) {
			count ++;
		}
	}
	xil_printf("%d ", count);
	xil_printf("Transmit buffer filled\r\n");
#elif defined(TO_VOID)
	// fill test pattern
	for (u32 i = 0; i < BufferLength; i++) {
		TxBuffer[i] = i;
	}
#endif

	Xil_DCacheFlushRange((UINTPTR)TxBuffer, BufferLengthBytes);

	Mm2sCreateRing(&Mm2s, BufferLengthBytes);

//#define SINGLE_SHOT
//#define REPEATED
#define CONTINUOUS

#if defined(SINGLE_SHOT)

	AxiStreamSinkMonitor_SetLastCount(&Sink, Mm2s.NumBds);
	AxiStreamSinkMonitor_Start(&Sink);

	u32 CyclicMode = 0;
	Mm2sStartTransfer(&Mm2s, (UINTPTR)TxBuffer, BufferLengthBytes, CyclicMode);

	// Wait until all of the bytes have been transferred
	while (!Mm2sTransferDone(&Mm2s));
	while (!AxiStreamSinkMonitor_GetIdle(&Sink)); // make sure the sink has received a tlast beat

	u32 Beats = AxiStreamSinkMonitor_GetBeatCount(&Sink);
	u32 Misses = AxiStreamSinkMonitor_GetMissCount(&Sink);
	u32 Errors = AxiStreamSinkMonitor_GetErrorCount(&Sink);

	xil_printf("%08x beats counted, %08x expected\r\n", Beats, BufferLengthBytes / sizeof(u32));
	xil_printf("%08x misses counted, 0 expected\r\n", Misses);
	xil_printf("%08x errors counted, 0 expected\r\n", Errors);

#elif defined(REPEATED)
	u32 Repeats = 10;

	AxiStreamSinkMonitor_SetLastCount(&Sink, Repeats * Mm2s.NumBds);
	AxiStreamSinkMonitor_Start(&Sink);
	u32 CyclicMode = 0;

	Mm2sStartTransfer(&Mm2s, (UINTPTR)TxBuffer, BufferLengthBytes, CyclicMode);
	u32 BlocksResubmitted = 0;

	do {
		BlocksResubmitted += Mm2sResubmit(&Mm2s);
	} while (BlocksResubmitted < (Repeats - 1) * Mm2s.NumBds);

	// Wait until all of the bytes have been transferred
	while (!Mm2sTransferDone(&Mm2s));
	while (!AxiStreamSinkMonitor_GetIdle(&Sink)); // make sure the sink has received a tlast beat

	u32 Beats = AxiStreamSinkMonitor_GetBeatCount(&Sink);
	u32 Misses = AxiStreamSinkMonitor_GetMissCount(&Sink);
	u32 Errors = AxiStreamSinkMonitor_GetErrorCount(&Sink);

	xil_printf("%08x beats counted, %08x expected\r\n", Beats, Repeats * BufferLengthBytes / sizeof(u32));
	xil_printf("%08x misses counted, 0 expected\r\n", Misses);
	// expect one discontinuity when the buffer starts being replayed
	xil_printf("%08x errors counted, %d expected\r\n", Errors, BlocksResubmitted / Mm2s.NumBds);

#elif defined(CONTINUOUS)
	u8 Char = 0;
	u8 Done = 0;
	u32 BlocksResubmitted = 0;

//#if defined(TO_ZMOD)
//
//#elif defined(TO_VOID)
	AxiStreamSinkMonitor_SetLastCount(&Sink, 0);
	AxiStreamSinkMonitor_Start(&Sink);
//#endif

	xil_printf("Starting AWG transfer, press any key to halt it\r\n");

	u32 CyclicMode = 0;
	Mm2sStartTransfer(&Mm2s, (UINTPTR)TxBuffer, BufferLengthBytes, CyclicMode);

	do {
		BlocksResubmitted += Mm2sResubmit(&Mm2s);
		Done = XUartPs_Recv(&Uart, &Char, 1);
	} while (!Done);

	// Wait until all of the bytes have been transferred
	while (!Mm2sTransferDone(&Mm2s));
	// FIXME: implement reset for stream sink tlast counter
//	while (!AxiStreamSinkMonitor_GetIdle(&Sink)); // make sure the sink has received a tlast beat

	usleep(500);
	u32 Beats = AxiStreamSinkMonitor_GetBeatCount(&Sink);
	u32 Misses = AxiStreamSinkMonitor_GetMissCount(&Sink);
	u32 Errors = AxiStreamSinkMonitor_GetErrorCount(&Sink);

	// FIXME: add a way to tell if counters have rolled over, a single bit that sets high on counter carry out is sufficient
	// FIXME: miss counter does not yet support continuous mode. I need to implement a way to re/set the tlast counter after the Sink has been started
	//		  this means that the beat count will read lower than expected since metrics are read out early

	xil_printf("%08x beats counted, %08x expected\r\n", Beats, (BlocksResubmitted + 1) * BufferLengthBytes / sizeof(u32));
	xil_printf("%08x misses counted, 0 expected\r\n", Misses);
	// expect one discontinuity when the buffer starts being replayed
	xil_printf("%08x errors counted, %d expected\r\n", Errors, BlocksResubmitted / Mm2s.NumBds);

#endif

	Mm2sDestroyRing(&Mm2s);
	free(TxBufferBase);

#if defined(TO_ZMOD)
	ZmodAwgAxiConfiguration_SetControl(&Awg, Control); // disable the awg
#endif
}
