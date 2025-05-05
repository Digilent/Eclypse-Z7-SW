/************************************************************************/
/*                                                                      */
/*  dpmutil.c  --  Digilent Platform Management Utility                 */
/*                                                                      */
/************************************************************************/
/*  Author: Michael T. Alexander, Thomas Kappenman                      */
/*  Copyright 2019 Digilent, Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  The Digilent Platform Management Utility (dpmutil) provides an      */
/*  API for discovering Zmods (SYZYGY Pods) attached                    */
/*  to a Digilent platform board, acquiring information about them,     */
/*  acquiring information about the Digilent platform board, and        */
/*  setting certain settings pertaining to the configuration of the     */
/*  Digilent platform board.                                            */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  08/21/2019(MichaelA): Created                                       */
/*	10/25/2019(MichaelA): modified FEnum to retrieve and display the    */
/*		PDID for SYZYGY Pods manufactured by Digilent	                */
/*	10/28/2019(MichaelA): modified OpenI2cController to search the      */
/*		"/sys/bus/i2c/devices" directory for a device whose	            */
/*		"device-name" is "pmcu-i2c" and if found open that device. If   */
/*		no such device is found then we assume "/dev/i2c-0" is the      */
/*		device entry corresponding to the I2C controller with the PMCU  */
/*		I2C bus                                                         */
/*	01/03/2020(MichaelA): modified FEnum to add support for displaying  */
/*		the 5V0, 3V3, and VIO voltages read by the pMCU on the ZmodLOOP	*/
/*	01/03/2020(MichaelA): added FWriteDNA function to add the ability   */
/*		to write binary data to the DNA section of the pMCU flash       */
/*	01/06/2020(MichaelA): modified FWriteDNA to add support for magic   */
/*		numbers for disabling flash write protection. Write protection  */
/*		is restored after the write operation completes                 */
/*  01/13/2020(MichaelA): modified FEnum to add support for displaying  */
/*      the calibration constants of the ZmodADC and ZmodDAC            */
/*  01/13/2020(MichaelA): moved code specific to ZmodLOOP to ZmodLOOP.h */
/*      and ZmodLOOP.c                                                  */
/*	05/04/2020(ThomasK): namechange to dpmutil.c. Changed from linux    */
/* 		console app to baremetal api                                    */
/*                                                                      */
/************************************************************************/

/* ------------------------------------------------------------ */
/*                  Include File Definitions                    */
/* ------------------------------------------------------------ */


#if defined(__linux__)
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <linux/i2c-dev.h>
#include <time.h>
#else
#include "sleep.h"
#endif
#include "dpmutil.h"
#include <stdio.h>

/* ------------------------------------------------------------ */
/*                  Miscellaneous Declarations                  */
/* ------------------------------------------------------------ */

/* The following addresses are utilized to retrieve information
** from Digilent SYZYGY pods (ZMODs).
*/
#define addrPdid			0x80FC

/* The following macro can be utilized to convert a Digilent PDID
** into a product.
*/
#define ProductFromPdid(pdid)   ((pdid >> 20) & 0xFFF)

/* ------------------------------------------------------------ */
/*                  Local Type Definitions                      */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/*                   Global Variables                           */
/* ------------------------------------------------------------ */

BOOL dpmutilfVerbose;

