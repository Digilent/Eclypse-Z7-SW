#ifndef SCOPE_CALIBRATION_H_   /* prevent circular inclusions */
#define SCOPE_CALIBRATION_H_

#include "zmod_scope_axi_configuration.h"
#include "xstatus.h"

#define ZMODSCOPE_ZMOD_PORT_A_VIO_GROUP 0
#define ZMODSCOPE_ZMOD_PORT_B_VIO_GROUP 1

typedef struct ZmodScopeInfo {
	u8 resolution;
	float maxSampleRateMHz;
} ZmodScopeInfo;

XStatus ZmodScope_ReadCoefficientsFromDna(u32 ZmodPortVioGroup, ZmodScope_CalibrationCoefficients *FactoryCoefficients, ZmodScope_CalibrationCoefficients *UserCoefficients);

#endif /* end of protection macro */
