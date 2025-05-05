/************************************************************************/
/*                                                                      */
/*  dpmutil.h  --  Digilent Platform Management Utility main program    */
/*                                                                      */
/************************************************************************/
/*  Author: Michael T. Alexander, Thomas Kappenman                      */
/*  Copyright 2019 Digilent, Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  The Digilent Platform Management Utility (dpmutil) is a program     */
/*  that provides an API for discovering Zmods (SYZYGY Pods) attached   */
/*  to a Digilent platform board, acquiring information about them,     */
/*  acquiring information about the Digilent platform board, and setting*/
/*  certain settings pertaining to the configuration of the Digilent    */
/*  platform board.                                                     */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  08/21/2019(MichaelA): Created                                       */
/*	10/25/2019(MichaelA): modified FEnum to retrieve and display the    */
/*		PDID for SYZYGY Pods manufactured by Digilent                   */
/*	10/28/2019(MichaelA): modified OpenI2cController to search the      */
/*		"/sys/bus/i2c/devices" directory for a device whose             */
/*		"device-name" is "pmcu-i2c" and if found open that device. If   */
/*		no such device is found then we assume "/dev/i2c-0" is the      */
/*		device entry corresponding to the I2C controller with the PMCU  */
/*		I2C bus                                                         */
/*	01/03/2020(MichaelA): modified FEnum to add support for displaying  */
/*		the 5V0, 3V3, and VIO voltages read by the pMCU on the ZmodLOOP */
/*	01/03/2020(MichaelA): added FWriteDNA function to add the ability   */
/*		to write binary data to the DNA section of the pMCU flash       */
/*	01/06/2020(MichaelA): modified FWriteDNA to add support for magic   */
/*		numbers for disabling flash write protection. Write protection  */
/*		is restored after the write operation completes                 */
/*  01/13/2020(MichaelA): modified FEnum to add support for displaying  */
/*      the calibration constants of the ZmodADC and ZmodDAC            */
/*  01/13/2020(MichaelA): moved code specific to ZmodLOOP to ZmodLOOP.h */
/*      and ZmodLOOP.c                                                  */
/*	05/04/2020(ThomasK): namechange to dpmutil.c. Packaged for          */
/* 		baremetal                                                       */
/*                                                                      */
/************************************************************************/

/* ------------------------------------------------------------ */
/*                  Include File Definitions                    */
/* ------------------------------------------------------------ */
#include "../dpmutil/I2CHAL.h"
#include "../dpmutil/PlatformMCU.h"
#include "../dpmutil/stdtypes.h"
#include "../dpmutil/syzygy.h"
#include "../dpmutil/Zmod.h"
#include "../dpmutil/ZmodADC.h"
#include "../dpmutil/ZmodDAC.h"
#include "../dpmutil/ZmodDigitizer.h"

/* ------------------------------------------------------------ */
/*                  Miscellaneous Declarations                  */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/*                  General Type Declarations                   */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/*                  Variable Declarations                       */
/* ------------------------------------------------------------ */

typedef struct{
	DWORD 					pdid;
	float 					fwVer;
	float					cfgVer;
	PLATFORM_CONFIG 		platcfg;
	BYTE					cntVioPort;
	BYTE					cnt5v0;
	BYTE					cnt3v3;
	BYTE					cntVadj;
	BYTE					cntProbe;
	TEMPERATURE_ATTRIBUTES	probeAttr[4];
	SHORT					temp[4];
	BYTE					cntFan;
	FAN_CAPABILITIES		fanCapabilities[4];
	FAN_CONFIGURATION		fanConfig[4];
	WORD					fanRPM[4];
}dpmutildevInfo_t;

typedef struct{
	WORD					currentAllowed5v0;
	WORD					currentRequested5v0;
	WORD					currentAllowed3v3;
	WORD					currentRequested3v3;
	WORD					vadjVoltage;
	VADJ_OVERRIDE			vadjOverride;
	WORD					currentAllowedVadj;
	WORD					currentRequestedVadj;
}dpmutilPowerInfo_t;

typedef struct{
	BYTE					i2cAddr;
	BYTE					group5v0;
	BYTE					group3v3;
	BYTE					groupVio;
	BYTE					portType;
	PmcuPortStatus			portSts;
	WORD					voltage;
}dpmutilPortInfo_t;

/* ------------------------------------------------------------ */
/*                  Procedure Declarations                      */
/* ------------------------------------------------------------ */

BOOL	dpmutilFGetInfo(dpmutildevInfo_t* pDevInfo);
BOOL	dpmutilFGetInfoPower(int chanid, dpmutilPowerInfo_t pPowerInfo[]);
BOOL	dpmutilFGetInfo5V0(int chanid, dpmutilPowerInfo_t pPowerInfo[]);
BOOL	dpmutilFGetInfo3V3(int chanid, dpmutilPowerInfo_t pPowerInfo[]);
BOOL	dpmutilFGetInfoVio(int chanid, dpmutilPowerInfo_t pPowerInfo[]);
BOOL	dpmutilFEnum(BOOL setCrcCheck, BOOL crcCheck, dpmutilPortInfo_t pPortInfo[]);
BOOL	dpmutilFSetPlatformConfig(dpmutildevInfo_t* pDevInfo, BOOL setEnforce5v0, BOOL enforce5v0, BOOL setEnforce3v3, BOOL enforce3v3, BOOL setEnforceVio, BOOL enforceVio, BOOL setCrcCheck, BOOL crcCheck);
BOOL	dpmutilFSetVioConfig(int chanid, BOOL setEnable, BOOL enable, BOOL setOverride, BOOL override, BOOL setVoltage, WORD voltage);
BOOL	dpmutilFSetFanConfig(int fanid, BOOL setEnable, BOOL enable, BOOL setSpeed, BYTE speed, BOOL setProbe, BYTE probe);
BOOL	dpmutilFResetPMCU();

