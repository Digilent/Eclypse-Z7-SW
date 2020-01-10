/**
 * @file main.cpp
 * @author Cosmin Tanislav
 * @author Cristian Fatu
 * @date 15 Nov 2019
 * @brief File containing the ZMOD DAC1411 linux demo.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <iostream>

#include "zmod.h"
#include "zmoddac1411.h"

#define TRANSFER_LEN	0x400
#define IIC_BASE_ADDR   0xE0005000
#define ZMOD_IRQ 		61

#define DAC_BASE_ADDR 		0x43C10000
#define DAC_DMA_BASE_ADDR 	0x40410000
#define DAC_FLASH_ADDR   	0x31
#define DAC_DMA_IRQ 		63

ZMODDAC1411 dacZmod(DAC_BASE_ADDR, DAC_DMA_BASE_ADDR, IIC_BASE_ADDR, DAC_FLASH_ADDR,
		ZMOD_IRQ, DAC_DMA_IRQ);

void dacDemo(uint8_t channel, size_t length) {
	uint32_t *buffer = dacZmod.allocChannelsBuffer(length);
	uint32_t value;

	for (size_t i = 0; i < length; i++) {
		value = dacZmod.arrangeSignedChannelData(0, i * 100) | dacZmod.arrangeSignedChannelData(1, i * 100);
		buffer[i] = value;
	}

	dacZmod.setData(buffer, length);
	dacZmod.start();

	dacZmod.freeDMABuffer(buffer, length);
}

int main() {
	dacDemo(0, TRANSFER_LEN);
	return 0;
}
