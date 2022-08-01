#ifndef ZMOD_AWG_AXI_CONFIGURATION_LOCALSRC_H_   /* prevent circular inclusions */
#define ZMOD_AWG_AXI_CONFIGURATION_LOCALSRC_H_

#include "ZmodAwgAxiConfiguration.h"

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
} ZmodAwg_CalibrationCoefficients;

void ZmodAwgAxiConfiguration_SetControl(ZmodAwgAxiConfiguration *InstPtr, u32 Control);
u32 ZmodAwgAxiConfiguration_GetControl(ZmodAwgAxiConfiguration *InstPtr);

u32 ZmodAwgAxiConfiguration_GetStatus(ZmodAwgAxiConfiguration *InstPtr);
#define ZmodAwgAxiConfiguration_InitDone(InstPtr) (ZMOD_AWG_AXI_CONFIGURATION_STATUS_INITDONEDAC_MASK & ZmodAwgAxiConfiguration_GetStatus(InstPtr))


void ZmodAwgAxiConfiguration_SetCalibration(ZmodAwgAxiConfiguration *InstPtr, ZmodAwg_CalibrationCoefficients Coeff);

#define AXI_ZMOD_AWG_MULT_COEFF_FIXED_POINT_ONES_BIT 16
static const ZmodAwg_CalibrationCoefficients ZmodAwg_DefaultCoeff = {
	1 << AXI_ZMOD_AWG_MULT_COEFF_FIXED_POINT_ONES_BIT,
	1 << AXI_ZMOD_AWG_MULT_COEFF_FIXED_POINT_ONES_BIT,
	0,
	0,
	1 << AXI_ZMOD_AWG_MULT_COEFF_FIXED_POINT_ONES_BIT,
	1 << AXI_ZMOD_AWG_MULT_COEFF_FIXED_POINT_ONES_BIT,
	0,
	0
};


#endif
