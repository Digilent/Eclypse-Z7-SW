#ifndef MANUAL_TRIGGER_HW_H_
#define MANUAL_TRIGGER_HW_H_

#include "xil_types.h"
#include "xil_io.h"

/* Register offsets */
#define MANUAL_TRIGGER_AP_CTRL_REG_OFFSET 0x0
#define MANUAL_TRIGGER_GIE_REG_OFFSET 0x4
#define MANUAL_TRIGGER_IP_INTR_EN_REG_OFFSET 0x8
#define MANUAL_TRIGGER_IP_INTR_STS_REG_OFFSET 0xc
#define MANUAL_TRIGGER_TRIGGER_REG_OFFSET 0x10

/* Control Register bitfields */
#define MANUAL_TRIGGER_AP_CTRL_START_MASK 0x01
#define MANUAL_TRIGGER_AP_CTRL_DONE_MASK 0x02
#define MANUAL_TRIGGER_AP_CTRL_IDLE_MASK 0x04
#define MANUAL_TRIGGER_AP_CTRL_READY_MASK 0x08
#define MANUAL_TRIGGER_AP_CTRL_AUTO_RESTART_MASK 0x80

/* Global interrupt enable register bitfields */
#define MANUAL_TRIGGER_GIE_ENABLE_MASK 0x01

/* IP interrupt enable register bitfields */
#define MANUAL_TRIGGER_IP_INTR_EN_AP_DONE_MASK 0x1
#define MANUAL_TRIGGER_IP_INTR_EN_AP_READY_MASK 0x2
#define MANUAL_TRIGGER_IP_INTR_EN_ALL_MASK 0x3

/* IP interrupt status register bitfields */
#define MANUAL_TRIGGER_IP_INTR_STS_AP_DONE_MASK 0x1
#define MANUAL_TRIGGER_IP_INTR_STS_AP_READY_MASK 0x2
/* TRIGGER register bitfields */
#define MANUAL_TRIGGER_TRIGGER_TRIGGER_MASK 0x1

#endif /* end of protection macro */
