/**
 * @file main.cpp
 * @author Cosmin Tanislav
 * @author Cristian Fatu
 * @date 15 Nov 2019
 * @brief File containing the ZMOD ADC1410 baremetal demo.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xil_printf.h>
#include "xparameters.h"

#include "zmodadc1410.h"
#include "zmod.h"

#define TRANSFER_LEN	0x400
#define ZMOD_ADC_BASE_ADDR 	XPAR_ZMODADC_0_AXI_ZMODADC1410_1_S00_AXI_BASEADDR
#define DMA_ADC_BASE_ADDR 	XPAR_ZMODADC_0_AXI_DMA_0_BASEADDR
#define IIC_BASE_ADDR   XPAR_PS7_I2C_1_BASEADDR
#define FLASH_ADDR_ADC   	0x30
#define ZMOD_ADC_IRQ 		XPAR_FABRIC_ZMODADC_0_AXI_ZMODADC1410_1_LIRQOUT_INTR
#define DMA_ADC_IRQ 		XPAR_FABRIC_ZMODADC_0_AXI_DMA_0_S2MM_INTROUT_INTR

void formatADCDataOverUART(ZMODADC1410 *padcZmod, uint32_t *acqBuffer, uint8_t gain, size_t length)
{
	char val_formatted[15];
	char time_formatted[15];
	uint32_t valBuf;
	int16_t valCh1;
	float valV;

	xil_printf("New acquisition ------------------------\r\n");
	xil_printf("Ch1\tRaw\tTime\t\r\n");
	for (size_t i = 0; i < length; i++) {
		valBuf = acqBuffer[i];
		valCh1 = padcZmod->signedChannelData(0, valBuf);
		valV = padcZmod->getVoltFromSignedRaw(valCh1, gain);

		padcZmod->formatValue(val_formatted, 1000.0*valV, "mV");
		if (i < 100) {
			padcZmod->formatValue(time_formatted, i*10, "ns");
		} else {
			padcZmod->formatValue(time_formatted, (float)(i)/100.0, "us");
		}

		xil_printf("%s\t%X\t%s\r\n", val_formatted, (uint32_t)(valCh1&0x3FFF), time_formatted);
	}
}

void adcDemo(size_t length) {
	ZMODADC1410 adcZmod(ZMOD_ADC_BASE_ADDR, DMA_ADC_BASE_ADDR, IIC_BASE_ADDR, FLASH_ADDR_ADC,
			ZMOD_ADC_IRQ, DMA_ADC_IRQ);
	uint8_t gain = 1; // HIGH
	uint32_t *acqBuffer = adcZmod.allocChannelsBuffer(length);
	adcZmod.setGain(0, gain);
	while(1) {
		adcZmod.acquireImmediatePolling(acqBuffer, length);

		formatADCDataOverUART(&adcZmod, acqBuffer, gain, length);
		sleep(2);
	}
	adcZmod.freeDMABuffer(acqBuffer, length);

}

int main() {
	adcDemo(TRANSFER_LEN);
    return 0;
}