/* ------------------------------------------------------------ */
/*                  Local Variables                             */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/*                  Procedure Definitions                       */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/***    dpmutilFGetInfo
**
**  Parameters:
**      pDevInfo		- Pointer to a dpmutilDevInfo_t object to store data
**
**  Return Values:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**
**  Description:
**      Get general configuration and information about the supported
**      features of the Platform MCU (PMCU). This function communicates
**      with the PMCU over the I2C bus to retrieve general information
**      about the capabilities of the PMCU and the board configuration.
**      This information includes the PMCU firwmare revision, SmartVIO
**      port count, power supply group counts (5V0, 3V3, VADJ), the
**      number of temperature probes supported by the board, and the
**      number of fans supported by the board. If the board supports
**      one or more temperature probe then the capabilities of each
**      supported probe and the most recent temperature measurement
**      of that probe are displayed via the console. If the board
**      supports one or more fan then the capabilities of each
**      supported fan are displayed via the console and if a fan supports
**      RPM measurement then the most recent RPM measurement is also
**      displayed.
*/
BOOL
dpmutilFGetInfo(dpmutildevInfo_t* pDevInfo) {

	int						fdI2c;
	WORD					wTemp;
	BYTE					i;

	fdI2c = -1;
#if defined(__linux__)

	fdI2c = I2CHALOpenI2cController();
	if ( 0 > fdI2c ) {
		printf("ERROR: failed to open file descriptor for I2C device\n");
		goto lErrorExit;
	}
#else
	if(!I2CHALInit(0)){
		printf("ERROR: failed to initialize I2C device\n");
		goto lErrorExit;
	}
#endif
	/* Read and display the PDID.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrPDID, (BYTE*)(&pDevInfo->pdid), 4, NULL) ) {
		printf("ERROR: failed to read PDID\n");
		goto lErrorExit;
	}
	if(dpmutilfVerbose)printf("PMCU_PDID:                       0x%08X\n", (unsigned int)pDevInfo->pdid);

	/* Read and display the firmware revision number.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrFirmwareVersion, (BYTE*)(&wTemp), 2, NULL) ) {
		printf("ERROR: failed to read FIRMWARE_VERSION register\n");
		goto lErrorExit;
	}
	if(dpmutilfVerbose)printf("PMCU_FIRMWARE_VERSION:           %d.%d\n", wTemp >> 8, wTemp & 0xFF);
	pDevInfo->fwVer = wTemp / (1<<8);

	/* Read and display the configuration revision number.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrConfigurationVersion, (BYTE*)(&wTemp), 2, NULL) ) {
		printf("ERROR: failed to read CONFIGURATION_VERSION register\n");
		goto lErrorExit;
	}
	if(dpmutilfVerbose)printf("PMCU_CONFIGURATION_VERSION:      %d.%d\n", wTemp >> 8, wTemp & 0xFF);
	pDevInfo->cfgVer = wTemp / (1<<8);

	/* Read and display the platform configuration.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrPlatformConfig, (BYTE*)(&pDevInfo->platcfg), 2, NULL) ) {
		printf("ERROR: failed to read PLATFORM_CONFIGURATION register\n");
		goto lErrorExit;
	}

	if(dpmutilfVerbose){
		memcpy(&wTemp, &(pDevInfo->platcfg), 2);
		printf("PLATFORM_CONFIGURATION:          0x%04X\n", wTemp);
		printf("    ENFORCE_5V0_CURRENT_LIMIT    [%c]\n", pDevInfo->platcfg.fEnforce5v0CurLimit ? 'Y':'N');
		printf("    ENFORCE_3V3_CURRENT_LIMIT    [%c]\n", pDevInfo->platcfg.fEnforce3v3CurLimit ? 'Y':'N');
		printf("    ENFORCE_VIO_CURRENT_LIMIT    [%c]\n", pDevInfo->platcfg.fEnforceVioCurLimit ? 'Y':'N');
		printf("    PERFORM_SYZYGY_CRC_CHECK     [%c]\n", pDevInfo->platcfg.fPerformCrcCheck ? 'Y':'N');
	}

	/* Read and display the SmartVio port count.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrPortCount, &pDevInfo->cntVioPort, 1, NULL) ) {
		printf("ERROR: failed to read SMARTVIO_PORT_COUNT register\n");
		goto lErrorExit;
	}
	if(dpmutilfVerbose)printf("SMARTVIO_PORT_COUNT:             %d\n", pDevInfo->cntVioPort);

	/* Read and display the 5V0 group count.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddr5v0GroupCount, &pDevInfo->cnt5v0, 1, NULL) ) {
		printf("ERROR: failed to read 5V0_GROUP_COUNT register\n");
		goto lErrorExit;
	}
	if(dpmutilfVerbose)printf("5V0_GROUP_COUNT:                 %d\n", pDevInfo->cnt5v0);

	/* Read and display the 3V3 group count.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddr3v3GroupCount, &pDevInfo->cnt3v3, 1, NULL) ) {
		printf("ERROR: failed to read 3V3_GROUP_COUNT register\n");
		goto lErrorExit;
	}
	if(dpmutilfVerbose)printf("3V3_GROUP_COUNT:                 %d\n", pDevInfo->cnt3v3);

	/* Read and display the VADJ group count.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrVadjGroupCount, &pDevInfo->cntVadj, 1, NULL) ) {
		printf("ERROR: failed to read VADJ_GROUP_COUNT register\n");
		goto lErrorExit;
	}
	if(dpmutilfVerbose)printf("VADJ_GROUP_COUNT:                %d\n", pDevInfo->cntVadj);

	/* Read and display the temperature probe count.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrTempProbeCount, &pDevInfo->cntProbe, 1, NULL) ) {
		printf("ERROR: failed to read TEMPERATURE_PROBE_COUNT register\n");
		goto lErrorExit;
	}
	if(dpmutilfVerbose)printf("TEMPERATURE_PROBE_COUNT:         %d\n", pDevInfo->cntProbe);

	for ( i = 0; i < pDevInfo->cntProbe; i++ ) {

		/* Read and display this temperature probe's capabilities.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrTemp1Attributes + (offsetTemperatureReg*i), (BYTE*)&pDevInfo->probeAttr[i], 1, NULL) ) {
			printf("ERROR: failed to read TEMPERATURE_%d_ATTRIBUTES register\n", i+1);
			goto lErrorExit;
		}
		if(dpmutilfVerbose){
			printf("    TEMPERATURE_%d_CAPABILITIES:  0x%02X\n", i + 1, pDevInfo->probeAttr[i].fs);
			printf("        PRESENT                  [%c]\n", pDevInfo->probeAttr[i].fPresent ? 'Y' : 'N');
			printf("        LOCATION                 ");
			switch ( pDevInfo->probeAttr[i].tlocation ) {
				case tlocationFpgaCpu1:
					printf("FPGA/CPU_1\n");
					break;
				case tlocationFpgaCpu2:
					printf("FPGA/CPU_2\n");
					break;
				case tlocationExternal1:
					printf("EXTERNAL_1\n");
					break;
				case tlocationExternal2:
					printf("EXTERNAL_2\n");
					break;
				default:
					printf("UNKNOWN\n");
					break;
			}
			printf("        TEMPERATURE_FORMAT       ");
			switch ( pDevInfo->probeAttr[i].tformat ) {
				case tformatDegCDecimal:
					printf("Degrees C (decimal)\n");
					break;
				case tformatDegCFixedPoint:
					printf("Degrees C (fixed point)\n");
					break;
				case tformatDegFDecimal:
					printf("Degrees F (decimal)\n");
					break;
				case tformatDegFFixedPoint:
					printf("Degrees F (fixed point)\n");
					break;
				default:
					printf("UNKNOWN\n");
					break;
			}
		}

		/* Read and display this probe's temperature.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrTemp1 + (offsetTemperatureReg*i), (BYTE*)&pDevInfo->temp[i], 2, NULL) ) {
			printf("ERROR: failed to read TEMPERATURE_%d register\n", i+1);
			goto lErrorExit;
		}
		if(dpmutilfVerbose){
			printf("    TEMPERATURE_%d:               ", i + 1);
			switch ( pDevInfo->probeAttr[i].tformat ) {
				case tformatDegCDecimal:
					printf("%hd Degrees C\n", pDevInfo->temp[i]);
					break;
				case tformatDegCFixedPoint:
					printf("%8.2f Degrees C\n", pDevInfo->temp[i] / 256.0);
					break;
				case tformatDegFDecimal:
					printf("%hd Degrees F\n", pDevInfo->temp[i]);
					break;
				case tformatDegFFixedPoint:
					printf("%8.2f Degrees F\n", pDevInfo->temp[i] / 256.0);
					break;
				default:
					printf("UNKNOWN\n");
					break;
			}

			if ( (i+1) != pDevInfo->cntProbe ) {
				printf("\n");
			}
		}
	}

	/* Read and display the fan count.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrFanCount, &pDevInfo->cntFan, 1, NULL) ) {
		printf("ERROR: failed to read FAN_COUNT register\n");
		goto lErrorExit;
	}
	if(dpmutilfVerbose)printf("FAN_COUNT:                       %d\n", pDevInfo->cntFan);

	for ( i = 0; i < pDevInfo->cntFan; i++ ) {

		/* Read and display this fan's capabilities.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrFan1Capabilities + (offsetFanReg*i), (BYTE*)&pDevInfo->fanCapabilities[i], 1, NULL) ) {
			printf("ERROR: failed to read FAN_%d_CAPABILITIES register\n", i+1);
			goto lErrorExit;
		}
		if(dpmutilfVerbose){
			printf("    FAN_%d_CAPABILITIES:          0x%02X\n", i + 1, pDevInfo->fanCapabilities[i].fs);
			printf("        ENABLE_AND_DISABLE       [%c]\n", pDevInfo->fanCapabilities[i].fcapEnable ? 'Y' : 'N');
			printf("        SET_FIXED_SPEED          [%c]\n", pDevInfo->fanCapabilities[i].fcapSetSpeed ? 'Y' : 'N');
			printf("        AUTO_SPEED_CONTROL       [%c]\n", pDevInfo->fanCapabilities[i].fcapAutoSpeed ? 'Y' : 'N');
			printf("        MEASURE_RPM              [%c]\n", pDevInfo->fanCapabilities[i].fcapMeasureRpm ? 'Y' : 'N');
		}
		/* Read and display this fan's configuration.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrFan1Config + (offsetFanReg*i), (BYTE*)&pDevInfo->fanConfig[i], 1, NULL) ) {
			printf("ERROR: failed to read FAN_%d_CONFIGURATION register\n", i+1);
			goto lErrorExit;
		}
		if(dpmutilfVerbose){
			printf("    FAN_%d_CONFIGURATION:         0x%02X\n", i + 1, pDevInfo->fanConfig[i].fs);
			printf("        ENABLE                   [%c]\n", pDevInfo->fanConfig[i].fEnable ? 'Y' : 'N');
			printf("        SPEED                    ");
			switch ( pDevInfo->fanConfig[i].fspeed ) {
				case fancfgMinimumSpeed:
					printf("MINIMUM\n");
					break;
				case fancfgMediumSpeed:
					printf("MEDIUM\n");
					break;
				case fancfgMaximumSpeed:
					printf("MAXIMUM\n");
					break;
				case fancfgAutoSpeed:
					printf("AUTOMATIC\n");
					break;
				default:
					printf("UNKNOWN\n");
					break;
			}
			printf("        TEMPERATURE_SOURCE       ");
			switch ( pDevInfo->fanConfig[i].tempsrc ) {
				case fancfgTempProbeNone:
					printf("NONE\n");
					break;
				case fancfgTempProbe1:
					printf("TEMP_PROBE_1\n");
					break;
				case fancfgTempProbe2:
					printf("TEMP_PROBE_2\n");
					break;
				case fancfgTempProbe3:
					printf("TEMP_PROBE_3\n");
					break;
				case fancfgTempProbe4:
					printf("TEMP_PROBE_4\n");
					break;
				default:
					printf("UNKNOWN\n");
					break;
			}

			printf("        MEASURE_RPM              [%c]\n", pDevInfo->fanCapabilities[i].fcapMeasureRpm ? 'Y' : 'N');
		}

		/* Read and display this fan's RPM.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrFan1Rpm + (offsetFanReg*i), (BYTE*)(&pDevInfo->fanRPM[i]), 2, NULL) ) {
			printf("ERROR: failed to read FAN_%d_RPM register\n", i+1);
			goto lErrorExit;
		}
		if(dpmutilfVerbose){
			printf("    FAN_%d_RPM:                   %d\n", i+1, pDevInfo->fanRPM[i]);


			if ( (i+1) != pDevInfo->cntFan ) {
				printf("\n");
			}
		}
	}

#if defined(__linux__)
	/* Close the I2C controller file descriptor.
	*/
	close(fdI2c);
#endif

	return fTrue;

lErrorExit:
#if defined(__linux__)
	if ( 0 <= fdI2c ) {
		close(fdI2c);
	}
#endif
	return fFalse;
}


/* ------------------------------------------------------------ */
/***    dpmutilFGetInfoPower
**
**  Parameters:
**      chanid			- channel id to get power from. -1 to scan all
**      pPowerInfo		- Pointer to a dpmutilPowerInfo_t array [8] to store data
**
**  Return Values:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**
**  Description:
**      Get  information about the on board power supplies (5V0, 3V3, VIO)
**      that are associated with the on board SmartVIO ports. This
**      function communicates with the Platform MCU (PMCU) over I2C to
**      determine the number of on board 5V0, 3V3, and VIO power supplies
**      that are associated with the on board VIO ports and to retrieve
**      various information about each of these supplies.
**
**      The "chanid <0...7>" parameter can be used to specify
**      the channel ID of a specific power supply. If chanid = -1 this function 
**		will retrieve and display information
**      for every channel supported by the board.
*/
BOOL
dpmutilFGetInfoPower(int chanid, dpmutilPowerInfo_t pPowerInfo[]) {

	BOOL	fRet;

	fRet = fTrue;

	if ( ! dpmutilFGetInfo5V0(chanid, pPowerInfo) ) {
		fRet = fFalse;
	}

	if(dpmutilfVerbose)printf("\n");

	if ( ! dpmutilFGetInfo3V3(chanid, pPowerInfo) ) {
		fRet = fFalse;
	}

	if(dpmutilfVerbose)printf("\n");

	if ( ! dpmutilFGetInfoVio(chanid, pPowerInfo) ) {
		fRet = fFalse;
	}

	return fRet;
}

