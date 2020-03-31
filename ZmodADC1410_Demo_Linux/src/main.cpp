/**
 * @file main.cpp
 * @author Cosmin Tanislav
 * @author Cristian Fatu
 * @date 15 Nov 2019
 * @brief File containing the ZMOD ADC1410 linux demo.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <iostream>

#include "zmodadc1410.h"
#include "zmod.h"

#define TRANSFER_LEN	0x400
#define IIC_BASE_ADDR   0xE0005000
#define ZMOD_IRQ 		61

#define ADC_BASE_ADDR		0x43C00000
#define ADC_DMA_BASE_ADDR 	0x40400000
#define ADC_FLASH_ADDR   	0x30
#define ADC_DMA_IRQ 		62

ZMODADC1410 adcZmod(ADC_BASE_ADDR, ADC_DMA_BASE_ADDR, IIC_BASE_ADDR, ADC_FLASH_ADDR,
		ZMOD_IRQ, ADC_DMA_IRQ);

/*
 * Format data contained in the buffer and writes it to a file and to the standard output.
 * The file is overwritten any time the function is called.
 * @param filename - the file name to contain the acquisition formatted data
 * @param acqBuffer - the buffer containing acquired data
 * @param channel - the channel where samples were acquired
 * @param gain - the gain for the channel
 * @param length	- the buffer length to be used
 */
void writeADCData(std::string filename, uint32_t *acqBuffer, uint8_t channel, uint8_t gain, size_t length) {
	char val_formatted[15];
	char time_formatted[15];
	std::ofstream file(filename);
	int16_t valCh;
	float val;
	file << "Channel, Time\n";

	for (size_t i = 0; i < length; i++) {
		if (i < 100)
		{
			adcZmod.formatValue(time_formatted, i*10, "ns");
		}
		else
		{
			adcZmod.formatValue(time_formatted, (float)(i)/100.0, "us");
		}

		valCh = adcZmod.signedChannelData(channel, acqBuffer[i]);
		val = adcZmod.getVoltFromSignedRaw(valCh, gain);
		adcZmod.formatValue(val_formatted, 1000.0*val, "mV");

		file << val_formatted << ", " << time_formatted << "\n";
		std::cout << val_formatted << ", " << time_formatted << "\n";
	}
	file.close();
}

/*
 * Performs (repeatedly) un-triggered acquisitions and writes the data to a file and to the standard output.
 * The file only contains the content of one acquisition, as it is overwritten.
 * @param channel - the channel where samples were acquired
 * @param gain - the gain for the channel
 * @param length	- the buffer length to be used
 */
void adcDemo(uint8_t channel, uint8_t gain, size_t length) {
	uint32_t *acqBuffer;
	adcZmod.setGain(channel, gain);
	while(1)
	{
		acqBuffer = adcZmod.allocChannelsBuffer(length);
		adcZmod.acquireImmediatePolling(acqBuffer, length);
		writeADCData("/home/eclypse/buffer_data.csv", acqBuffer, channel, gain, length);
		adcZmod.freeChannelsBuffer(acqBuffer, length);
		sleep(2);
	}
}


/*
 * ADC Linux Demo
 */
int main() {
	std::cout << "ZmodADC1410 Demo\n";
	// channel 					CH1
	// gain						LOW - Corresponds to HIGH input Range
	// length					0x400
	adcDemo(0, 0, TRANSFER_LEN);
	return 0;
}
