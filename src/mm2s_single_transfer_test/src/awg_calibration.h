#ifndef AWG_CALIBRATION_H_   /* prevent circular inclusions */
#define AWG_CALIBRATION_H_

#include "zmod_awg_axi_configuration.h"
#include "xstatus.h"

#define ZMODAWG_ZMOD_PORT_A_VIO_GROUP 0
#define ZMODAWG_ZMOD_PORT_B_VIO_GROUP 1

XStatus ZmodAwg_ReadCoefficientsFromDna(u32 ZmodPortVioGroup, ZmodAwg_CalibrationCoefficients *FactoryCoefficients, ZmodAwg_CalibrationCoefficients *UserCoefficients);

#endif /* end of protection macro */