/* ------------------------------------------------------------ */
/***    dpmutilFGetInfo5V0
**
**  Parameters:
**      chanid			- channel id to get power from. -1 to scan all
**      pPowerInfo		- Pointer to a dpmutilPowerInfo_t array [8] to store data
**
**  Return Values:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**
**  Description:
**      Get  information about the on board 5V0 power supplies that are
**      associated with the on board SmartVIO ports. This function
**      communicates with the Platform MCU (PMCU) over I2C to
**      determine the number of on board 5V0 power supplies, to retrieve
**      the amount of current that each supply is capable of providing,
**      and to retrieve the sum of current requested by all SmartVIO
**      ports that are associated with each supply. All of this information
**      is output to the console.
**
**      The "chanid <0...7>" parameter can be used to specify
**      the channel ID of a specific power supply. If chanid = -1 this function
**		will retrieve and display information
**      for every channel supported by the board.
*/
BOOL
dpmutilFGetInfo5V0(int chanid, dpmutilPowerInfo_t pPowerInfo[]) {

	int				fdI2c;
	BYTE			csupply;
	BYTE			isupply;

	fdI2c = -1;
#if defined(__linux__)
	fdI2c = I2CHALOpenI2cController();
	if ( 0 > fdI2c ) {
		printf("ERROR: failed to open file descriptor for I2C device\n");
		goto lErrorExit;
	}
#else
	if(!I2CHALInit(0)){
		printf("ERROR: failed to initialize I2C device\n");
		goto lErrorExit;
	}
#endif
	/* Determine how many 5V0 supplies there are.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddr5v0GroupCount, &csupply, 1, NULL) ) {
		printf("ERROR: failed to read 5V0_GROUP_COUNT register\n");
		goto lErrorExit;
	}

	if ( chanid != -1 ) {
		if ( chanid >= csupply ) {
			printf("ERROR: device has %d 5V0 supplies. Channel %d is\n", csupply, chanid);
			printf("not supported by this device\n");
			goto lErrorExit;
		}

		isupply = chanid;
		csupply = chanid + 1;
	}
	else {
		if(dpmutilfVerbose){
			if ( 1 < csupply ) {
				printf("Found %d 5V0 supplies\n", csupply);
			}
			else {
				printf("Found 1 5V0 supply\n");
			}
		}
		isupply = 0;
	}

	while ( isupply < csupply ) {

		if(dpmutilfVerbose)printf("Supply: 5V0_%c\n", 0x41 + isupply);

		/* Read and display the current allowed for the supply.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddr5v0ACurrentAllowed + (offset5v0Reg*isupply), (BYTE*)&pPowerInfo[isupply].currentAllowed5v0, 2, NULL) ) {
			printf("ERROR: failed to read 5V0_%c_CURRENT_ALLOWED register\n", 0x41 + isupply);
			goto lErrorExit;
		}
		if(dpmutilfVerbose)printf("    5V0_%c_CURRENT_ALLOWED:       %d mA\n", 0x41 + isupply, pPowerInfo[isupply].currentAllowed5v0);

		/* Read and display the current requested for the supply.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddr5v0ACurrentRequested + (offset3v3Reg*isupply), (BYTE*)&pPowerInfo[isupply].currentRequested5v0, 2, NULL) ) {
			printf("ERROR: failed to read 5V0_%c_CURRENT_REQUESTED register\n", 0x41 + isupply);
			goto lErrorExit;
		}
		if(dpmutilfVerbose)printf("    5V0_%c_CURRENT_REQUESTED:     %d mA\n", 0x41 + isupply, pPowerInfo[isupply].currentRequested5v0);

		isupply++;
		if(dpmutilfVerbose){
			if ( isupply != csupply ) {
				printf("\n");
			}
		}
	}

#if defined(__linux__)
	/* Close the I2C controller file descriptor.
	*/
	close(fdI2c);
#endif

	return fTrue;

lErrorExit:
#if defined(__linux__)
	if ( 0 <= fdI2c ) {
		close(fdI2c);
	}
#endif

	return fFalse;
}

/* ------------------------------------------------------------ */
/***    dpmutilFGetInfo3V3
**
**  Parameters:
**      chanid			- channel id to get power from. -1 to scan all
**      pPowerInfo		- Pointer to a dpmutilPowerInfo_t array [8] to store data
**
**  Return Values:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**
**  Description:
**      Get  information about the on board 3V3 power supplies that are
**      associated with the on board SmartVIO ports. This function
**      communicates with the Platform MCU (PMCU) over I2C to
**      determine the number of on board 3V3 power supplies, to retrieve
**      the amount of current that each supply is capable of providing,
**      and to retrieve the sum of current requested by all SmartVIO
**      ports that are associated with each supply. All of this information
**      is output to the console.
**
**      The "chanid <0...7>" parameter can be used to specify
**      the channel ID of a specific power supply. If chanid = -1 this function
**		will retrieve and display information
**      for every channel supported by the board.
*/
BOOL
dpmutilFGetInfo3V3(int chanid, dpmutilPowerInfo_t pPowerinfo[]) {

	int				fdI2c;
	BYTE			csupply;
	BYTE			isupply;

	fdI2c = -1;
#if defined(__linux__)
	fdI2c = I2CHALOpenI2cController();
	if ( 0 > fdI2c ) {
		printf("ERROR: failed to open file descriptor for I2C device\n");
		goto lErrorExit;
	}
#else
	if(!I2CHALInit(0)){
		printf("ERROR: failed to initialize I2C device\n");
		goto lErrorExit;
	}
#endif

	/* Determine how many 3V3 supplies there are.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddr3v3GroupCount, &csupply, 1, NULL) ) {
		printf("ERROR: failed to read 3V3_GROUP_COUNT register\n");
		goto lErrorExit;
	}

	if ( chanid != -1 ) {
		if ( chanid >= csupply ) {
			printf("ERROR: device has %d 3V3 supplies. Channel %d is\n", csupply, chanid);
			printf("not supported by this device\n");
			goto lErrorExit;
		}

		isupply = chanid;
		csupply = chanid + 1;
	}
	else {
		if(dpmutilfVerbose){
			if ( 1 < csupply ) {
				printf("Found %d 3V3 supplies\n", csupply);
			}
			else {
				printf("Found 1 3V3 supply\n");
			}
		}
		isupply = 0;
	}

	while ( isupply < csupply ) {

		if(dpmutilfVerbose)printf("Supply: 3V3_%c\n", 0x41 + isupply);

		/* Read and display the current allowed for the supply.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddr3v3ACurrentAllowed + (offset3v3Reg*isupply), (BYTE*)&pPowerinfo[isupply].currentAllowed3v3, 2, NULL) ) {
			printf("ERROR: failed to read 3V3_%c_CURRENT_ALLOWED register\n", 0x41 + isupply);
			goto lErrorExit;
		}
		if(dpmutilfVerbose)printf("    3V3_%c_CURRENT_ALLOWED:       %d mA\n", 0x41 + isupply, pPowerinfo[isupply].currentAllowed3v3);

		/* Read and display the current requested for the supply.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddr3v3ACurrentRequested + (offset3v3Reg*isupply), (BYTE*)&pPowerinfo[isupply].currentRequested3v3, 2, NULL) ) {
			printf("ERROR: failed to read 3V3_%c_CURRENT_REQUESTED register\n", 0x41 + isupply);
			goto lErrorExit;
		}
		if(dpmutilfVerbose)printf("    3V3_%c_CURRENT_REQUESTED:     %d mA\n", 0x41 + isupply, pPowerinfo[isupply].currentRequested3v3);

		isupply++;
		if(dpmutilfVerbose){
			if ( isupply != csupply ) {
				printf("\n");
			}
		}
	}
#if defined(__linux__)
	/* Close the I2C controller file descriptor.
	*/
	close(fdI2c);
#endif
	return fTrue;

lErrorExit:
#if defined(__linux__)
	if ( 0 <= fdI2c ) {
		close(fdI2c);
	}
#endif
	return fFalse;
}

