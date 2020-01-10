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

void writeDataToFile(std::string filename, uint32_t *firstBuffer, uint32_t *secondBuffer, uint8_t channel, size_t length) {
	std::ofstream file(filename);
	int32_t value;
	float time;

	file << "Time (s),Channel 1 (V),Channel 2 (V)\n";
	for (size_t i = 0; i < length; i++) {
		time = i / 100000000.0;
		file << time;

		value = adcZmod.signedChannelData(channel, firstBuffer[i]);
		file << "," << value;

		value = adcZmod.signedChannelData(channel, secondBuffer[i]);
		file << "," << value;

		file << "\n";
	}
}

void printData(uint32_t* buffer, uint8_t channel, size_t length) {
	int32_t value;

	for (size_t i = 0; i < length; i++) {
		value = adcZmod.signedChannelData(channel, buffer[i]);
		std::cout << value << "\n";
	}
}

void adcDemo(uint8_t channel, size_t length) {
	uint32_t *firstBuffer = adcZmod.allocChannelsBuffer(length);
	uint32_t *secondBuffer = adcZmod.allocChannelsBuffer(length);

	adcZmod.acquireImmediatePolling(firstBuffer, length);
	adcZmod.acquireImmediatePolling(secondBuffer, length);

	writeDataToFile("buffer_data.csv", firstBuffer, secondBuffer, channel, length);

	adcZmod.freeDMABuffer(firstBuffer, length);
	adcZmod.freeDMABuffer(secondBuffer, length);
}

int main() {
	adcDemo(0, TRANSFER_LEN);
	return 0;
}
