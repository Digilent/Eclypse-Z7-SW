#include "zmod_scope_config.h"

/***************************************************************************
* This function starts the Zmod Scope Config HLS IP and enables auto-restart.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 0 on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigEnable(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	ACCESS_REG(uio->mapPtr, 0x4) = 0x1;
	ACCESS_REG(uio->mapPtr, 0x8) = 0x1;

	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_HLS_CONTROL) = 0x81;
	return 0;
}

/***************************************************************************
* This function stops the Zmod Scope Config HLS IP and disables auto-restart.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 0 on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigDisable(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_HLS_CONTROL) = 0;
	return 0;
}
/***************************************************************************
* This function sets the external coefficients of the Zmod Scope's channel 1.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
* @param	HgMultCoef		 	- high gain multiplicative coefficient
* @param	LgMultCoef		 	- low  gain multiplicative coefficient
* @param	HgAddCoef		 	- high gain additive coefficient
* @param	LgAddCoef		 	- low  gain additive coefficient
*
* @return	 0 on success,
* 			-1 if the provided UIO is NULL,
*
******************************************************************************/
int8_t fnZmodScopeConfigCh1SetExtCoefficients(UIO* uio, uint32_t HgMultCoef, uint32_t LgMultCoef,
		uint32_t HgAddCoef, uint32_t LgAddCoef) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH1_HG_MULT_COEF) = HgMultCoef;
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH1_LG_MULT_COEF) = LgMultCoef;
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH1_HG_ADD_COEF) = HgAddCoef;
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH1_LG_ADD_COEF) = LgAddCoef;
	return 0;
}

/***************************************************************************
* This function gets the external coefficients of the Zmod Scope's channel 1.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
* @param	HgMultCoef		 	- high gain multiplicative coefficient
* @param	LgMultCoef		 	- low  gain multiplicative coefficient
* @param	HgAddCoef		 	- high gain additive coefficient
* @param	LgAddCoef		 	- low  gain additive coefficient
*
* @return	 0 on success,
* 			-1 if the provided UIO is NULL,
* 			-2 if one of the coefficients' address is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigCh1GetExtCoefficients(UIO* uio, uint32_t* HgMultCoef, uint32_t* LgMultCoef,
		uint32_t* HgAddCoef, uint32_t* LgAddCoef) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	if(HgMultCoef == NULL || LgMultCoef == NULL || HgAddCoef == NULL || LgAddCoef == NULL) {
		perror("Channel 1 Coefficient address is NULL");
		return -2;
	}
	*HgMultCoef = ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH1_HG_MULT_COEF);
	*LgMultCoef = ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH1_LG_MULT_COEF);
	*HgAddCoef  = ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH1_HG_ADD_COEF);
	*LgAddCoef  = ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH1_LG_ADD_COEF);
	return 0;
}

/***************************************************************************
* This function sets the gain of the Zmod Scope's channel 1.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
* @param	gain			 	- HIGH/LOW gain
*
* @return	 0 on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigCh1SetGain(UIO* uio, GAIN gain) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) = (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) & 0b111110) + gain;
	return 0;
}

/***************************************************************************
* This function gets the gain of the Zmod Scope's channel 1.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 Channel 1 Gain on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigCh1GetGain(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	return (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) & 0b000001);
}

/***************************************************************************
* This function sets the coupling of the Zmod Scope's channel 1.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
* @param	coupling			- AC/DC coupling
*
* @return	 0 on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigCh1SetCoupling(UIO* uio, COUPLING coupling) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) = (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) & 0b111011) + (coupling << 2);
	return 0;
}

/***************************************************************************
* This function gets the coupling of the Zmod Scope's channel 1.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 Channel 1 Coupling on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigCh1GetCoupling(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	return (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) & 0b000100) >> 2;
}

/***************************************************************************
* This function sets the external coefficients of the Zmod Scope's channel 2.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
* @param	HgMultCoef		 	- high gain multiplicative coefficient
* @param	LgMultCoef		 	- low  gain multiplicative coefficient
* @param	HgAddCoef		 	- high gain additive coefficient
* @param	LgAddCoef		 	- low  gain additive coefficient
*
* @return	 0 on success,
* 			-1 if the provided UIO is NULL,
*
******************************************************************************/
int8_t fnZmodScopeConfigCh2SetExtCoefficients(UIO* uio, uint32_t HgMultCoef, uint32_t LgMultCoef,
		uint32_t HgAddCoef, uint32_t LgAddCoef) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH2_HG_MULT_COEF) = HgMultCoef;
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH2_LG_MULT_COEF) = LgMultCoef;
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH2_HG_ADD_COEF) = HgAddCoef;
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH2_LG_ADD_COEF) = LgAddCoef;
	return 0;
}