/* ------------------------------------------------------------ */
/***    dpmutilFGetInfoVio
**
**  Parameters:
**      chanid			- channel id to get power from. -1 to scan all
**      pPowerInfo		- Pointer to a dpmutilPowerInfo_t array [8] to store data
**
**  Return Values:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**
**  Description:
**      Get  information about the on board VIO (VADJ) power supplies that
**      are associated with the on board SmartVIO ports. This function
**      communicates with the Platform MCU (PMCU) over I2C to
**      determine the number of on board VIO power supplies, to retrieve
**      the amount of current that each supply is capable of providing,
**      to retrieve the sum of current requested by all SmartVIO
**      ports that are associated with each supply, and to retrieve all
**      status and configuration information associated with each supply.
**      All of this information is output to the console.
**
**      The "chanid <0...7>" parameter can be used to specify
**      the channel ID of a specific power supply. If chanid = -1 this function
**		will retrieve and display information
**      for every channel supported by the board.
*/
BOOL
dpmutilFGetInfoVio(int chanid, dpmutilPowerInfo_t pPowerInfo[]) {

	int				fdI2c;
	BYTE			cvadj;
	BYTE			ivadj;
	VADJ_STATUS		vadjsts;

	fdI2c = -1;
#if defined(__linux__)
	fdI2c = I2CHALOpenI2cController();
	if ( 0 > fdI2c ) {
		printf("ERROR: failed to open file descriptor for I2C device\n");
		goto lErrorExit;
	}
#else
	if(!I2CHALInit(0)){
		printf("ERROR: failed to initialize I2C device\n");
		goto lErrorExit;
	}
#endif
	/* Determine how many VADJ supplies there are.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrVadjGroupCount, &cvadj, 1, NULL) ) {
		printf("ERROR: failed to read VADJ_GROUP_COUNT register\n");
		goto lErrorExit;
	}

	/* Get the status for all VADJ supplies.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrVadjStatus, (BYTE*)&vadjsts, 2, NULL) ) {
		printf("ERROR: failed to read VADJ_STATUS register\n");
		goto lErrorExit;
	}

	if ( chanid != -1 ) {
		if ( chanid >= cvadj ) {
			printf("ERROR: device has %d VIO supplies. Channel %d is\n", cvadj, chanid);
			printf("not supported by this device\n");
			goto lErrorExit;
		}

		ivadj = chanid;
		cvadj = chanid + 1;
	}
	else {
		if(dpmutilfVerbose){
			if ( 1 < cvadj ) {
				printf("Found %d VIO supplies\n", cvadj);
			}
			else {
				printf("Found 1 VIO supply\n");
			}
		}
		ivadj = 0;
	}

	while ( ivadj < cvadj ) {

		if(dpmutilfVerbose){
			printf("Supply: VADJ_%c\n", 0x41 + ivadj);

			printf("    VADJ_%c_ENABLED:              [%c]\n", 0x41 + ivadj, (vadjsts.fsEn & (1<<ivadj)) ? 'Y' : 'N');
			printf("    VADJ_%c_POWER_GOOD:           [%c]\n", 0x41 + ivadj, (vadjsts.fsPgood & (1<<ivadj)) ? 'Y' : 'N');
		}

		/* Read and display the voltage setting for the current supply.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrVadjAVoltage + (offsetVadjReg*ivadj), (BYTE*)&pPowerInfo[ivadj].vadjVoltage, 2, NULL) ) {
			printf("ERROR: failed to read VADJ_%c_VOLTAGE register\n", 0x41 + ivadj);
			goto lErrorExit;
		}
		if(dpmutilfVerbose)printf("    VADJ_%c_VOLTAGE:              %d mV\n", 0x41 + ivadj, pPowerInfo[ivadj].vadjVoltage * 10);

		/* Read and display the current allowed for the supply.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrVadjACurrentAllowed + (offsetVadjReg*ivadj), (BYTE*)&pPowerInfo[ivadj].currentAllowedVadj, 2, NULL) ) {
			printf("ERROR: failed to read VADJ_%c_CURRENT_ALLOWED register\n", 0x41 + ivadj);
			goto lErrorExit;
		}
		if(dpmutilfVerbose)printf("    VADJ_%c_CURRENT_ALLOWED:      %d mA\n", 0x41 + ivadj, pPowerInfo[ivadj].currentAllowedVadj);

		/* Read and display the current requested for the supply.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrVadjACurrentRequested + (offsetVadjReg*ivadj), (BYTE*)&pPowerInfo[ivadj].currentRequestedVadj, 2, NULL) ) {
			printf("ERROR: failed to read VADJ_%c_CURRENT_REQUESTED register\n", 0x41 + ivadj);
			goto lErrorExit;
		}
		if(dpmutilfVerbose)printf("    VADJ_%c_CURRENT_REQUESTED:    %d mA\n", 0x41 + ivadj, pPowerInfo[ivadj].currentRequestedVadj);

		/* Read and display the override register for the supply.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrVadjAOverride + (offsetVadjReg*ivadj), (BYTE*)&pPowerInfo[ivadj].vadjOverride, 2, NULL) ) {
			printf("ERROR: failed to read VADJ_%c_OVERRIDE register\n", 0x41 + ivadj);
			goto lErrorExit;
		}
		if(dpmutilfVerbose){
		printf("    VADJ_%c_OVERRIDE:             0x%04X\n", 0x41 + ivadj, pPowerInfo[ivadj].vadjOverride.fs);
		printf("        ENABLE_OVERRIDE          [%c]\n", pPowerInfo[ivadj].vadjOverride.fOverride ? 'Y' : 'N');
		printf("        ENABLE_SUPPLY            [%c]\n", pPowerInfo[ivadj].vadjOverride.fEnable ? 'Y' : 'N');
		printf("        VOLTAGE_TO_SET           %d mV\n", pPowerInfo[ivadj].vadjOverride.vltgSet * 10);
		}
		ivadj++;
		if(dpmutilfVerbose){
			if ( ivadj != cvadj ) {
				printf("\n");
			}
		}
	}

#if defined(__linux__)
	/* Close the I2C controller file descriptor.
	*/
	close(fdI2c);
#endif
	return fTrue;

lErrorExit:
#if defined(__linux__)
	if ( 0 <= fdI2c ) {
		close(fdI2c);
	}
#endif
	return fFalse;
}


/* ------------------------------------------------------------ */
/***    dpmutilFEnum
**
**  Parameters:
**      setCrcCheck			- Flag to set crcCheck or not
**      crcCheck			- False to skip crcCheck when reading Syzygy DNA header
**      pPortInfo			- dpmutilPortInfo_t object array [8] to store data
**
**  Return Values:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**
**  Description:
**      Enumerate SmartVIO ports. This function communicates with the
**      Platform MCU over the I2C bus to determine how many SmartVIO ports
**      the board supports and to retrieve the configuration and status of
**      those ports. If a SmartVIO port has a SYZYGY pod installed then
**      the I2C bus is used to retrieve the Standard SYZYGY firmware
**      registers and the SYZYGY DNA (including all string fields) and that
**      information is output to the console.
*/
BOOL
dpmutilFEnum(BOOL setCrcCheck, BOOL crcCheck, dpmutilPortInfo_t pPortInfo[]) {

	int				fdI2c;
	BYTE			csvioPorts;
	BYTE			isvioPort;
	VADJ_STATUS		vadjsts;
	SzgStdFwRegs	szgstdfwRegs;
	SzgDnaHeader	szgdnaHeader;
	SzgDnaStrings	szgdnaStrings;
	DWORD			pdid;

	fdI2c = -1;
	memset(&szgdnaStrings, 0, sizeof(SzgDnaStrings));
#if defined(__linux__)
	fdI2c = I2CHALOpenI2cController();
	if ( 0 > fdI2c ) {
		printf("ERROR: failed to open file descriptor for I2C device\n");
		goto lErrorExit;
	}
#else
	if(!I2CHALInit(0)){
		printf("ERROR: failed to initialize I2C device\n");
		goto lErrorExit;
	}
#endif

	/* Get the status for all VADJ supplies.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrVadjStatus, (BYTE*)&vadjsts, 2, NULL) ) {
		printf("ERROR: failed to read VADJ_STATUS register\n");
		goto lErrorExit;
	}

	/* Determine how many SmartVIO ports the board contains.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrPortCount, &csvioPorts, 1, NULL) ) {
		printf("ERROR: failed to read SMART_VIO_PORT_COUNT register\n");
		goto lErrorExit;
	}

	if(dpmutilfVerbose)printf("Found %d SmartVIO port(s)\n", csvioPorts);

	for ( isvioPort = 0; isvioPort < csvioPorts; isvioPort++ ) {

		if(dpmutilfVerbose)printf("\nPort: %c\n", 0x41 + isvioPort);

		/* Read and display the I2C address for this port.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrPortAI2cAddress + (offsetPortReg*isvioPort), &pPortInfo[isvioPort].i2cAddr, 1, NULL) ) {
			printf("ERROR: failed to read PORT_%c_I2C_ADDRESS register\n", 0x41 + isvioPort);
			goto lErrorExit;
		}
		if(dpmutilfVerbose)printf("    PORT_%c_I2C_ADDRESS:    0x%02X\n", 0x41 + isvioPort, pPortInfo[isvioPort].i2cAddr);

		/* Read and display the 5V0 group for this port.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrPortA5v0Group + (offsetPortReg*isvioPort), &pPortInfo[isvioPort].group5v0, 1, NULL) ) {
			printf("ERROR: failed to read PORT_%c_5V0_GROUP register\n", 0x41 + isvioPort);
			goto lErrorExit;
		}
		if(dpmutilfVerbose)printf("    PORT_%c_5V0_GROUP:      %d\n", 0x41 + isvioPort, pPortInfo[isvioPort].group5v0);

		/* Read and display the 3V3 group for this port.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrPortA3v3Group + (offsetPortReg*isvioPort), &pPortInfo[isvioPort].group3v3, 1, NULL) ) {
			printf("ERROR: failed to read PORT_%c_3V3_GROUP register\n", 0x41 + isvioPort);
			goto lErrorExit;
		}
		if(dpmutilfVerbose)printf("    PORT_%c_3V3_GROUP:      %d\n", 0x41 + isvioPort, pPortInfo[isvioPort].group3v3);

		/* Read and display the VIO group for this port.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrPortAVioGroup + (offsetPortReg*isvioPort), &pPortInfo[isvioPort].groupVio, 1, NULL) ) {
			printf("ERROR: failed to read PORT_%c_VIO_GROUP register\n", 0x41 + isvioPort);
			goto lErrorExit;
		}
		if(dpmutilfVerbose)printf("    PORT_%c_VIO_GROUP:      %d\n", 0x41 + isvioPort, pPortInfo[isvioPort].groupVio);

		/* Read and display the port type for this port.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrPortAType + (offsetPortReg*isvioPort), &pPortInfo[isvioPort].portType, 1, NULL) ) {
			printf("ERROR: failed to read PORT_%c_I2C_ADDRESS register\n", 0x41 + isvioPort);
			goto lErrorExit;
		}
		if(dpmutilfVerbose){
			printf("    PORT_%c_TYPE:           0x%02X (", 0x41 + isvioPort, pPortInfo[isvioPort].portType);
			switch ( pPortInfo[isvioPort].portType ) {
				case ptypeSyzygyStd:
					printf("SYZYGY_STD)\n");
					break;
				case ptypeSyzygyTxr2:
					printf("SYZYGY_TXR2)\n");
					break;
				case ptypeSyzygyTxr4:
					printf("SYZYGY_TXR4)\n");
					break;
				case ptypeNone:
				default:
					printf("UNKNOWN)\n");
					break;
			}
		}

		/* Read and display the status for this port.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrPortAStatus + (offsetPortReg*isvioPort), (BYTE*)&pPortInfo[isvioPort].portSts, 1, NULL) ) {
			printf("ERROR: failed to read PORT_%c_I2C_ADDRESS register\n", 0x41 + isvioPort);
			goto lErrorExit;
		}
		if(dpmutilfVerbose){
			printf("    PORT_%c_STATUS:         0x%02X\n", 0x41 + isvioPort, *(BYTE*)&pPortInfo[isvioPort].portSts);
			printf("        PRESENT            [%c]\n", pPortInfo[isvioPort].portSts.fPresent ? 'Y':'N');
			printf("        DOUBLE_WIDE        [%c]\n", pPortInfo[isvioPort].portSts.fDW ? 'Y':'N');
			printf("        5V0_WITHIN_LIMIT   [%c]\n", pPortInfo[isvioPort].portSts.f5v0InLimit ? 'Y':'N');
			printf("        3V3_WITHIN_LIMIT   [%c]\n", pPortInfo[isvioPort].portSts.f3v3InLimit ? 'Y':'N');
			printf("        VIO_WITHIN_LIMIT   [%c]\n", pPortInfo[isvioPort].portSts.fVioInLimit ? 'Y':'N');
			printf("        ALLOW_VIO_ENABLE   [%c]\n", pPortInfo[isvioPort].portSts.fAllowVioEnable ? 'Y':'N');
		}
		/* Read and display the VIO voltage setting for this port.
		*/
		if ( ! PmcuI2cRead(fdI2c, regaddrVadjAVoltage + (offsetVadjReg*pPortInfo[isvioPort].groupVio), (BYTE*)&pPortInfo[isvioPort].voltage, 2, NULL) ) {
			printf("ERROR: failed to read VADJ_%c_VOLTAGE register\n", 0x41 + pPortInfo[isvioPort].groupVio);
			goto lErrorExit;
		}
		if(dpmutilfVerbose){
			if ( vadjsts.fsEn & (1 << pPortInfo[isvioPort].groupVio) ) {
				printf("    PORT_%c_VIO_ENABLE:     [Y]\n", 0x41 + isvioPort);
				printf("    PORT_%c_VOLTAGE:        %d mV\n", 0x41 + isvioPort, pPortInfo[isvioPort].voltage * 10);
			}
			else {
				printf("    PORT_%c_VIO_ENABLE:     [N]\n", 0x41 + isvioPort);
				printf("    PORT_%c_VOLTAGE:        0 mV\n", 0x41 + isvioPort);
			}

			if (( pPortInfo[isvioPort].portSts.fPresent )  && ( IsSyzygyPort(pPortInfo[isvioPort].portType) )) {

				if ( ! SyzygyReadStdFwRegisters(fdI2c, pPortInfo[isvioPort].i2cAddr, &szgstdfwRegs) ) {
					printf("ERROR: failed to retrieve SYZYGY standard fw registers from 0x%02X\n", pPortInfo[isvioPort].i2cAddr);
					goto lErrorExit;
				}

				if ( ! SyzygyReadDNAHeader(fdI2c, pPortInfo[isvioPort].i2cAddr, &szgdnaHeader, setCrcCheck ? crcCheck : fTrue) ) {
					printf("ERROR: failed to retrieve SYZYGY DNA header from 0x%02X\n", pPortInfo[isvioPort].i2cAddr);
					goto lErrorExit;
				}

				if ( ! SyzygyReadDNAStrings(fdI2c, pPortInfo[isvioPort].i2cAddr, &szgdnaHeader, &szgdnaStrings) ) {
					printf("Error: failed to retrieve SYZYGY DNA strings from 0x%02X\n", pPortInfo[isvioPort].i2cAddr);
					SyzygyFreeDNAStrings(&szgdnaStrings);
					goto lErrorExit;
				}

				printf("    Manufacturer Name:     %s\n", szgdnaStrings.szManufacturerName);
				printf("    Product Name:          %s\n", szgdnaStrings.szProductName);
				printf("    Product Model:         %s\n", szgdnaStrings.szProductModel);
				printf("    Product Version:       %s\n", szgdnaStrings.szProductVersion);
				printf("    Serial Number:         %s\n", szgdnaStrings.szSerialNumber);
				printf("    Firmware Version:      %d.%d\n", szgstdfwRegs.fwverMjr, szgstdfwRegs.fwverMin);
				printf("    DNA Version:           %d.%d\n", szgstdfwRegs.dnaverMjr, szgstdfwRegs.dnaverMin);
				printf("    Maximum 5V Load:       %d mA\n", szgdnaHeader.crntRequired5v0);
				printf("    Maximum 3.3V Load:     %d mA\n", szgdnaHeader.crntRequired3v3);
				printf("    Maximum VIO Load:      %d mA\n", szgdnaHeader.crntRequiredVio);
				printf("    Voltage Range 1:       %d to %d mV\n", szgdnaHeader.vltgRange1Min * 10, szgdnaHeader.vltgRange1Max * 10);
				printf("    Voltage Range 2:       %d to %d mV\n", szgdnaHeader.vltgRange2Min * 10, szgdnaHeader.vltgRange2Max * 10);
				printf("    Voltage Range 3:       %d to %d mV\n", szgdnaHeader.vltgRange3Min * 10, szgdnaHeader.vltgRange3Max * 10);
				printf("    Voltage Range 4:       %d to %d mV\n", szgdnaHeader.vltgRange4Min * 10, szgdnaHeader.vltgRange4Max * 10);
				printf("    Attribute Flags:       0x%04X\n", szgdnaHeader.fsAttributes);
				printf("        IS_LVDS            [%c]\n", szgdnaHeader.fsAttributes & sattrLvds ? 'Y' : 'N');
				printf("        IS_DOUBLEWIDE      [%c]\n", szgdnaHeader.fsAttributes & sattrDoubleWide ? 'Y' : 'N');
				printf("        IS_TXR4            [%c]\n", szgdnaHeader.fsAttributes & sattrTxr4 ? 'Y' : 'N');

				if ( 0 == strncmp(szgdnaStrings.szManufacturerName, "Digilent", strlen("Digilent")) ) {
					if ( ! SyzygyI2cRead(fdI2c, pPortInfo[isvioPort].i2cAddr, addrPdid, (BYTE*)&pdid, 4, NULL) ) {
						printf("Error: failed to read PDID from 0x%02X\n", pPortInfo[isvioPort].i2cAddr);
						goto lErrorExit;
					}
					printf("    PDID:                  0x%08X\n", (unsigned int)pdid);

					/* Output additional information (if available) based on the
					** product number of the installed module.
					*/
					switch ( ProductFromPdid(pdid) ) {
						case prodZmodADC:
							if ( ! FDisplayZmodADCCal(fdI2c, pPortInfo[isvioPort].i2cAddr) ) {
								goto lErrorExit;
							}
							break;

						case prodZmodDAC:
							if ( ! FDisplayZmodDACCal(fdI2c, pPortInfo[isvioPort].i2cAddr) ) {
								goto lErrorExit;
							}
							break;


						default:
							break;
					}
				}

				SyzygyFreeDNAStrings(&szgdnaStrings);
			}
		}
	}

