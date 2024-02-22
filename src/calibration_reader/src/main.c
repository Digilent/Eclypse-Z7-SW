#include "./dpmutil/dpmutil.h"
#include "stdio.h"

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

		// Read the product id
		DWORD Pdid;
		if (!FZmodReadPdid(fdI2cDev, PortInfo[iPort].i2cAddr, &Pdid)) {
			continue;
		}

		ZMOD_FAMILY Family;
		if (!FGetZmodFamily(Pdid, &Family)) {
			printf("========= Unsupported Zmod (%s) populated on %s =========\r\n", DnaStrings.szProductName, PortName);
			continue;
		}

		switch (Family) {
		case ZMOD_FAMILY_ADC:
			printf("========= %s : %s Calibration Coefficients =========\r\n", PortName, DnaStrings.szProductName);
			FDisplayZmodADCCal(fdI2cDev, PortInfo[iPort].i2cAddr);
			ZMOD_ADC_CAL ADCFactoryCalibration, ADCUserCalibration;
			FGetZmodADCCal(fdI2cDev, PortInfo[iPort].i2cAddr, &ADCFactoryCalibration, &ADCUserCalibration);
			printf("\r\n");
			break;
		case ZMOD_FAMILY_DAC:
			printf("========= %s : %s Calibration Coefficients =========\r\n", PortName, DnaStrings.szProductName);
			FDisplayZmodDACCal(fdI2cDev, PortInfo[iPort].i2cAddr);
			ZMOD_DAC_CAL DACFactoryCalibration, DACUserCalibration;
			FGetZmodDACCal(fdI2cDev, PortInfo[iPort].i2cAddr, &DACFactoryCalibration, &DACUserCalibration);
			printf("\r\n");
			break;
		case ZMOD_FAMILY_DIGITIZER:
			printf("========= %s : %s Calibration Coefficients =========\r\n", PortName, DnaStrings.szProductName);
			FDisplayZmodDigitizerCal(fdI2cDev, PortInfo[iPort].i2cAddr);
			ZMOD_DIGITIZER_CAL DigitizerFactoryCalibration, DigitizerUserCalibration;
			FGetZmodDigitizerCal(fdI2cDev, PortInfo[iPort].i2cAddr, &DigitizerFactoryCalibration, &DigitizerUserCalibration);
			printf("\r\n");
			break;
		}

		// Free memory allocated to hold DNA strings like product name
		SyzygyFreeDNAStrings(&DnaStrings);
	}

	return 0;
}
