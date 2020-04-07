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

/*
 * Simple ADC test, puts the ADC in the test mode (ramp),
 * performs an acquisition under specific trigger conditions
 * and verifies the acquired data to be consistent with these conditions.
 */
void testZMODADC1410Ramp_Auto()
{
	ZMODADC1410 adcZmod(ZMOD_ADC_BASE_ADDR, DMA_ADC_BASE_ADDR, IIC_BASE_ADDR, FLASH_ADDR_ADC,
			ZMOD_ADC_IRQ, DMA_ADC_IRQ);
	if(adcZmod.autoTestRamp(1, 0, 0, 4, TRANSFER_LEN) == ERR_SUCCESS)
	{
		xil_printf("Success autotest ADC ramp\r\n");
	}
	else
	{
		xil_printf("Error autotest ADC ramp\r\n");
	}
}

/*
 * Format data contained in the buffer and sends it over UART.
 * It displays the acquired value (in mV), raw value (as 14 bits hexadecimal value)
 * and time stamp within the buffer (in time units).
 * @param padcZmod - pointer to the ZMODADC1410 object
 * @param acqBuffer - the buffer containing acquired data
 * @param channel - the channel where samples were acquired
 * @param gain - the gain for the channel
 * @param length	- the buffer length to be used
 */
void formatADCDataOverUART(ZMODADC1410 *padcZmod, uint32_t *acqBuffer, uint8_t channel, uint8_t gain, size_t length)
{
	char val_formatted[15];
	char time_formatted[15];
	uint32_t valBuf;
	int16_t valCh;
	float val;
	xil_printf("New acquisition ------------------------\r\n");
	xil_printf("Ch1\tRaw\tTime\t\r\n");
	for (size_t i = 0; i < length; i++)
	{
		valBuf = acqBuffer[i];
		valCh = padcZmod->signedChannelData(channel, valBuf);
		val = padcZmod->getVoltFromSignedRaw(valCh, gain);
		padcZmod->formatValue(val_formatted, 1000.0*val, "mV");
		if (i < 100)
		{
			padcZmod->formatValue(time_formatted, i*10, "ns");
		}
		else
		{
			padcZmod->formatValue(time_formatted, (float)(i)/100.0, "us");
		}


		xil_printf("%s\t%X\t%s\r\n", val_formatted, (uint32_t)(valCh&0x3FFF), time_formatted);
	}
}


/*
 * Simple ADC test, acquires data and sends it over UART.
 * @param channel - the channel where samples will be acquired
 * @param gain - the gain for the channel
 * @param length	- the buffer length to be used
 */
void adcDemo(uint8_t channel, uint8_t gain, size_t length) {
	ZMODADC1410 adcZmod(ZMOD_ADC_BASE_ADDR, DMA_ADC_BASE_ADDR, IIC_BASE_ADDR, FLASH_ADDR_ADC,
			ZMOD_ADC_IRQ, DMA_ADC_IRQ);
	uint32_t *acqBuffer;
	adcZmod.setGain(channel, gain);
	while(1)
	{
		acqBuffer = adcZmod.allocChannelsBuffer(length);
		adcZmod.acquireImmediatePolling(acqBuffer, length);

		formatADCDataOverUART(&adcZmod, acqBuffer, channel, gain, length);
		adcZmod.freeChannelsBuffer(acqBuffer, length);
		sleep(2);
	}

}

/*
 * ADC Baremetal Demo
 */
int main() {
	// channel 					CH1
	// gain						LOW - Corresponds to HIGH input Range
	// length					0x400
	adcDemo(0, 0, TRANSFER_LEN);

	//testZMODADC1410Ramp_Auto();
	return 0;
}
