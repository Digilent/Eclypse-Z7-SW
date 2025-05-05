#ifndef ZMOD_SCOPE_AXI_CONFIGURATION_H_   /* prevent circular inclusions */
#define ZMOD_SCOPE_AXI_CONFIGURATION_H_

#include "xil_types.h"
#include "xil_io.h"

#define AXI_ZMOD_SCOPE_CONTROL_REG_OFFSET 						    0x00
#define AXI_ZMOD_SCOPE_GIE_REG_OFFSET 		    					0x04
#define AXI_ZMOD_SCOPE_IP_INTR_EN_REG_OFFSET 			    		0x08
#define AXI_ZMOD_SCOPE_IP_INTR_STS_REG_OFFSET 			    		0x0C
#define AXI_ZMOD_SCOPE_CHANNEL_1_HIGH_GAIN_MULT_COEFF_REG_OFFSET    0x10
#define AXI_ZMOD_SCOPE_CHANNEL_1_LOW_GAIN_MULT_COEFF_REG_OFFSET     0x18
#define AXI_ZMOD_SCOPE_CHANNEL_1_HIGH_GAIN_ADD_COEFF_REG_OFFSET     0x20
#define AXI_ZMOD_SCOPE_CHANNEL_1_LOW_GAIN_ADD_COEFF_REG_OFFSET 	    0x28
#define AXI_ZMOD_SCOPE_CHANNEL_2_HIGH_GAIN_MULT_COEFF_REG_OFFSET    0x30
#define AXI_ZMOD_SCOPE_CHANNEL_2_LOW_GAIN_MULT_COEFF_REG_OFFSET     0x38
#define AXI_ZMOD_SCOPE_CHANNEL_2_HIGH_GAIN_ADD_COEFF_REG_OFFSET     0x40
#define AXI_ZMOD_SCOPE_CHANNEL_2_LOW_GAIN_ADD_COEFF_REG_OFFSET 	    0x48
#define AXI_ZMOD_SCOPE_CONFIG_REG_OFFSET 							0x50
#define AXI_ZMOD_SCOPE_STATUS_REG_OFFSET 							0x58

/* Control register bitfields */
#define AXI_ZMOD_SCOPE_CONTROL_AP_START_MASK 		0x01
#define AXI_ZMOD_SCOPE_CONTROL_AP_DONE_MASK  		0x02
#define AXI_ZMOD_SCOPE_CONTROL_AP_IDLE_MASK  		0x04
#define AXI_ZMOD_SCOPE_CONTROL_AP_READY_MASK 		0x08
#define AXI_ZMOD_SCOPE_CONTROL_AUTO_RESTART_MASK	0x80

/* Global interrupt enable register bitfields */
#define AXI_ZMOD_SCOPE_GIE_ENABLE_MASK 		0x01

/* IP interrupt enable register bitfields */
#define AXI_ZMOD_SCOPE_IP_INTR_EN_CHANNEL_0_MASK 	0x1
#define AXI_ZMOD_SCOPE_IP_INTR_EN_CHANNEL_1_MASK 	0x2
#define AXI_ZMOD_SCOPE_IP_INTR_EN_ALL_MASK 0x3

/* IP interrupt status register bitfields */
#define AXI_ZMOD_SCOPE_IP_INTR_STS_CHANNEL_0_MASK 	0x1
#define AXI_ZMOD_SCOPE_IP_INTR_STS_CHANNEL_1_MASK 	0x2

/* Channel 1 high gain multiplicative coefficient register bitfields */
#define AXI_ZMOD_SCOPE_CHANNEL_1_HIGH_GAIN_MULT_COEFF_MASK 0x3FFFF

/* Channel 1 low gain multiplicative coefficient register bitfields */
#define AXI_ZMOD_SCOPE_CHANNEL_1_LOW_GAIN_MULT_COEFF_MASK 0x3FFFF

/* Channel 1 high gain additive coefficient register bitfields */
#define AXI_ZMOD_SCOPE_CHANNEL_1_HIGH_GAIN_ADD_COEFF_MASK 0x3FFFF

/* Channel 1 low gain additive coefficient register bitfields */
#define AXI_ZMOD_SCOPE_CHANNEL_1_LOW_GAIN_ADD_COEFF_MASK 0x3FFFF

/* Channel 2 high gain multiplicative coefficient register bitfields */
#define AXI_ZMOD_SCOPE_CHANNEL_2_HIGH_GAIN_MULT_COEFF_MASK 0x3FFFF

