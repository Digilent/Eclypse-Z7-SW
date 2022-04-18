//-------------------------------------------------------------------------------
//--
//-- File: main.c
//-- Author: Nita Eduard
//-- Date: 16 March 2022
//--
//-------------------------------------------------------------------------------
//-- MIT License
//
//-- Copyright (c) 2022 Digilent
//
//-- Permission is hereby granted, free of charge, to any person obtaining a copy
//-- of this software and associated documentation files (the "Software"), to deal
//-- in the Software without restriction, including without limitation the rights
//-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//-- copies of the Software, and to permit persons to whom the Software is
//-- furnished to do so, subject to the following conditions:
//
//-- The above copyright notice and this permission notice shall be included in all
//-- copies or substantial portions of the Software.
//
//-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//-- SOFTWARE.
//--
//-------------------------------------------------------------------------------
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>
#include <libuio.h>
#include "eth/eth.h"
#include "stream_decimator/stream_decimator.h"
#include "zmod_scope_config/zmod_scope_config.h"
#include "dma/dma.h"

/************************** Constant Definitions ****************************/
#define DMA_BASE_ADDR 0x41E00000

/************************** User Definitions ********************************/
/* Number of block descriptors used by the DMA. Maximum of 255. */
#define BD_NUM       			255

/************************** Function Prototypes *****************************/
int ethLogData(int client, uint32_t* transferBuffer, uint64_t transferSize,
		uint32_t bd_size, uint32_t bd_num, uint32_t data_size);

/************************** Global Variables ********************************/
// Global counter used to keep track of how many block descriptors need to be sent
// This is incremented whenever the DMA finished writing to a block descriptor
int64_t bd_to_be_sent;

int main(int argc, char *argv[])
{
	int rc, server, client;
	uintptr_t dmaAddr;
	uint32_t* transferBuffer;
	UIO* uioStreamDecimator;
	UIO* uioZmodScopeConfig;
	GAIN gainCh1, gainCh2;
	COUPLING couplingCh1, couplingCh2;
	uint64_t transferSize;
	uint32_t decimationFactor;
	uint32_t packetLength;
	uint8_t configuration[14];

	// map Stream Decimator Registers
	uioStreamDecimator = UIO_MAP(0, 0);
	// map Zmod Scope Registers
	uioZmodScopeConfig = UIO_MAP(1, 0);
	// start Zmod Scope Config HLS IP
	fnZmodScopeConfigEnable(uioZmodScopeConfig);
	fnZmodScopeConfigDisableReset(uioZmodScopeConfig);
	fnZmodScopeConfigEnableAcquisition(uioZmodScopeConfig);
	while(fnZmodScopeConfigIsRstBusy(uioZmodScopeConfig));
	// configure Zmod Scope with default coefficients
	fnZmodScopeConfigCh1SetExtCoefficients(uioZmodScopeConfig, 0x11631, 0x10A14, 0x3FE10, 0x3FE2D);
	fnZmodScopeConfigCh2SetExtCoefficients(uioZmodScopeConfig, 0x1178C, 0x10B14, 0x001F4, 0x00010);
	//fnZmodScopeConfigCh2SetExtCoefficients(uioZmodScopeConfig, 0x10000, 0x10000, 0, 0);

	// Setup the ethernet socket
	server = fnEthInitConnection(8082);
	if(server < 0) {
		printf("Failed initializing the server socket!\n");
		return -1;
	}

	while(1)
	{
		printf("Waiting for a connection...\n");

		// Wait for connection
		client = fnEthWaitForConnection(server);
		if(client < 0) {
			printf("Failed while waiting for a connection!\n");
			goto CLOSE_SERVER;
		}
		printf("Connection established!\n");
		printf("Receiving configuration...\n");

		rc = fnEthRecvData(client, configuration, 14);
		if(rc < 0) {
			printf("Failed receiving data from client!\n");
			goto CLOSE_CLIENT;
		}
		printf("Received configuration!\n");

		// extract configuration from payload buffer
		transferSize = (configuration[0] << 24) + (configuration[1] << 16) + (configuration[2] << 8) + configuration[3];
		gainCh1 = configuration[4] & 1;
		gainCh2 = (configuration[4] & 2) >> 1;
		couplingCh1 = configuration[5] & 1;
		couplingCh2 = (configuration[5] & 2) >> 1;
		decimationFactor = (configuration[6] << 24) + (configuration[7] << 16) + (configuration[8] << 8) + configuration[9];
		packetLength = (configuration[10] << 24) + (configuration[11] << 16) + (configuration[12] << 8) + configuration[13];

		fnZmodScopeConfigCh1SetGain(uioZmodScopeConfig, gainCh1);
		fnZmodScopeConfigCh2SetGain(uioZmodScopeConfig, gainCh2);
		fnZmodScopeConfigCh1SetCoupling(uioZmodScopeConfig, couplingCh1);
		fnZmodScopeConfigCh2SetCoupling(uioZmodScopeConfig, couplingCh2);

		// setup Stream Decimator IP
		rc = fnStreamDecimatorSetup(uioStreamDecimator, decimationFactor, packetLength);
		if(rc < 0) {
			printf("Failed setting up Stream Decimator\n");
			goto CLOSE_CLIENT;
		}

		// Initiate DMA
		dmaAddr = fnInitDMA(DMA_BASE_ADDR, DMA_DIRECTION_RX, 61);

		// Allocate the transfer buffer
		transferBuffer = fnAllocBuffer(dmaAddr, BD_NUM * packetLength * sizeof(uint32_t));
		if(!transferBuffer) {
			printf("Buffer failed to allocate!\n");
			goto DESTROY_DMA;
		}

		printf("Starting transfer...\n");
		// Start the DMA transfer
		rc = fnOneWayDMATransfer(dmaAddr, transferBuffer, BD_NUM, BD_NUM * packetLength * sizeof(uint32_t));
		if(rc != 0) {
			printf("Failed to start DMA transfer!\n");
			goto FREE_BUFFER;
		}

		rc = ethLogData(client, transferBuffer, transferSize, packetLength, BD_NUM, sizeof(uint32_t));
		printf("Transfer done!\n");

		FREE_BUFFER:
		fnFreeBuffer(dmaAddr, transferBuffer, BD_NUM * packetLength * sizeof(uint32_t));
		DESTROY_DMA:
		fnDestroyDMA(dmaAddr);
		CLOSE_CLIENT:
		close(client);
	}
	CLOSE_SERVER:
	close(server);

	return 0;
}

