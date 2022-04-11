/**
 * @file zmod_scope_config/zmod_scope_config.h
 * @author Nita Eduard
 * @date 16 March 2022
 * @brief Function declarations used in accessing calibration coefficients,
 * 		  gain, coupling and status flags of the Zmod Scope.
 */
#ifndef ZMOD_SCOPE_CONFIG_H
#define ZMOD_SCOPE_CONFIG_H
#include <libuio.h>

#define ZMOD_SCOPE_REG_ADDR_HLS_CONTROL 0x0
#define ZMOD_SCOPE_REG_ADDR_CH1_HG_MULT_COEF 0x10
#define ZMOD_SCOPE_REG_ADDR_CH1_LG_MULT_COEF 0x18
#define ZMOD_SCOPE_REG_ADDR_CH1_HG_ADD_COEF 0x20
#define ZMOD_SCOPE_REG_ADDR_CH1_LG_ADD_COEF 0x28

#define ZMOD_SCOPE_REG_ADDR_CH2_HG_MULT_COEF 0x30
#define ZMOD_SCOPE_REG_ADDR_CH2_LG_MULT_COEF 0x38
#define ZMOD_SCOPE_REG_ADDR_CH2_HG_ADD_COEF 0x40
#define ZMOD_SCOPE_REG_ADDR_CH2_LG_ADD_COEF 0x48

#define ZMOD_SCOPE_REG_ADDR_CONFIG 0x50
#define ZMOD_SCOPE_REG_ADDR_STATUS_FLAGS 0x58

typedef enum {
	LOW,
	HIGH
} GAIN;

typedef enum {
	AC,
	DC
} COUPLING;

// Control
int8_t fnZmodScopeConfigEnable(UIO* uio);
int8_t fnZmodScopeConfigDisable(UIO* uio);
int8_t fnZmodScopeConfigEnableAcquisition(UIO* uio);
int8_t fnZmodScopeConfigDisableAcquisition(UIO* uio);
int8_t fnZmodScopeConfigSetTestMode(UIO* uio, uint8_t testMode);
int8_t fnZmodScopeConfigGetTestMode(UIO* uio);

// Channel 1
int8_t fnZmodScopeConfigCh1SetExtCoefficients(UIO* uio, uint32_t HgMultCoef, uint32_t LgMultCoef,
		uint32_t HgAddCoef, uint32_t LgAddCoef);
int8_t fnZmodScopeConfigCh1GetExtCoefficients(UIO* uio, uint32_t* HgMultCoef, uint32_t* LgMultCoef,
		uint32_t* HgAddCoef, uint32_t* LgAddCoef);
int8_t fnZmodScopeConfigCh1SetGain(UIO* uio, GAIN gain);
int8_t fnZmodScopeConfigCh1GetGain(UIO* uio);
int8_t fnZmodScopeConfigCh1SetCoupling(UIO* uio, COUPLING coupling);
int8_t fnZmodScopeConfigCh1GetCoupling(UIO* uio);

// Channel 2
int8_t fnZmodScopeConfigCh2SetExtCoefficients(UIO* uio, uint32_t HgMultCoef, uint32_t LgMultCoef,
		uint32_t HgAddCoef, uint32_t Ch1LgAddCoef);
int8_t fnZmodScopeConfigCh2GetExtCoefficients(UIO* uio, uint32_t* HgMultCoef, uint32_t* LgMultCoef,
		uint32_t* HgAddCoef, uint32_t* LgAddCoef);
int8_t fnZmodScopeConfigCh2SetGain(UIO* uio, GAIN gain);
int8_t fnZmodScopeConfigCh2GetGain(UIO* uio);
int8_t fnZmodScopeConfigCh2SetCoupling(UIO* uio, COUPLING coupling);
int8_t fnZmodScopeConfigCh2GetCoupling(UIO* uio);

// Status flags
int8_t fnZmodScopeGetRstBusy(UIO* uio);
int8_t fnZmodScopeGetConfigError(UIO* uio);
int8_t fnZmodScopeGetInitDoneADC(UIO* uio);
int8_t fnZmodScopeGetInitDoneRelay(UIO* uio);
int8_t fnZmodScopeGetDataOverflow(UIO* uio);

#endif
