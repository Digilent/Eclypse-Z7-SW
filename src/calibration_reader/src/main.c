#include "./dpmutil/dpmutil.h"
#include "stdio.h"

#define ZMODFAMILY_ZMODSCOPE 0
#define ZMODFAMILY_ZMODAWG 1
#define ZMODFAMILY_ZMODDIGITIZER 2
#define ZMODFAMILY_UNSUPPORTED -1

typedef struct ZmodInfo {
	u8 resolution;
	float maxSampleRateMHz;
	u32 family; // 0 for adc, 1 for dac
} ZmodInfo;

ZmodInfo LookupZmodInfo (SzgDnaStrings DnaStrings) {
	ZmodInfo info;

	if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1410-40") == 0) {
		info.family = ZMODFAMILY_ZMODSCOPE;
		info.maxSampleRateMHz = 40.0f;
		info.resolution = 14;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1410-105") == 0) {
		info.family = ZMODFAMILY_ZMODSCOPE;
		info.maxSampleRateMHz = 105.0f;
		info.resolution = 14;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1410-125") == 0) {
		info.family = ZMODFAMILY_ZMODSCOPE;
		info.maxSampleRateMHz = 125.0f;
		info.resolution = 14;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1210-40") == 0) {
		info.family = ZMODFAMILY_ZMODSCOPE;
		info.maxSampleRateMHz = 40.0f;
		info.resolution = 12;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1210-125") == 0) {
		info.family = ZMODFAMILY_ZMODSCOPE;
		info.maxSampleRateMHz = 125.0f;
		info.resolution = 12;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1010-40") == 0) {
		info.family = ZMODFAMILY_ZMODSCOPE;
		info.maxSampleRateMHz = 40.0f;
		info.resolution = 10;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod ADC 1010-125") == 0) {
		info.family = ZMODFAMILY_ZMODSCOPE;
		info.maxSampleRateMHz = 125.0f;
		info.resolution = 10;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod DAC 1411-125") == 0) {
		info.family = ZMODFAMILY_ZMODAWG;
		info.maxSampleRateMHz = 125.0f;
		info.resolution = 14;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod Digitizer 1430-125") == 0) {
		info.family = ZMODFAMILY_ZMODDIGITIZER;
		info.maxSampleRateMHz = 125.0f;
		info.resolution = 14;
	}
	else if (strcmp(DnaStrings.szProductModel, "Zmod Digitizer 1450-105") == 0) {
		info.family = ZMODFAMILY_ZMODDIGITIZER;
		info.maxSampleRateMHz = 125.0f;
		info.resolution = 14;
	}
	else {
		info.family = ZMODFAMILY_UNSUPPORTED;
	}
	return info;
}

#define ZMOD_PORT_A_GROUPVIO 0
#define ZMOD_PORT_B_GROUPVIO 1

int main () {
	int fdI2cDev = 0; // this isn't using linux so this doesn't matter
	char *PortName;

	dpmutilPortInfo_t PortInfo[8] = {0};

	// enumerate the Syzygy ports to figure out which have Zmods installed
	dpmutilFEnum(FALSE, FALSE, PortInfo);

	SzgDnaHeader DnaHeader;
	SzgDnaStrings DnaStrings = {0};

	// Iterate over all of the ports enumerated
	for (u32 iPort = 0; iPort < 8; iPort++) {
		// check if a zmod is populated on that port
		if (PortInfo[iPort].portSts.fPresent == 0) {
			continue;
		}

		// Use group VIO to detect which port is which. For the Eclypse Z7, Zmod A = 0, Zmod B = 1
		switch (PortInfo[iPort].groupVio) {
		case ZMOD_PORT_A_GROUPVIO:
			PortName = "Zmod Port A";
			break;
		case ZMOD_PORT_B_GROUPVIO:
			PortName = "Zmod Port B";
			break;
		default:
			PortName = "Invalid VIO Group";
		}

		// Read the standard DNA information
		SyzygyReadDNAHeader(fdI2cDev, PortInfo[iPort].i2cAddr, &DnaHeader, FALSE);
		SyzygyReadDNAStrings(fdI2cDev, PortInfo[iPort].i2cAddr, &DnaHeader, &DnaStrings);

		// Figure out which Zmod is being used based on the name loaded in DNA
		switch (LookupZmodInfo(DnaStrings).family) {
		case ZMODFAMILY_ZMODSCOPE:
			printf("========= %s : %s Calibration Coefficients =========\r\n", PortName, DnaStrings.szProductName);
			FDisplayZmodADCCal(fdI2cDev, PortInfo[iPort].i2cAddr);
			ZMOD_ADC_CAL ADCFactoryCalibration, ADCUserCalibration;
			FGetZmodADCCal(fdI2cDev, PortInfo[iPort].i2cAddr, &ADCFactoryCalibration, &ADCUserCalibration);
			printf("\r\n");
			break;
		case ZMODFAMILY_ZMODAWG:
			printf("========= %s : %s Calibration Coefficients =========\r\n", PortName, DnaStrings.szProductName);
			FDisplayZmodDACCal(fdI2cDev, PortInfo[iPort].i2cAddr);
			ZMOD_DAC_CAL DACFactoryCalibration, DACUserCalibration;
			FGetZmodDACCal(fdI2cDev, PortInfo[iPort].i2cAddr, &DACFactoryCalibration, &DACUserCalibration);
			printf("\r\n");
			break;
		case ZMODFAMILY_ZMODDIGITIZER:
			printf("========= %s : %s Calibration Coefficients =========\r\n", PortName, DnaStrings.szProductName);
			FDisplayZmodDigitizerCal(fdI2cDev, PortInfo[iPort].i2cAddr);
			ZMOD_DIGITIZER_CAL DigitizerFactoryCalibration, DigitizerUserCalibration;
			FGetZmodDigitizerCal(fdI2cDev, PortInfo[iPort].i2cAddr, &DigitizerFactoryCalibration, &DigitizerUserCalibration);
			printf("\r\n");
			break;
		default:
			printf("========= Unsupported Zmod (%s) populated on %s =========\r\n", DnaStrings.szProductName, PortName);
		}

		// Free memory allocated to hold DNA strings like product name
		SyzygyFreeDNAStrings(&DnaStrings);
	}

	return 0;
}