#if defined(__linux__)
	/* Close the I2C controller file descriptor.
	*/
	close(fdI2c);
#endif
	return fTrue;

lErrorExit:
#if defined(__linux__)
	if ( 0 <= fdI2c ) {
		close(fdI2c);
	}
#endif
	SyzygyFreeDNAStrings(&szgdnaStrings);

	return fFalse;
}

/* ------------------------------------------------------------ */
/***    dpmutilFSetPlatformConfig
**
**  Parameters:
**      pDevInfo		- Pointer to a dpmutilDevInfo_t object to store data
**      setEnforce5v0	- flag to set enforce5v0 setting
**      enforce5v0		- if flag is true, value to set enforce5v0 to
**      setEnforce3v3	- flag to set enforce3v3 setting
**      enforce3v3		- if flag is true, value to set enforce3v3 to
**      setEnforceVio	- flag to set enforceVio setting
**      enforceVio		- if flag is true, value to set enforceVio to
**      setCrcCheck		- flag to set CrcCheck setting
**      crcCheck		- if flag is true, value to set checkcrc to
**
**  Return Values:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**
**  Description:
**      Modify one or more field of the Platform MCU (PMCU) Platform
**      configuration Register. This function uses the I2C bus to
**      retrieve the contents of the PMCU's Platform Configuration
**      Register, modifies the specified field(s) of the register,
**      and then writes the new settings to the register. Settings
**      that may be modified include enforcing the 5V0 current limit,
**      enforcing the 3V3 current limit, enforcing the VOI current limit,
**      and performing CRC checks of SYZYGY headers. Please note that the
**      the Platform Configuration is stored in the PMCU's EEPROM and is
**      only read during firmware initialization. Therefore any changes
**      made to the Platform Configuration Register will not take effect
**      until the next time the PMCU is reset.
**
**      The "enforce5v0 <fTrue,fFalse>" parameter can be used to enable or disable
**      enforcement of 5V0 current limits. If enforcement is enabled and
**      the sum of the current requested by all associated SmartVIO pods
**      exceeds the total current that the supply can provide then the
**      VIO supply associated with the SmartVIO port will not be enabled.
**
**      The "enforce3v3 <fTrue,fFalse>" parameter can be used to enable or disable
**      enforcement of 3V3 current limits. If enforcement is enabled and
**      the sum of the current requested by all associated SmartVIO pods
**      exceeds the total current that the supply can provide then the
**      VIO supply associated with the SmartVIO port will not be enabled.
**
**      The "enforcevio <fTrue,fFalse>" parameter can be used to enable or disable
**      enforcement of VIO current limits. If enforcement is enabled and
**      the sum of the current requested by all associated SmartVIO pods
**      exceeds the total current that the supply can provide then the
**      VIO supply associated with the SmartVIO port will not be enabled.
**
**      The "checkcrc <fTrue,fFalse>" parameter can be used to enable or disable
**      SYZYGY header DNA checks. If CRC checks are enabled and the CRC
**      computed does not match then the VIO supply associated with the
**      SmartVIO port will not be enabled.
*/
BOOL
dpmutilFSetPlatformConfig(dpmutildevInfo_t* pDevInfo, BOOL setEnforce5v0, BOOL enforce5v0, BOOL setEnforce3v3, BOOL enforce3v3, BOOL setEnforceVio, BOOL enforceVio, BOOL setCrcCheck, BOOL crcCheck) {

	int					fdI2c;
	WORD				wTemp;
	PLATFORM_CONFIG*	ppcfg;
#if defined(__linux__)
	struct timespec		tsWait;
#endif
	fdI2c = -1;

	/* Make sure the user passed in a parameter specifying the value to
	** set for one or more of the bits in the platform configuration
	** register, otherwise there is nothing to do.
	*/
	if (( ! setEnforce5v0 ) &&
		( ! setEnforce3v3) &&
		( ! setEnforceVio) &&
		( ! setCrcCheck )) {
		printf("ERROR: you must specify one or more field to set in the\n");
		printf("platform configuration register. Use the \"-enforce5v0\",\n");
		printf("\"-enforce3v3\", \"-enforcevio\", and \"-checkcrc\" options\n");
		printf("to specify the field to set.\n");
		goto lErrorExit;
	}

#if defined(__linux__)
	fdI2c = I2CHALOpenI2cController();
	if ( 0 > fdI2c ) {
		printf("ERROR: failed to open file descriptor for I2C device\n");
		goto lErrorExit;
	}
#else
	if(!I2CHALInit(0)){
		printf("ERROR: failed to initialize I2C device\n");
		goto lErrorExit;
	}
#endif

	/* Read and display the platform configuration register.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrPlatformConfig, (BYTE*)(&pDevInfo->platcfg), 2, NULL) ) {
		printf("ERROR: failed to read PLATFORM_CONFIGURATION register\n");
		goto lErrorExit;
	}

	if(dpmutilfVerbose){
		printf("Existing PLATFORM_CONFIGURATION: 0x%04X\n", *(WORD*)&(pDevInfo->platcfg));
		printf("    ENFORCE_5V0_CURRENT_LIMIT    [%c]\n", pDevInfo->platcfg.fEnforce5v0CurLimit ? 'Y':'N');
		printf("    ENFORCE_3V3_CURRENT_LIMIT    [%c]\n", pDevInfo->platcfg.fEnforce3v3CurLimit ? 'Y':'N');
		printf("    ENFORCE_VIO_CURRENT_LIMIT    [%c]\n", pDevInfo->platcfg.fEnforceVioCurLimit ? 'Y':'N');
		printf("    PERFORM_SYZYGY_CRC_CHECK     [%c]\n", pDevInfo->platcfg.fPerformCrcCheck ? 'Y':'N');
	}

	/* Update the fields of the platform configuration register based on
	** the arguments specified by the user.
	*/
	if ( setEnforce5v0 ) {
		pDevInfo->platcfg.fEnforce5v0CurLimit = ( enforce5v0 ) ? 1 : 0;
	}
	if ( setEnforce3v3 ) {
		pDevInfo->platcfg.fEnforce3v3CurLimit = ( enforce3v3 ) ? 1 : 0;
	}
	if ( setEnforceVio ) {
		pDevInfo->platcfg.fEnforceVioCurLimit = ( enforceVio ) ? 1 : 0;
	}
	if ( setCrcCheck ) {
		pDevInfo->platcfg.fPerformCrcCheck = ( crcCheck ) ? 1 : 0;
	}

	/* Attempt to write the new configuration to the PMCU.
	*/
	if ( ! PmcuI2cWrite(fdI2c, regaddrPlatformConfig, (BYTE*)(&pDevInfo->platcfg), 2, NULL) ) {
		printf("ERROR: failed to write PLATFORM_CONFIGURATION register\n");
		goto lErrorExit;
	}

	/* Give the platform MCU time to write the EEPROM. The typical write
	** time is 3.3ms per byte and writing the platform configuration
	** register results in a total of 10 bytes being written (2 bytes
	** of data and 4 bytes of sequence numbers). Therefore I suggest
	** waiting at least 50 milliseconds before attempting to communicate
	** with the PMCU.
	*/
#if defined(__linux__)
	tsWait.tv_sec = 0;
	tsWait.tv_nsec = 50000000;
	nanosleep(&tsWait, NULL);
#else
	usleep(50000);
#endif

	/* Read and display the platform configuration register.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrPlatformConfig, (BYTE*)(&wTemp), 2, NULL) ) {
		printf("ERROR: failed to read PLATFORM_CONFIGURATION register 2\n");
		goto lErrorExit;
	}
	ppcfg = (PLATFORM_CONFIG*)(&wTemp);

	/* Display the new configuration.
	*/
	if(dpmutilfVerbose){
		printf("\nNew PLATFORM_CONFIGURATION:      0x%04X\n", wTemp);
		printf("    ENFORCE_5V0_CURRENT_LIMIT    [%c]\n", ppcfg->fEnforce5v0CurLimit ? 'Y':'N');
		printf("    ENFORCE_3V3_CURRENT_LIMIT    [%c]\n", ppcfg->fEnforce3v3CurLimit ? 'Y':'N');
		printf("    ENFORCE_VIO_CURRENT_LIMIT    [%c]\n", ppcfg->fEnforceVioCurLimit ? 'Y':'N');
		printf("    PERFORM_SYZYGY_CRC_CHECK     [%c]\n", ppcfg->fPerformCrcCheck ? 'Y':'N');
	}

	if ( *(WORD*)&pDevInfo->platcfg != wTemp ) {
		printf("ERROR: new platform configuration (0x%04X) does\n", *(WORD*)&pDevInfo->platcfg);
		printf("not match specified configuration (0x%04X)\n", wTemp);
		goto lErrorExit;
	}

#if defined(__linux__)
	/* Close the I2C controller file descriptor.
	*/
	close(fdI2c);
#endif
	return fTrue;

lErrorExit:
#if defined(__linux__)
	if ( 0 <= fdI2c ) {
		close(fdI2c);
	}
#endif
	return fFalse;
}