/* Channel 2 low gain multiplicative coefficient register bitfields */
#define AXI_ZMOD_SCOPE_CHANNEL_2_LOW_GAIN_MULT_COEFF_MASK 0x3FFFF

/* Channel 2 high gain additive coefficient register bitfields */
#define AXI_ZMOD_SCOPE_CHANNEL_2_HIGH_GAIN_ADD_COEFF_MASK 0x3FFFF

/* Channel 2 low gain additive coefficient register bitfields */
#define AXI_ZMOD_SCOPE_CHANNEL_2_LOW_GAIN_ADD_COEFF_MASK 0x3FFFF

/* Common coeff constants */
#define AXI_ZMOD_SCOPE_MULT_COEFF_FIXED_POINT_ONES_BIT 16

/* Config register bitfields */
#define AXI_ZMOD_SCOPE_CONFIG_CHANNEL_1_GAIN_SELECT_MASK 		0x01
#define AXI_ZMOD_SCOPE_CONFIG_CHANNEL_2_GAIN_SELECT_MASK 		0x02
#define AXI_ZMOD_SCOPE_CONFIG_CHANNEL_1_COUPLING_SELECT_MASK 	0x04
#define AXI_ZMOD_SCOPE_CONFIG_CHANNEL_2_COUPLING_SELECT_MASK 	0x08
#define AXI_ZMOD_SCOPE_CONFIG_TEST_MODE_MASK 					0x10
#define AXI_ZMOD_SCOPE_CONFIG_ENABLE_ACQUISITION_MASK 			0x20
#define AXI_ZMOD_SCOPE_CONFIG_RESETN_MASK 						0x40
#define AXI_ZMOD_SCOPE_CONFIG_ALL_SOFTWARE_BITS					0x1f

/* Status register bitfields */
#define AXI_ZMOD_SCOPE_STATUS_RESET_BUSY_MASK 			0x01
#define AXI_ZMOD_SCOPE_STATUS_INIT_DONE_ADC_MASK 		0x02
#define AXI_ZMOD_SCOPE_STATUS_CONFIG_ERROR_MASK 		0x04
#define AXI_ZMOD_SCOPE_STATUS_INIT_DONE_RELAY_MASK 		0x08
#define AXI_ZMOD_SCOPE_STATUS_DATA_OVERFLOW_MASK 		0x10

/* Device handlers */
typedef struct {
	u32 Channel1_HighGainMultiplicative;
	u32 Channel1_LowGainMultiplicative;
	u32 Channel1_HighGainAdditive;
	u32 Channel1_LowGainAdditive;
	u32 Channel2_HighGainMultiplicative;
	u32 Channel2_LowGainMultiplicative;
	u32 Channel2_HighGainAdditive;
	u32 Channel2_LowGainAdditive;
} ZmodScope_CalibrationCoefficients;

typedef struct {
	UINTPTR BaseAddr;
	u8 Range; // 1 high; 0 low
	u8 Coupling; // 1 DC; 0 AC
} ZmodScope;

#define ZmodScope_In32	Xil_In32
#define ZmodScope_Out32	Xil_Out32

#define ZmodScope_ReadReg(BaseAddress, RegOffset)  ZmodScope_In32((BaseAddress) + (RegOffset))
#define ZmodScope_WriteReg(BaseAddress, RegOffset, Data)  ZmodScope_Out32((BaseAddress) + (RegOffset), (Data))

void ZmodScope_Initialize(ZmodScope *InstPtr, u32 BaseAddr);

// provide software access to only these:
//AXI_ZMOD_SCOPE_CONFIG_CHANNEL_1_GAIN_SELECT_MASK
//AXI_ZMOD_SCOPE_CONFIG_CHANNEL_2_GAIN_SELECT_MASK
//AXI_ZMOD_SCOPE_CONFIG_CHANNEL_1_COUPLING_SELECT_MASK
//AXI_ZMOD_SCOPE_CONFIG_CHANNEL_2_COUPLING_SELECT_MASK
//AXI_ZMOD_SCOPE_CONFIG_TEST_MODE_MASK
void ZmodScope_SetConfig(ZmodScope *InstPtr, u32 Config);
u32 ZmodScope_GetConfig(ZmodScope *InstPtr);

void ZmodScope_SetCalibrationCoefficients(ZmodScope *InstPtr, ZmodScope_CalibrationCoefficients CalibrationCoefficients);

void ZmodScope_StartStream(ZmodScope *InstPtr);

float ZmodScope_ConvertData(ZmodScope *InstPtr, u32 RawSample, u8 Channel);

#endif /* end of protection macro */
