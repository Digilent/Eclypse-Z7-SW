/**
 * @file stream_decimator/stream_decimator.h
 * @author Nita Eduard
 * @date 3 March 2022
 * @brief Function declarations used for the AXI4-Stream Decimator IP
 */
#ifndef AXIS_DECIMATOR_H
#define AXIS_DECIMATOR_H

#define AXIS_DECIMATOR_REG_ADDR_HLS_CONTROL 0x0
#define AXIS_DECIMATOR_REG_ADDR_CONFIG 0x10

#include <libuio.h>
int fnStreamDecimatorSetDecimationFactor(UIO* uio, uint32_t decimation_factor);
int fnStreamDecimatorGetDecimationFactor(UIO* uio);
int fnStreamDecimatorSetPacketLength(UIO* uio, uint32_t packet_length);
int fnStreamDecimatorGetPacketLength(UIO* uio);
int fnStreamDecimatorReset(UIO* uio);
int fnStreamDecimatorSetup(UIO* uio, uint32_t decimationFactor, uint32_t packetLength);

#endif