/* ------------------------------------------------------------ */
/***    dpmutilFSetVioConfig
**
**  Parameters:
**      chanid			- channel id to set
**      setEnable		- flag to set enabled setting
**      enable			- if flag is true, value to set enable to
**      setOverride		- flag to set override setting
**      override		- if flag is true, value to set override to
**      setVoltage		- flag to set voltage setting
**      voltage			- if flag is true, value to set voltage to
**
**  Return Values:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**
**  Description:
**      Modify one or more field of the Platform MCU (PMCU) VADJ_n_OVERRIDE
**      register. The VADJ_n_OVERRIDE register can be used to override the
**      state of a specific VIO supply. This includes enabling or disabling
**      the supply, as well as setting the output voltage. When a
**      VADJ_n_OVERRIDE register is written the PMCU will check to make sure
**      that the specified settings do not conflict with the requirements of
**      any SmartVIO port associated with the specified supply. If there
**      aren't any conflicts then the specified settings will take place
**      immediately. However, if there is a conflict then the changes to the
**      VADJ_n_OVERRIDE register, and the associated power supply, will be
**      restricted in order to meet the requirements of all associated
**      SmartVIO ports.
**
**      The "chanid <0...7>" parameter must be used to specify
**      the channel ID of the VIO supply.
**
**      The "override <fTrue,fFalse>" parameter can be used to enable or disable
**      overriding the VADJ supply configuration. If override is enabled
**      then the associated VIO supply will be configured based on the
**      enable and voltage fields of the VADJ_n_OVERRIDE register.
**
**      The "enable <fTrue,fFalse>" parameter can be used to enable or disable the
**      associated VIO supply. This setting has no impact when the override
**      field of the VADJ_n_OVERRIDE register is cleared.
**
**      The "voltage <millivolts>" parameter can be used to specify the
**      voltage of the associated VIO supply. This setting has no impact
**      when the override field of the VADJ_n_OVERRIDE register is cleared.
*/
BOOL
dpmutilFSetVioConfig(int chanid, BOOL setEnable, BOOL enable, BOOL setOverride, BOOL override, BOOL setVoltage, WORD voltage) {

	int				fdI2c;
	WORD			wTemp;
	BYTE			cvadj;
	VADJ_STATUS		vadjsts;
	VADJ_OVERRIDE	vadjow;
	VADJ_OVERRIDE	vadjow2;
#if defined(__linux__)
	struct timespec		tsWait;
#endif

	fdI2c = -1;

	/* Make sure the user specified the channel ID.
	*/
	if ( chanid < 0 ) {
		printf("ERROR: you must specify a channel identifier using the \"-chanid\" option\n");
		goto lErrorExit;
	}

	/* Make sure the user passed in a parameter specifying the value to
	** set for one or more field of the VADJ_n_OVERRIDE register, otherwise
	** there is nothing to do.
	*/
	if (( ! setEnable ) && ( ! setOverride ) && ( ! setVoltage )) {
		printf("ERROR: you must specify one or more field to set. Use\n");
		printf("the \"-override\", \"-enable\", and \"-voltage\" options to \n");
		printf("specify the field to set.\n");
		goto lErrorExit;
	}

#if defined(__linux__)
	fdI2c = I2CHALOpenI2cController();
	if ( 0 > fdI2c ) {
		printf("ERROR: failed to open file descriptor for I2C device\n");
		goto lErrorExit;
	}
#else
	if(!I2CHALInit(0)){
		printf("ERROR: failed to initialize I2C device\n");
		goto lErrorExit;
	}
#endif

	/* Determine how many VADJ supplies there are.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrVadjGroupCount, &cvadj, 1, NULL) ) {
		printf("ERROR: failed to read VADJ_GROUP_COUNT register\n");
		goto lErrorExit;
	}

	/* Make sure the specified channel is supported by this device.
	*/
	if ( chanid >= cvadj ) {
		printf("ERROR: device has %d VIO supplies. Channel %d is\n", cvadj, chanid);
		printf("not supported by this device\n");
		goto lErrorExit;
	}

	/* Read and display the override register for the supply.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrVadjAOverride + (offsetVadjReg*chanid), (BYTE*)&vadjow, 2, NULL) ) {
		printf("ERROR: failed to read VADJ_%c_OVERRIDE register\n", 0x41 + chanid);
		goto lErrorExit;
	}
	if(dpmutilfVerbose){
		printf("Existing VADJ_%c_OVERRIDE:    0x%04X\n", 0x41 + chanid, vadjow.fs);
		printf("    ENABLE_OVERRIDE          [%c]\n", vadjow.fOverride ? 'Y' : 'N');
		printf("    ENABLE_SUPPLY            [%c]\n", vadjow.fEnable ? 'Y' : 'N');
		printf("    VOLTAGE_TO_SET           %d mV\n", vadjow.vltgSet * 10);
	}

	/* Read and display the voltage register for the supply.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrVadjAVoltage + (offsetVadjReg*chanid), (BYTE*)&wTemp, 2, NULL) ) {
		printf("ERROR: failed to read VADJ_%c_VOLTAGE register\n", 0x41 + chanid);
		goto lErrorExit;
	}
	if(dpmutilfVerbose)printf("Existing VADJ_%c_VOLTAGE:     %d mV\n", 0x41 + chanid, wTemp * 10);

	/* Get and display the status for this supply.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrVadjStatus, (BYTE*)&vadjsts, 2, NULL) ) {
		printf("ERROR: failed to read VADJ_STATUS register\n");
		goto lErrorExit;
	}
	if(dpmutilfVerbose){
		printf("VADJ_%c_ENABLED:              [%c]\n", 0x41 + chanid, (vadjsts.fsEn & (1<<chanid)) ? 'Y' : 'N');
		printf("VADJ_%c_POWER_GOOD:           [%c]\n", 0x41 + chanid, (vadjsts.fsPgood & (1<<chanid)) ? 'Y' : 'N');
	}

	/* Update the fields of the override register to reflect the settings
	** provided by the user.
	*/
	if ( setVoltage ) {
		vadjow.vltgSet = voltage / 10;
	}
	if ( setEnable ) {
		vadjow.fEnable = enable ? 1 : 0;
	}
	if ( setOverride ) {
		vadjow.fOverride = override ? 1 : 0 ;
	}

	/* Dispaly the new override settings that we intend to apply.
	*/
	if(dpmutilfVerbose){
		printf("\nNew VADJ_%c_OVERRIDE:         0x%04X\n", 0x41 + chanid, vadjow.fs);
		printf("    ENABLE_OVERRIDE          [%c]\n", vadjow.fOverride ? 'Y' : 'N');
		printf("    ENABLE_SUPPLY            [%c]\n", vadjow.fEnable ? 'Y' : 'N');
		printf("    VOLTAGE_TO_SET           %d mV\n", vadjow.vltgSet * 10);
	}

	/* Attempt to write the new configuration to the PMCU.
	*/
	if ( ! PmcuI2cWrite(fdI2c, regaddrVadjAOverride + (offsetVadjReg*chanid), (BYTE*)(&vadjow), 2, NULL) ) {
		printf("ERROR: failed to write VADJ_%c_OVERRIDE register\n", 0x41 + chanid);
		goto lErrorExit;
	}

	/* Give the platform MCU time to process the changes to the
	** VADJ_n_OVERRIDE register before attempting to read the
	** override register or associated voltage register.
	*/
#if defined(__linux__)
	tsWait.tv_sec = 0;
	tsWait.tv_nsec = 50000000;
	nanosleep(&tsWait, NULL);
#else
	usleep(50000);
#endif

	/* Read and display the new override register settings.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrVadjAOverride + (offsetVadjReg*chanid), (BYTE*)&vadjow2, 2, NULL) ) {
		printf("ERROR: failed to read VADJ_%c_OVERRIDE register\n", 0x41 + chanid);
		goto lErrorExit;
	}
	if(dpmutilfVerbose){
		printf("\nActual VADJ_%c_OVERRIDE:      0x%04X\n", 0x41 + chanid, vadjow2.fs);
		printf("    ENABLE_OVERRIDE          [%c]\n", vadjow2.fOverride ? 'Y' : 'N');
		printf("    ENABLE_SUPPLY            [%c]\n", vadjow2.fEnable ? 'Y' : 'N');
		printf("    VOLTAGE_TO_SET           %d mV\n", vadjow2.vltgSet * 10);
	}

	/* Read and display the voltage register for the supply.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrVadjAVoltage + (offsetVadjReg*chanid), (BYTE*)&wTemp, 2, NULL) ) {
		printf("ERROR: failed to read VADJ_%c_VOLTAGE register\n", 0x41 + chanid);
		goto lErrorExit;
	}
	if(dpmutilfVerbose)printf("Actual VADJ_%c_VOLTAGE:       %d mV\n", 0x41 + chanid, wTemp * 10);

	/* Get and display the status for this supply.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrVadjStatus, (BYTE*)&vadjsts, 2, NULL) ) {
		printf("ERROR: failed to read VADJ_STATUS register\n");
		goto lErrorExit;
	}
	if(dpmutilfVerbose){
		printf("VADJ_%c_ENABLED:              [%c]\n", 0x41 + chanid, (vadjsts.fsEn & (1<<chanid)) ? 'Y' : 'N');
		printf("VADJ_%c_POWER_GOOD:           [%c]\n", 0x41 + chanid, (vadjsts.fsPgood & (1<<chanid)) ? 'Y' : 'N');
	}

	if ( vadjow.fs != vadjow2.fs ) {
		printf("ERROR: new VADJ_%c_OVERRIDE configuration (0x%04X) does\n", 0x41 + chanid, vadjow2.fs);
		printf("not match specified configuration (0x%04X)\n", vadjow.fs);
		goto lErrorExit;
	}

#if defined(__linux__)
	/* Close the I2C controller file descriptor.
	*/
	close(fdI2c);
#endif
	return fTrue;

lErrorExit:
#if defined(__linux__)
	if ( 0 <= fdI2c ) {
		close(fdI2c);
	}
#endif

	return fFalse;
}