/**
 *	This function sends data from a cyclical DMA buffer to a client socket.
 *
 *	@param client 			- TCP client socket to send data to
 *	@param transferBuffer 	- memory-mapped cyclical DMA buffer
 *	@param transferSize	- number of elements to be transfered. This will be rounded up
 *								to the closest value that is a multiple of bd_size
 *	@param bd_size			- size of a block descriptor. Should not be greater
 *								than the DMA packet length
 *	@param bd_num			- number of block descriptors which make up the
*								cyclical DMA buffer
 *	@param data_size		- size in bytes of the data type which is transfered
 */
int ethLogData(int client, uint32_t* transferBuffer, uint64_t transferSize,
		uint32_t bd_size, uint32_t bd_num, uint32_t data_size) {
	int rc;
	// Index of the current block descriptor inside the cyclical DMA buffer
	uint32_t bd_current = 1;
	// Number of block descriptors sent so far
	uint32_t bd_sent = 0;
	// Number of block descriptors that need to transfered
	uint32_t bd_to_transfer = (transferSize + bd_size - 1) / bd_size;
	// Number of block descriptors to send
	uint32_t bd_to_send;
	// Size in bytes of the block descriptors to send
	uint32_t bytes_to_transfer;
	// Number of block descriptors written by the DMA but not sent
	// Due to the behavior of the DMA when starting,
	// do not take into account the first buffer
	bd_to_be_sent = -1;
	// Start transferring data
	while((bd_to_transfer && bd_sent < bd_to_transfer)){
		// Wait for DMA to write to a block descriptor. It will interrupt the sleep
		sleep(1);
		// Select how many block descriptors need to be sent
		// If the cyclic DMA index looped back to the beginning,
		// send the block descriptors up to the end first
		if(bd_to_be_sent < bd_num - bd_current) {
			bd_to_send = bd_to_be_sent;
		}else {
			bd_to_send = bd_num-bd_current;
		}
		// Ensure that no more than bd_to_transfer block descriptors are sent
		if(bd_to_send > bd_to_transfer - bd_sent) {
			bd_to_send = bd_to_transfer - bd_sent;
		}
		// Setup the number of bytes to be transfered
	    bytes_to_transfer = data_size * bd_size * bd_to_send;
	    // Send data
	    rc = fnEthSendData(client,  (uint8_t*)&transferBuffer[bd_current * bd_size], bytes_to_transfer);
		if(rc < 0) {
			return rc;
		}
	    // Decrement number of block descriptors to be sent.
		bd_to_be_sent -= bd_to_send;
		// Update current block descriptor index
		bd_current = (bd_current + bd_to_send) % bd_num;
		// Update how many buffers were sent so far
		bd_sent += bd_to_send;
	}
	return 0;
}
