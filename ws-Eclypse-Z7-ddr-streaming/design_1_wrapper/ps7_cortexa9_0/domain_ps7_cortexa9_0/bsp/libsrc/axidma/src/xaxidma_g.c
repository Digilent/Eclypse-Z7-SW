#include "xaxidma.h"

XAxiDma_Config XAxiDma_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,axi-dma-7.1", /* compatible */
		0x40010000, /* reg */
		0x0, /* xlnx,sg-include-stscntrl-strm */
		0x1, /* xlnx,include-mm2s */
		0x0, /* xlnx,include-mm2s-dre */
		0x40, /* xlnx,mm2s-data-width */
		0x0, /* xlnx,include-s2mm */
		0x0, /* xlnx,include-s2mm-dre */
		0x20, /* xlnx,s2mm-data-width */
		0x1, /* xlnx,include-sg */
		0x1, /* xlnx,num-mm2s-channels */
		0x1, /* xlnx,num-s2mm-channels */
		0x100, /* xlnx,mm2s-burst-size */
		0x10, /* xlnx,s2mm-burst-size */
		0x0, /* xlnx,micro-dma */
		0x20, /* xlnx,addrwidth */
		0x1a, /* xlnx,sg-length-width */
		{0xffff,  0xffff}, /* interrupts */
		0xffff /* interrupt-parent */
	},
	{
		"xlnx,axi-dma-7.1", /* compatible */
		0x40050000, /* reg */
		0x0, /* xlnx,sg-include-stscntrl-strm */
		0x0, /* xlnx,include-mm2s */
		0x0, /* xlnx,include-mm2s-dre */
		0x20, /* xlnx,mm2s-data-width */
		0x1, /* xlnx,include-s2mm */
		0x0, /* xlnx,include-s2mm-dre */
		0x40, /* xlnx,s2mm-data-width */
		0x1, /* xlnx,include-sg */
		0x1, /* xlnx,num-mm2s-channels */
		0x1, /* xlnx,num-s2mm-channels */
		0x10, /* xlnx,mm2s-burst-size */
		0x100, /* xlnx,s2mm-burst-size */
		0x0, /* xlnx,micro-dma */
		0x20, /* xlnx,addrwidth */
		0x1a, /* xlnx,sg-length-width */
		{0xffff,  0xffff}, /* interrupts */
		0xffff /* interrupt-parent */
	},
	 {
		 NULL
	}
};