/***************************************************************************
* This function gets the external coefficients of the Zmod Scope's channel 2.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
* @param	HgMultCoef		 	- high gain multiplicative coefficient
* @param	LgMultCoef		 	- low  gain multiplicative coefficient
* @param	HgAddCoef		 	- high gain additive coefficient
* @param	LgAddCoef		 	- low  gain additive coefficient
*
* @return	 0 on success,
* 			-1 if the provided UIO is NULL,
* 			-2 if one of the coefficients' address is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigCh2GetExtCoefficients(UIO* uio, uint32_t* HgMultCoef, uint32_t* LgMultCoef,
		uint32_t* HgAddCoef, uint32_t* LgAddCoef) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	if(HgMultCoef == NULL || LgMultCoef == NULL || HgAddCoef == NULL || LgAddCoef == NULL) {
		perror("Channel 2 Coefficient address is NULL");
		return -2;
	}
	*HgMultCoef = ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH2_HG_MULT_COEF);
	*LgMultCoef = ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH2_LG_MULT_COEF);
	*HgAddCoef  = ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH2_HG_ADD_COEF);
	*LgAddCoef  = ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CH2_LG_ADD_COEF);
	return 0;
}

/***************************************************************************
* This function sets the gain of the Zmod Scope's channel 2.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
* @param	gain			 	- HIGH/LOW gain
*
* @return	 0 on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigCh2SetGain(UIO* uio, GAIN gain) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) = (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) & 0b111101) + (gain << 1);
	return 0;
}

/***************************************************************************
* This function gets the gain of the Zmod Scope's channel 2.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 Channel 2 Gain on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigCh2GetGain(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	return (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) & 0b000010) >> 1;
}

/***************************************************************************
* This function sets the coupling of the Zmod Scope's channel 2.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
* @param	coupling			- AC/DC coupling
*
* @return	 0 on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigCh2SetCoupling(UIO* uio, COUPLING coupling) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) = (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) & 0b110111) + (coupling << 3);
	return 0;
}

/***************************************************************************
* This function gets the coupling of the Zmod Scope's channel 2.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 Channel 2 Coupling on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigCh2GetCoupling(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	return (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) & 0b001000) >> 3;
}
/***************************************************************************
* This function enables the test mode on the Zmod Scope.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 0 on success,
* 			-1 if the provided UIO is NULL,
*
******************************************************************************/
int8_t fnZmodScopeConfigEnableTestMode(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}

	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) = (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) & 0b101111) + (1 << 4);
	return 0;
}

/***************************************************************************
* This function disables the test mode value on the Zmod Scope.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 0 on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigGetTestMode(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) = (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) & 0b101111) + (0 << 4);
	return 0;
}

/***************************************************************************
* This function enables acquisition on the Zmod Scope.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 0 on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigEnableAcquisition(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}

	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) = (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) & 0b011111) + (1 << 5);
	return 0;
}

/***************************************************************************
* This function disables acquisition on the Zmod Scope.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 0 on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigDisableAcquisition(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) = (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_CONFIG) & 0b011111) + (0 << 5);
	return 0;
}

/***************************************************************************
* This function gets the coupling of the Zmod Scope's RstBusy Flag.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 flag value on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigGetRstBusy(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	return (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_STATUS_FLAGS) & 0b00001);
}

/***************************************************************************
* This function gets the coupling of the Zmod Scope's InitDoneADC Flag.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 flag value on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigGetInitDoneADC(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	return (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_STATUS_FLAGS) & 0b00010) >> 1;
}

/***************************************************************************
* This function gets the coupling of the Zmod Scope's ConfigError Flag.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 flag value on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigGetConfigError(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	return (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_STATUS_FLAGS) & 0b00100) >> 2;
}

/***************************************************************************
* This function gets the coupling of the Zmod Scope's InitDoneRelay Flag.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 flag value on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigGetInitDoneRelay(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	return (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_STATUS_FLAGS) & 0b01000) >> 3;
}

/***************************************************************************
* This function gets the coupling of the Zmod Scope's DataOverflow Flag.
*
* @param	uio	 		   	 	- UIO struct to which the registers are mapped
*
* @return	 flag value on success,
* 			-1 if the provided UIO is NULL
*
******************************************************************************/
int8_t fnZmodScopeConfigGetDataOverflow(UIO* uio) {
	if(uio == NULL) {
		perror("UIO is NULL");
		return -1;
	}
	return (ACCESS_REG(uio->mapPtr, ZMOD_SCOPE_REG_ADDR_STATUS_FLAGS) & 0b10000) >> 4;
}