/* ------------------------------------------------------------ */
/***    dpmutilFSetFanConfig
**
**  Parameters:
**      fanid			- fan id to configure
**      setEnable		- flag to set enabled setting
**      enable			- if flag is true, value to set enable to
**      setSpeed		- flag to set speed setting
**      speed			- if flag is true, value to set speed to
**      setProbe		- flag to set probe setting
**      probe			- if flag is true, value to set probe to
**
**  Return Values:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**
**  Description:
**      Modify one or more field of the Platform MCU (PMCU)
**      FAN_n_CONFIGURATION register. The FAN_n_CONFIGURATION register is
**      used to specify the settings of the associated fan. This may include
**      the enable state of the fan, the fan's speed, and the associated
**      temperature probe. Please note that not all fan ports support
**      enable/disable, fixed speed control, or automatic speed control
**      (temperature based). Changes to a FAN_n_CONFIGURATION register
**      will be restricted to the be within the supported capabilities of
**      the port and take effect immediately after the register is written.
**      Additionally, the FAN configuration is written to EEPROM and will
**      restored each time the PMCU is reset or power cycled.
**
**      The "fanid <0...3>" parameter must be used to specify ID of the
**      fan configuration to be modified.
**
**      The "enable <fTrue,fFalse>" parameter can be used to enable or disable the
**      associated fan.
**
**      The "speed <0...3>" parameter can be used to specify the speed of
**      the associated fan. Please note that not all fans support this
**      functionality and some ports that do support this functionality
**      may not support automatic fan speed control.
**      0 - minimum
**      1 - medium
**      2 - maximum
**      3 - auto
**
**      The "-probe <0...4>" parameter can be used to specify the
**      temperature probe associated with a fan if that fan supports automatic
**      speed control.
**      0 - none
**      1...4 - probe[1...4]
*/
BOOL
dpmutilFSetFanConfig(int fanid, BOOL setEnable, BOOL enable, BOOL setSpeed, BYTE speed, BOOL setProbe, BYTE probe) {

	int					fdI2c;
	BYTE				cfan;
	FAN_CAPABILITIES	fcap;
	FAN_CONFIGURATION	fcfg;
	FAN_CONFIGURATION	fcfg2;
#if defined(__linux__)
	struct timespec		tsWait;
#endif

	fdI2c = -1;

	/* Make sure the user passed in a parameter specifying the value to
	** set for one or more fields of the FAN_n_CONFIGURATION register.
	*/
	if (( ! setEnable ) && ( ! setSpeed ) && ( ! setProbe )) {
		printf("ERROR: you must specify one or more field to set. Use\n");
		printf("the \"-enable\", \"-speed\", and \"-probe\" options to \n");
		printf("specify the field to set.\n");
		goto lErrorExit;
	}

#if defined(__linux__)
	fdI2c = I2CHALOpenI2cController();
	if ( 0 > fdI2c ) {
		printf("ERROR: failed to open file descriptor for I2C device\n");
		goto lErrorExit;
	}
#else
	if(!I2CHALInit(0)){
		printf("ERROR: failed to initialize I2C device\n");
		goto lErrorExit;
	}
#endif

	/* Determine how many fans the device supports.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrFanCount, &cfan, 1, NULL) ) {
		printf("ERROR: failed to read FAN_COUNT register\n");
		goto lErrorExit;
	}

	/* Make sure the specified fan is supported by this device.
	*/
	if ( fanid >= cfan ) {
		printf("ERROR: device supports %d fans. Fan %d is\n", cfan, fanid + 1);
		printf("not supported by this device\n");
		goto lErrorExit;
	}

	/* Read and display this fan's capabilities.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrFan1Capabilities + (offsetFanReg*fanid), (BYTE*)&fcap, 1, NULL) ) {
		printf("ERROR: failed to read FAN_%d_CAPABILITIES register\n", fanid+1);
		goto lErrorExit;
	}

	if(dpmutilfVerbose){
		printf("FAN_%d_CAPABILITIES:              0x%02X\n", fanid + 1, fcap.fs);
		printf("    ENABLE_AND_DISABLE           [%c]\n", fcap.fcapEnable ? 'Y' : 'N');
		printf("    SET_FIXED_SPEED              [%c]\n", fcap.fcapSetSpeed ? 'Y' : 'N');
		printf("    AUTO_SPEED_CONTROL           [%c]\n", fcap.fcapAutoSpeed ? 'Y' : 'N');
		printf("    MEASURE_RPM                  [%c]\n", fcap.fcapMeasureRpm ? 'Y' : 'N');
	}

	/* Read and display this fan's configuration.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrFan1Config + (offsetFanReg*fanid), (BYTE*)&fcfg, 1, NULL) ) {
		printf("ERROR: failed to read FAN_%d_CONFIGURATION register\n", fanid+1);
		goto lErrorExit;
	}
	if(dpmutilfVerbose){
		printf("\nExisting FAN_%d_CONFIGURATION:    0x%02X\n", fanid + 1, fcfg.fs);
		printf("    ENABLE                       [%c]\n", fcfg.fEnable ? 'Y' : 'N');
		printf("    SPEED                        ");
		switch ( fcfg.fspeed ) {
			case fancfgMinimumSpeed:
				printf("MINIMUM\n");
				break;
			case fancfgMediumSpeed:
				printf("MEDIUM\n");
				break;
			case fancfgMaximumSpeed:
				printf("MAXIMUM\n");
				break;
			case fancfgAutoSpeed:
				printf("AUTOMATIC\n");
				break;
			default:
				printf("UNKNOWN\n");
				break;
		}
		printf("    TEMPERATURE_SOURCE           ");
		switch ( fcfg.tempsrc ) {
			case fancfgTempProbeNone:
				printf("NONE\n");
				break;
			case fancfgTempProbe1:
				printf("TEMP_PROBE_1\n");
				break;
			case fancfgTempProbe2:
				printf("TEMP_PROBE_2\n");
				break;
			case fancfgTempProbe3:
				printf("TEMP_PROBE_3\n");
				break;
			case fancfgTempProbe4:
				printf("TEMP_PROBE_4\n");
				break;
			default:
				printf("UNKNOWN\n");
				break;
		}
	}

	/* Update the configuration based on the parameters provided by the user.
	*/
	if ( setEnable ) {
		fcfg.fEnable = enable ? 1 : 0;
	}
	if ( setSpeed ) {
		fcfg.fspeed = speed;
	}
	if ( setProbe ) {
		fcfg.tempsrc = probe;
	}

	if(dpmutilfVerbose){
		printf("\nNew FAN_%d_CONFIGURATION:         0x%02X\n", fanid + 1, fcfg.fs);
		printf("    ENABLE                       [%c]\n", fcfg.fEnable ? 'Y' : 'N');
		printf("    SPEED                        ");
		switch ( fcfg.fspeed ) {
			case fancfgMinimumSpeed:
				printf("MINIMUM\n");
				break;
			case fancfgMediumSpeed:
				printf("MEDIUM\n");
				break;
			case fancfgMaximumSpeed:
				printf("MAXIMUM\n");
				break;
			case fancfgAutoSpeed:
				printf("AUTOMATIC\n");
				break;
			default:
				printf("UNKNOWN\n");
				break;
		}
		printf("    TEMPERATURE_SOURCE           ");
		switch ( fcfg.tempsrc ) {
			case fancfgTempProbeNone:
				printf("NONE\n");
				break;
			case fancfgTempProbe1:
				printf("TEMP_PROBE_1\n");
				break;
			case fancfgTempProbe2:
				printf("TEMP_PROBE_2\n");
				break;
			case fancfgTempProbe3:
				printf("TEMP_PROBE_3\n");
				break;
			case fancfgTempProbe4:
				printf("TEMP_PROBE_4\n");
				break;
			default:
				printf("UNKNOWN\n");
				break;
		}
	}

	/* Send the new fan configuration to the PMCU.
	*/
	if ( ! PmcuI2cWrite(fdI2c, regaddrFan1Config + (offsetFanReg*fanid), (BYTE*)&fcfg, 1, NULL) ) {
		printf("ERROR: failed to write FAN_%d_CONFIGURATION register\n", fanid + 1);
		goto lErrorExit;
	}

	/* Give the platform MCU time to write the EEPROM. The typical write
	** time is 3.3ms per byte and writing the platform configuration
	** register results in a total of 10 bytes being written (2 bytes
	** of data and 4 bytes of sequence numbers). Therefore I suggest
	** waiting at least 50 milliseconds before attempting to communicate
	** with the PMCU.
	*/
#if defined(__linux__)
	tsWait.tv_sec = 0;
	tsWait.tv_nsec = 50000000;
	nanosleep(&tsWait, NULL);
#else
	usleep(50000);
#endif

	/* Read and display the fan configuration that was actually set.
	*/
	if ( ! PmcuI2cRead(fdI2c, regaddrFan1Config + (offsetFanReg*fanid), (BYTE*)&fcfg2, 1, NULL) ) {
		printf("ERROR: failed to read FAN_%d_CONFIGURATION register\n", fanid+1);
		goto lErrorExit;
	}
	if(dpmutilfVerbose){
		printf("\nActual FAN_%d_CONFIGURATION:      0x%02X\n", fanid + 1, fcfg2.fs);
		printf("    ENABLE                       [%c]\n", fcfg2.fEnable ? 'Y' : 'N');
		printf("    SPEED                        ");
		switch ( fcfg2.fspeed ) {
			case fancfgMinimumSpeed:
				printf("MINIMUM\n");
				break;
			case fancfgMediumSpeed:
				printf("MEDIUM\n");
				break;
			case fancfgMaximumSpeed:
				printf("MAXIMUM\n");
				break;
			case fancfgAutoSpeed:
				printf("AUTOMATIC\n");
				break;
			default:
				printf("UNKNOWN\n");
				break;
		}
		printf("    TEMPERATURE_SOURCE           ");
		switch ( fcfg2.tempsrc ) {
			case fancfgTempProbeNone:
				printf("NONE\n");
				break;
			case fancfgTempProbe1:
				printf("TEMP_PROBE_1\n");
				break;
			case fancfgTempProbe2:
				printf("TEMP_PROBE_2\n");
				break;
			case fancfgTempProbe3:
				printf("TEMP_PROBE_3\n");
				break;
			case fancfgTempProbe4:
				printf("TEMP_PROBE_4\n");
				break;
			default:
				printf("UNKNOWN\n");
				break;
		}
	}

	if ( fcfg.fs != fcfg2.fs ) {
		printf("ERROR: new FAN_%d_CONFIGURATION (0x%02X) does\n", fanid + 1, fcfg2.fs);
		printf("not match specified configuration (0x%02X)\n", fcfg.fs);
		goto lErrorExit;
	}

#if defined(__linux__)
	/* Close the I2C controller file descriptor.
	*/
	close(fdI2c);
#endif
	return fTrue;

lErrorExit:
#if defined(__linux__)
	if ( 0 <= fdI2c ) {
		close(fdI2c);
	}
#endif

	return fFalse;
}

