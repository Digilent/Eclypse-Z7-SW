/**
 * @file dma.h
 * @author Cosmin Tanislav
 * @author Cristian Fatu
 * @date 15 Nov 2019
 * @brief Function declarations used for DMA transfer.
 */

#ifndef DMA_H_
#define DMA_H_

#include <stdint.h>
#include "libaxidma.h"

/**
 * Direction of a DMA transfer.
 */
enum dma_direction {
	DMA_DIRECTION_TX, ///< TX transfer
	DMA_DIRECTION_RX, ///< RX transfer
};

/**
 * Struct containing data specific to this DMA instance.
 */
typedef struct _dma_env {
	uintptr_t addr; ///< the physical address of the DMA device
	enum dma_direction direction; ///< the direction of the DMA transfer
	axidma_dev_t dma_inst; ///< a pointer to the instance of the AXI DMA library
	int channel_id; ///< the channel id that will be used for DMA transfers
	uint8_t complete_flag; ///< whether the current DMA transfer is complete or not
} DMAEnv;

extern int64_t bd_to_be_sent;
void fnDMASetInterruptHandler(DMAEnv* dma_env, axidma_cb_t callback, void *data);
uint32_t fnInitDMA(uintptr_t addr, enum dma_direction direction, int dmaInterrupt);
void fnDestroyDMA(uintptr_t addr);
int fnOneWayDMATransfer(uintptr_t addr, uint32_t *buf, int buf_num, size_t length);
uint8_t fnIsDMATransferComplete(uintptr_t addr);
void* fnAllocBuffer(uintptr_t addr, size_t size);
void fnFreeBuffer(uintptr_t addr, void *buf, size_t size);

#endif /* DMA_H_ */
