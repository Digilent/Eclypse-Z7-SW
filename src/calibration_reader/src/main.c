#include "./dpmutil/dpmutil.h"

typedef enum ZmodFamily {
	ZmodADC = 0,
	ZmodDAC = 1
} ZmodFamily;

typedef struct ZmodInfo {
	u8 resolution;
	float maxSampleRateMHz;
	ZmodFamily family; // 0 for adc, 1 for dac
} ZmodInfo;

ZmodInfo LookupZmodInfo (SzgDnaStrings DnaStrings) {
	ZmodInfo info;

	if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1410-40") == 0) {
		info.family = ZmodADC;
		info.maxSampleRateMHz = 40.0f;
		info.resolution = 14;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1410-105") == 0) {
		info.family = ZmodADC;
		info.maxSampleRateMHz = 105.0f;
		info.resolution = 14;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1410-125") == 0) {
		info.family = ZmodADC;
		info.maxSampleRateMHz = 125.0f;
		info.resolution = 14;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1210-40") == 0) {
		info.family = ZmodADC;
		info.maxSampleRateMHz = 40.0f;
		info.resolution = 12;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1210-125") == 0) {
		info.family = ZmodADC;
		info.maxSampleRateMHz = 125.0f;
		info.resolution = 12;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1010-40") == 0) {
		info.family = ZmodADC;
		info.maxSampleRateMHz = 40.0f;
		info.resolution = 10;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1010-125") == 0) {
		info.family = ZmodADC;
		info.maxSampleRateMHz = 125.0f;
		info.resolution = 10;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod DAC 1411-125") == 0) {
		info.family = ZmodDAC;
		info.maxSampleRateMHz = 125.0f;
		info.resolution = 14;
	}
	return info;
}

int main () {
	int fdI2cDev = 0; // this isn't using linux so this doesn't matter

	dpmutilPortInfo_t PortInfo[8] = {0};

	// enumerate the Syzygy ports to figure out which have Zmods installed
	dpmutilFEnum(FALSE, FALSE, PortInfo);

// FIXME: how do enum indices line up with ports? is it consistent?

	SzgDnaHeader DnaHeader;
	SzgDnaStrings DnaStrings = {0};

	for (u32 iPort = 0; iPort < 8; iPort++) {
		if (PortInfo[iPort].portType == 0) {
			continue;
		}

		SyzygyReadDNAHeader(fdI2cDev, PortInfo[iPort].i2cAddr, &DnaHeader, FALSE);
		SyzygyReadDNAStrings(fdI2cDev, PortInfo[iPort].i2cAddr, &DnaHeader, &DnaStrings);

		switch (LookupZmodInfo(DnaStrings).family) {
		case ZmodADC:
			printf("========= %s Calibration Coefficients =========\r\n", DnaStrings.szProductName);
			FDisplayZmodADCCal(fdI2cDev, PortInfo[iPort].i2cAddr);
			ZMOD_ADC_CAL ADCFactoryCalibration, ADCUserCalibration;
			FGetZmodADCCal(fdI2cDev, PortInfo[iPort].i2cAddr, &ADCFactoryCalibration, &ADCUserCalibration);
			printf("\r\n");
			break;
		case ZmodDAC:
			printf("========= %s Calibration Coefficients =========\r\n", DnaStrings.szProductName);
			FDisplayZmodDACCal(fdI2cDev, PortInfo[iPort].i2cAddr);
			ZMOD_DAC_CAL DACFactoryCalibration, DACUserCalibration;
			FGetZmodDACCal(fdI2cDev, PortInfo[iPort].i2cAddr, &DACFactoryCalibration, &DACUserCalibration);
			printf("\r\n");
			break;
		}

		SyzygyFreeDNAStrings(&DnaStrings);
	}

	return 0;
}
