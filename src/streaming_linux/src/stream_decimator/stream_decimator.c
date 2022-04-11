#include "stream_decimator.h"

/***************************************************************************
* This function sets the decimation factor of the AXI4-Stream Decimator IP.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
* @param	decimation_factor 	- decimation factor which will be used by the IP
* @return	 0 on success,
* 			-1 if the provided UIO is NULL,
* 			-2 if the decimation factor is greater than 32767
*
******************************************************************************/
int fnStreamDecimatorSetDecimationFactor(UIO* uio, uint32_t decimation_factor) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	if(decimation_factor > 32767) {
		perror("Decimation Factor must be less than 32768");
		return -2;
	}
	ACCESS_REG(uio->mapPtr, AXIS_DECIMATOR_REG_ADDR_CONFIG) = (decimation_factor << 16) + (0x0000FFFF & ACCESS_REG(uio->mapPtr, AXIS_DECIMATOR_REG_ADDR_CONFIG));
	return 0;
}

/***************************************************************************
* This function gets the decimation factor of the AXI4-Stream Decimator IP.
*
* @param	uio	- UIO struct to which the registers are mapped
* @return	the decimation factor on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int fnStreamDecimatorGetDecimationFactor(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	return (ACCESS_REG(uio->mapPtr, AXIS_DECIMATOR_REG_ADDR_CONFIG) >> 16) & 0x0000FFFF;
}

/***************************************************************************
* This function sets the packet length of the AXI4-Stream Decimator IP.
*
* @param	uio	 		   	- UIO struct to which the registers are mapped
* @param	packet_length	- the number of samples sent after which a
* 								TLAST signal is generated
* @return	 0 on success,
* 			-1 if the provided UIO is NULL,
* 			-2 if the packet length is greater than 65535
*
******************************************************************************/
int fnStreamDecimatorSetPacketLength(UIO* uio, uint32_t packet_length) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	if(packet_length > 65535) {
		perror("Packet length must be less than 65536");
		return -2;
	}
	ACCESS_REG(uio->mapPtr, AXIS_DECIMATOR_REG_ADDR_CONFIG) = (packet_length << 1) + (0xFFFF0001 & ACCESS_REG(uio->mapPtr, AXIS_DECIMATOR_REG_ADDR_CONFIG));
	return 0;
}

/***************************************************************************
* This function gets the packet length of the AXI4-Stream Decimator IP.
*
* @param	uio	 		   	- UIO struct to which the registers are mapped
* @return	 the packet length on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int fnStreamDecimatorGetPacketLength(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	return ACCESS_REG(uio->mapPtr, AXIS_DECIMATOR_REG_ADDR_CONFIG) >> 1 & 0x00007FFF;
}

/***************************************************************************
* This function stops the IP and resets the internal counters used to determine
* the sample to send after decimation and
* the number of samples until sending a TLAST
* of the AXI4-Stream Decimator IP.
* It will then re-enable the IP and de-assert the reset.
*
* @param	uio	 		   	- UIO struct to which the registers are mapped
* @return	 0 on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int fnStreamDecimatorReset(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	// Stop HLS IP
	ACCESS_REG(uio->mapPtr, AXIS_DECIMATOR_REG_ADDR_HLS_CONTROL) = 0;
	// Stop and reset internal counters
	ACCESS_REG(uio->mapPtr, AXIS_DECIMATOR_REG_ADDR_CONFIG) = ACCESS_REG(uio->mapPtr, 0x10) | 1;
	// Start HLS IP
	ACCESS_REG(uio->mapPtr, AXIS_DECIMATOR_REG_ADDR_HLS_CONTROL) = 0x81;
	// Enable internal counters
	ACCESS_REG(uio->mapPtr, AXIS_DECIMATOR_REG_ADDR_CONFIG) = ACCESS_REG(uio->mapPtr, 0x10) & 0xFFFFFFFE;
	return 0;
}

/***************************************************************************
* This function sets up the AXI4-Stream Decimator IP.
* It will first map it's registers to an UIO struct.
* It will then set the decimation factor and packet length.
* Finally it will reset and start the IP.
*
* @param	uio	 		   	 - UIO struct to which the registers will be mapped
* @param	decimationFactor - decimation factor which will be used by the IP
* @param	packetLength	 - the number of samples after which TLAST is generated
* @return	0 if the IP was set up succesfully,
* 			-1 if setting the decimation factor failed,
* 			-2 if setting the packet length failed,
* 			-3 if resetting the IP failed.
*
******************************************************************************/
int fnStreamDecimatorSetup(UIO* uio, uint32_t decimationFactor, uint32_t packetLength) {
	int rc;
	// set decimation factor
	rc = fnStreamDecimatorSetDecimationFactor(uio, decimationFactor);
	if(rc < 0) {
		return -1;
	}
	// set packet length
	rc = fnStreamDecimatorSetPacketLength(uio, packetLength);
	if(rc < 0) {
		return -2;
	}
	// reset Stream Decimator and set auto-restart
	rc = fnStreamDecimatorReset(uio);
	if(rc < 0) {
		return -3;
	}
	return 0;
}