/* ------------------------------------------------------------ */
/***    dpmutilFResetPMCU
**
**  Parameters:
**      none
**
**  Return Values:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**
**  Description:
**      This function uses the I2C bus to write a positive value to the
**      software reset register of the Platform MCU (PMCU), which causes
**      the process to perform a software reset.
*/
BOOL
dpmutilFResetPMCU() {

	int		fdI2c;
	BYTE	bTemp;

	fdI2c = -1;

#if defined(__linux__)
	fdI2c = I2CHALOpenI2cController();
	if ( 0 > fdI2c ) {
		printf("ERROR: failed to open file descriptor for I2C device\n");
		goto lErrorExit;
	}
#else
	if(!I2CHALInit(0)){
		printf("ERROR: failed to initialize I2C device\n");
		goto lErrorExit;
	}
#endif

	/* Send the reset command to the Platform MCU (PMCU). A non-zero value
	** must be sent to the reset address in order for the PMCU to perform
	** a software reset. Please note that upon reset the PMCU will initially
	** configure its I2C controller for master mode and use it to enumerate
	** any SmartVIO devices that are plugged into the onboard SmartVIO ports
	** and turn on the applicable VADJ supplies. The PMCU does not reconfigure
	** itself as an I2C slave until its done enabling the VADJ supplies.
	** Therefore it is recommended that you avoid using the I2C bus for at
	** least 1 second after sending the reset command.
	*/
	bTemp = 1;
	if ( ! PmcuI2cWrite(fdI2c, regaddrSoftwareReset, &bTemp, 1, NULL) ) {
		printf("ERROR: failed to write SOFTWARE_RESET register\n");
		goto lErrorExit;
	}

	if(dpmutilfVerbose)printf("Successfully sent reset command to Platform MCU!\n");

#if defined(__linux__)
	/* Close the I2C controller file descriptor.
	*/
	close(fdI2c);
#endif
	return fTrue;

lErrorExit:
#if defined(__linux__)
	if ( 0 <= fdI2c ) {
		close(fdI2c);
	}
#endif

	return fFalse;
}
