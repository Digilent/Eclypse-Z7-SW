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

/*
 * Simple DAC test, using simple ramp values populated in the buffer.
* @param offset - the voltage offset for the generated ramp
* @param amplitude - the amplitude for the generated ramp
* @param step - the step between two generated samples
* @param channel - the channel where samples will be generated
* @param frequencyDivider - the output frequency divider
* @param gain - the gain for the channel
*/
void dacRampDemo(float offset, float amplitude, float step, uint8_t channel, uint8_t frequencyDivider, uint8_t gain)
{
	ZMODDAC1411 dacZmod(DAC_BASE_ADDR, DAC_DMA_BASE_ADDR, IIC_BASE_ADDR, DAC_FLASH_ADDR, DAC_DMA_IRQ);
	uint32_t *buf;
	float val;
	uint32_t valBuf;
	int16_t valRaw;
	size_t length = (int)((amplitude * 2.0) / step) * 2;
	int i;
	if (length > ((1<<14) - 1))
	{
		// limit the length to maximum buffer size (1<<14 - 1)
		length = ((1<<14) - 1);
		// adjust step
		step = amplitude/(length>>2);
	}

	buf = dacZmod.allocChannelsBuffer(length);

	dacZmod.setOutputSampleFrequencyDivider(frequencyDivider);
	dacZmod.setGain(channel, gain);

	i = 0;
	// ramp up
	for(val = -amplitude; val < amplitude; val += step)
	{
		valRaw = dacZmod.getSignedRawFromVolt(val + offset, gain);
		valBuf = dacZmod.arrangeChannelData(channel, valRaw);
		buf[i++] = valBuf;
	}
	// ramp down
	for(val = amplitude; val > -amplitude; val -= step)
	{
		valRaw = dacZmod.getSignedRawFromVolt(val + offset, gain);
		valBuf = dacZmod.arrangeChannelData(channel, valRaw);
		buf[i++] = valBuf;
	}

	// send data to DAC and start the instrument
	dacZmod.setData(buf, length);
	dacZmod.start();
	dacZmod.freeChannelsBuffer(buf, length);
}

int main() {
	std::cout << "ZmodDAC1411 Demo\n";
	// offset 					2 V
	// amplitude 				3 V
	// step 					10 mV
	// channel 					CH1
	// Output Frequency Divider	2
	// gain						HIGH - Corresponds to HIGH input Range
	dacRampDemo(2, 3, 0.01, 0, 2, 1);
	return 0;
}
