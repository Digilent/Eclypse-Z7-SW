/************************************************************************/
/*                                                                      */
/*  main.cpp  --  Digilent Platform Management Utility main program     */
/*                                                                      */
/************************************************************************/
/*  Author: Michael T. Alexander                                        */
/*  Copyright 2019 Digilent, Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  The Digilent Platform Management Utility (dpmutil) is a program     */
/*  that provides a command line interface for discovering Zmods  		*/
/*  (SYZYGY Pods) attached to a Digilent platform board, acquiring      */
/*  information about them, acquiring information about the Digilent  	*/
/*  platform board, and setting certain settings pertaining to the      */
/*  configuration of the Digilent platform board.                       */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  08/21/2019(MichaelA): Created                                       */
/*	10/25/2019(MichaelA): modified FEnum to retrieve and display the	*/
/*		PDID for SYZYGY Pods manufactured by Digilent					*/
/*	10/28/2019(MichaelA): modified OpenI2cController to search the		*/
/*		"/sys/bus/i2c/devices" directory for a device whose				*/
/*		"device-name" is "pmcu-i2c" and if found open that device. If	*/
/*		no such device is found then we assume "/dev/i2c-0" is the		*/
/*		device entry corresponding to the I2C controller with the PMCU	*/
/*		I2C bus															*/
/*	01/03/2020(MichaelA): modified FEnum to add support for displaying	*/
/*		the 5V0, 3V3, and VIO voltages read by the pMCU on the ZmodLOOP	*/
/*	01/03/2020(MichaelA): added FWriteDNA function to add the ability	*/
/*		to write binary data to the DNA section of the pMCU flash		*/
/*	01/06/2020(MichaelA): modified FWriteDNA to add support for magic	*/
/*		numbers for disabling flash write protection. Write protection	*/
/*		is restored after the write operation completes					*/
/*  01/13/2020(MichaelA): modified FEnum to add support for displaying  */
/*      the calibration constants of the ZmodADC and ZmodDAC            */
/*  01/13/2020(MichaelA): moved code specific to ZmodLOOP to ZmodLOOP.h */
/*      and ZmodLOOP.c                                                  */
/*	05/15/2020(ThomasK): modified to work with dpmutil api				*/
/*                                                                      */
/************************************************************************/

#define _APPVERS	1.5

/* ------------------------------------------------------------ */
/*                  Include File Definitions                    */
/* ------------------------------------------------------------ */

#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <dirent.h>
#include <inttypes.h>
#include "dpmutil/dpmutil.h"

/* ------------------------------------------------------------ */
/*                  Miscellaneous Declarations                  */
/* ------------------------------------------------------------ */

#define cchCmdMax			64
#define cchDescriptionMax	64
#define cchOptionMax		64
#define cchVersionMax		256
#define cchDeviceNameMax	64

/* The following macros can be used to convert the value of a predefined
** macro into a string literal.
*/
#define SzFromMacroArg_(x)  #x
#define SzFromMacroArg(x)   SzFromMacroArg_(x)

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

typedef BOOL	(* PFNCMD)();

typedef struct {
	char	szCmd[cchCmdMax + 1];
	char	szDescription[cchDescriptionMax + 1];
	PFNCMD	pfncmd;
} CMD;

typedef struct {
	char	szOption[cchOptionMax + 1];
	char	szDescription[cchDescriptionMax + 1];
} OPTN;

/* ------------------------------------------------------------ */
/*                   Global Variables                           */
/* ------------------------------------------------------------ */

extern BOOL	dpmutilfVerbose;

/* ------------------------------------------------------------ */
/*                  Forward Declarations                        */
/* ------------------------------------------------------------ */


BOOL	FParseArguments(int cszArg, char* rgszArg[]);
BOOL	FCheckCmd(const char* szCmdCheck);

BOOL	FGetInfo();

BOOL	FGetInfoPower();
BOOL	FGetInfo5V0();
BOOL	FGetInfo3V3();
BOOL	FGetInfoVio();
BOOL	FEnum();
BOOL	FSetPlatformConfig();
BOOL	FSetVioConfig();
BOOL	FSetFanConfig();
BOOL	FResetPMCU();
BOOL	FHelp();
BOOL	FVersion();

void	PrintCommands();
void	PrintOptions();

/* ------------------------------------------------------------ */
/*                  Local Variables                             */
/* ------------------------------------------------------------ */

const char	szAppName[] = "Digilent Eclypse Utility";
const char	szContactInfo[] = "support@digilentinc.com";

//const char  szI2cDeviceName[] = "pmcu-i2c";
//const char	szI2cDeviceNameDefault[] = "/dev/i2c-0";

CMD     rgcmd[] = {
	{"getinfo",      "get basic configuration and supported platform features",    &FGetInfo },
	{"getinfopower", "get 5V0, 3V3, and VIO power supply information",             &FGetInfoPower },
	{"getinfovio",   "get VIO supply information",                                 &FGetInfoVio },
	{"getinfo5v0",   "get 5V0 supply information",                                 &FGetInfo5V0 },
	{"getinfo3v3",   "get 3V3 supply information",                                 &FGetInfo3V3 },
    {"enum",         "enumerate SmartVio ports (discover and list)",               &FEnum },
	{"setplatcfg",   "set the platform configuration register",                    &FSetPlatformConfig },
	{"setviocfg",    "set the VADJ_n_OVERRIDE reigster for a specific channel",    &FSetVioConfig },
	{"setfancfg",    "set the FAN_n_CONFIGURATION register for the specified fan", &FSetFanConfig },
	{"resetpmcu",    "reset the platform mcu",                                     &FResetPMCU },
    {"help",         "",                                                           &FHelp },
    {"version",      "",                                                           &FVersion },
    {"",             "",                                                           NULL }
};

OPTN   rgoptn[] = {
	{"-chanid      ", "channel identifier, chanid <0...7,a...h,A...H>"},
	{"-fanid       ", "fan identifier, fanid <1...4>"},
	{"-port        ", "port identifier, port <a...z, A...Z, 0...25>"},
	{"-enable      ", "enable or disable a function, enable <y/n>"},
	{"-override    ", "enable or disable VADJ override, override <y/n>"},
	{"-voltage     ", "voltage to set for VADJ override, voltage <millivolts>"},
	{"-enforce5v0  ", "enforce 5V0 current limit, enforce5v0 <y/n>"},
	{"-enforce3v3  ", "enforce 3V3 current limit, enforce3v3 <y/n>"},
	{"-enforcevio  ", "enforce VIO current limit, enforcevio <y/n>"},
	{"-checkcrc    ", "perform SYZYGY Header CRC check, checkrc <y/n>"},
	{"-speed       ", "fan speed, speed <minimum,medium,maximum,auto>"},
	{"-probe       ", "fan temperature probe, probe <none,p1,p2,p3,p4>"},
    {"-?, --help   ", "print usage, supported arguments, and options"},
    {"-v, --version", "print program version"},
//	{"--verbose    ", "display more detailed error messages"},
    {"", ""}
};

BOOL	fCmd;
BOOL	fSetEnforce5v0;
BOOL	fEnforce5v0;
BOOL	fSetEnforce3v3;
BOOL	fEnforce3v3;
BOOL	fSetEnforceVio;
BOOL	fEnforceVio;
BOOL	fSetCrcCheck;
BOOL	fCrcCheck;
BOOL	fChanid;
BOOL	fFanid;
BOOL	fPortid;
BOOL	fSetEnable;
BOOL	fEnable;
BOOL	fSetOverride;
BOOL	fOverride;
BOOL	fSetVoltage;
BOOL	fSetSpeed;
BOOL	fSetProbe;
//BOOL	fVerify;
//BOOL	fMagic;

char*	pszCmd;
//char*	pszDNAFile;
char	szCmd[cchCmdMax + 1];
BYTE	chanidGetSet;
BYTE	fanidGetSet;
BYTE	portid;
BYTE	fspeedSet;
BYTE	fprobeSet;
WORD	vltgSet;
dpmutildevInfo_t devInfo;
dpmutilPowerInfo_t powerInfo[8];
dpmutilPortInfo_t portInfo[8];
//BYTE	bMagic;

/* ------------------------------------------------------------ */
/*                  Procedure Definitions                       */
/* ------------------------------------------------------------ */
/***    main
**
**  Parameters:
**      cszArg  - number of command line arguments
**      rgszArg - array of command line argument strings
**
**  Return Values:
**      0 for success, 1 otherwise
**
**  Errors:
**
**  Description:
**      Application entry point.
*/
int
main( int cszArg, char* rgszArg[] ) {

	DWORD   icmd;
	PFNCMD  pfncmd;

	dpmutilfVerbose=fTrue;

	/* Parse the command and command options from the command line
	** arguments.
	*/
	if ( ! FParseArguments(cszArg, rgszArg) ) {
		/* If we failed to parse the command line arguments then an error
		** message has already been output to the user.
		*/
		return 1;
	}

	/* Check to see if the user specified a command.
	*/
	if ( ! fCmd ) {
		/* The user failed to specify a command so we can't do anything.
		*/
		printf("ERROR: no command specified\n");
		FHelp();
		return 1;
	}

	/* Acquire a pointer to the appropriate command handler.
	*/
	pfncmd = NULL;
	for ( icmd = 0; NULL != rgcmd[icmd].pfncmd; icmd++ ) {
		if ( 0 == strcmp(rgcmd[icmd].szCmd, szCmd) ) {
			pfncmd = rgcmd[icmd].pfncmd;
			break;
		}
	}

	if ( NULL == pfncmd ) {
		/* Failed to acquire a pointer to the appropriate command handler.
		*/
		printf("ERROR: invalid command specified: %s\n", szCmd);
		FHelp();
		return 1;
	}

	/* We acquired a pointer to the command handler. Now attempt to execute
	** the handler.
	*/
	if ( ! (*pfncmd)() ) {
		/* An error occurred during the execution of the command handler.
		** An appropriate error message should have been displayed by the
		** handler.
		*/
		return 1;
	}

	return 0;
}

BOOL FGetInfo(){
	return dpmutilFGetInfo(&devInfo);
}

BOOL	FGetInfoPower(){
	return dpmutilFGetInfoPower(chanidGetSet, powerInfo);
}
BOOL	FGetInfo5V0(){
	return dpmutilFGetInfo5V0(chanidGetSet, powerInfo);
}
BOOL	FGetInfo3V3(){
	return dpmutilFGetInfo3V3(chanidGetSet, powerInfo);
}
BOOL	FGetInfoVio(){
	return dpmutilFGetInfoVio(chanidGetSet, powerInfo);
}
BOOL	FEnum(){
	return dpmutilFEnum(fSetCrcCheck, fCrcCheck, portInfo);
}
BOOL	FSetPlatformConfig(){
	return dpmutilFSetPlatformConfig(&devInfo, fSetEnforce5v0, fEnforce5v0, fSetEnforce3v3, fEnforce3v3, fSetEnforceVio, fEnforceVio, fSetCrcCheck, fCrcCheck);
}
BOOL	FSetVioConfig(){
	return dpmutilFSetVioConfig(chanidGetSet, fSetEnable, fEnable, fSetOverride, fOverride, fSetVoltage, vltgSet);
}
BOOL	FSetFanConfig(){
	return dpmutilFSetFanConfig(fanidGetSet, fSetEnable, fEnable, fSetSpeed, fspeedSet, fSetProbe, fprobeSet);
}
BOOL	FResetPMCU(){
	return dpmutilFResetPMCU();
}


/* ------------------------------------------------------------ */
/***    FParseArguments
**
**  Parameters:
**      cszArg  - number of command line arguments
**      rgszArg - array of command line argument strings
**
**  Return Values:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**
**  Description:
**      Parse the command line arguments into commands and options.
*/
BOOL
FParseArguments(int cszArg, char* rgszArg[]) {

	int		iszArg;
	DWORD	ccmd;

	pszCmd = NULL;
//	pszDNAFile = NULL;

	/* Set the number of commands discovered thus far to 0. If the user
	** specifies more than one command then an error has occured because
	** we can only execute a single command.
	*/
	ccmd = 0;

	/* Set all of the flags to their default values of fFalse. Flags will
	** only be set to fTrue when the corresponding command or option has
	** been found and parsed from the command line arguments.
	*/
	fCmd = fFalse;
	fChanid = fFalse;
	fFanid = fFalse;
	fPortid = fFalse;
	fSetEnforce5v0 = fFalse;
	fEnforce5v0 = fFalse;
	fSetEnforce3v3 = fFalse;
	fEnforce3v3 = fFalse;
	fSetEnforceVio = fFalse;
	fEnforceVio = fFalse;
	fSetCrcCheck = fFalse;
	fCrcCheck = fFalse;
	fSetEnable = fFalse;
	fEnable = fFalse;
	fSetOverride = fFalse;
	fOverride = fFalse;
	fSetVoltage = fFalse;
	fSetSpeed = fFalse;
	fSetProbe = fFalse;
//	fVerbose = fFalse;

	/* Set all other parsed parameters to their default values.
	*/
	chanidGetSet = 0;
	fanidGetSet = 0;
	portid = 0;
	fspeedSet = fancfgMinimumSpeed;
	fprobeSet = fancfgTempProbeNone;
	vltgSet = 0;

	/* Set all of the string parameters to their default values: empty
	** strings.
	*/
	szCmd[0] = '\0';

	/* Get a pointer to the command string used to launch the application.
	** This is used when printing the usage as part of the help command.
	*/
	pszCmd = rgszArg[0];

	/* Parse the command line arguments into commands and options.
	*/
	iszArg = 1;
	while ( iszArg < cszArg ) {

		/* Check for the -chanid option. If this option is specified
		** then the user wants to specify the channel associated with
		** a get or set command.
		*/
		if ( 0 == strcmp(rgszArg[iszArg], "-chanid") ) {
			iszArg++;
			if ( iszArg >= cszArg ) {
				printf("ERROR: no channel identifier character specified\n");
				printf("specify '0' to '7', 'a' to 'f', or 'A' to 'F'\n");
				return fFalse;
			}

			if (( NULL == rgszArg[iszArg] ) || ( 1 != strlen(rgszArg[iszArg]) )) {
				printf("ERROR: invalid channel identifier character specified\n");
				printf("specify '0' to '7', 'a' to 'f', or 'A' to 'F'\n");
				return fFalse;
			}

			switch ( rgszArg[iszArg][0] ) {
				case '0':
				case 'a':
				case 'A':
					chanidGetSet = 0;
					break;
				case '1':
				case 'b':
				case 'B':
					chanidGetSet = 1;
					break;
				case '2':
				case 'c':
				case 'C':
					chanidGetSet = 2;
					break;
				case '3':
				case 'd':
				case 'D':
					chanidGetSet = 3;
					break;
				case '4':
				case 'e':
				case 'E':
					chanidGetSet = 4;
					break;
				case '5':
				case 'f':
				case 'F':
					chanidGetSet = 5;
					break;
				case '6':
				case 'g':
				case 'G':
					chanidGetSet = 6;
					break;
				case '7':
				case 'h':
				case 'H':
					chanidGetSet = 7;
					break;
				default:
					printf("ERROR: invalid channel identifier character specified\n");
					printf("specify '0' to '7', 'a' to 'f', or 'A' to 'F'\n");
					return fFalse;
			}

			fChanid = fTrue;
		}

		/* Check for the -fanid option. If this option is specified
		** then the user wants to specify the fan associated with
		** a get or set command.
		*/
		else if ( 0 == strcmp(rgszArg[iszArg], "-fanid") ) {
			iszArg++;
			if ( iszArg >= cszArg ) {
				printf("ERROR: no fan identifier character specified\n");
				printf("specify '1' to '4'\n");
				return fFalse;
			}

			if (( NULL == rgszArg[iszArg] ) || ( 1 != strlen(rgszArg[iszArg]) )) {
				printf("ERROR: invalid fan identifier specified\n");
				printf("specify '1' to '4'\n");
				return fFalse;
			}

			switch ( rgszArg[iszArg][0] ) {
				case '1':
					fanidGetSet = 0;
					break;
				case '2':
					fanidGetSet = 1;
					break;
				case '3':
					fanidGetSet = 2;
					break;
				case '4':
					fanidGetSet = 3;
					break;
				default:
					printf("ERROR: invalid fan identifier specified\n");
					printf("specify '1' to '4'\n");
					return fFalse;
			}

			fFanid = fTrue;
		}

		/* Check for the -port option. If this option is specified
		** then the user wants to specify the SVIO port identifier.
		*/
		else if ( 0 == strcmp(rgszArg[iszArg], "-port") ) {
			iszArg++;
			if ( iszArg >= cszArg ) {
				printf("ERROR: no port identifier character specified\n");
				printf("specify 'a' to 'z', 'A' to 'Z', or '0' to '25'\n");
				return fFalse;
			}

			if ( NULL == rgszArg[iszArg] ) {
				printf("ERROR: invalid port identifier specified\n");
				printf("specify 'a' to 'z', 'A' to 'Z', or '0' to '25'\n");
				return fFalse;
			}

			if ( 1 == strlen(rgszArg[iszArg]) ) {
				if (( 'a' <= rgszArg[iszArg][0] ) && ( 'z' >= rgszArg[iszArg][0] )) {
					portid = rgszArg[iszArg][0] - 'a';
				}
				else if (( 'A' <= rgszArg[iszArg][0] ) && ( 'Z' >= rgszArg[iszArg][0] )) {
					portid = rgszArg[iszArg][0] - 'A';
				}
				else if (( '0' <= rgszArg[iszArg][0] ) && ( '9' >= rgszArg[iszArg][0] )) {
					portid = rgszArg[iszArg][0] - '0';
				}
				else {
					printf("ERROR: invalid port identifier specified\n");
					printf("specify 'a' to 'z', 'A' to 'Z', or '0' to '25'\n");
					return fFalse;
				}
			}
			else {
				if ( 1 != sscanf(rgszArg[iszArg],"%" SCNu8 ,&portid) ) {
					printf("ERROR: invalid port identifier specified\n");
					printf("specify 'a' to 'z', 'A' to 'Z', or '0' to '25'\n");
					return fFalse;
				}
			}


			fPortid = fTrue;
		}

		/* Check for the -enable option. If this option is specified then
		** then the user wants to set or clear the enable bit in one of
		** the configuration registers.
		*/
		else if ( 0 == strcmp(rgszArg[iszArg], "-enable") ) {
			iszArg++;
			if ( iszArg >= cszArg ) {
				printf("ERROR: no enable option specified\n");
				printf("specify 'y' to enable the function, 'n' to disable it\n");
				return fFalse;
			}

			if (( NULL == rgszArg[iszArg] ) ||
				( 1 != strlen(rgszArg[iszArg]) ) ||
				(( 'y' != rgszArg[iszArg][0] ) && ( 'n' != rgszArg[iszArg][0] ))) {
				printf("ERROR: invalid enable option specified\n");
				printf("specify 'y' to enable the function, 'n' to disable it\n");
				return fFalse;
			}

			fSetEnable = fTrue;
			fEnable = ( 'y' == rgszArg[iszArg][0] ) ? fTrue : fFalse;
		}

		/* Check for the -enforce5v0 option. If this option is specified
		** then the user wants to set or clear the enforce 5v0 current
		** limit bit in the platform configuration register.
		*/
		else if ( 0 == strcmp(rgszArg[iszArg], "-enforce5v0") ) {
			iszArg++;
			if ( iszArg >= cszArg ) {
				printf("ERROR: no enforce 5v0 current limit option specified\n");
				printf("specify 'y' to enable current limit, 'n' to disable it\n");
				return fFalse;
			}

			if (( NULL == rgszArg[iszArg] ) ||
				( 1 != strlen(rgszArg[iszArg]) ) ||
				(( 'y' != rgszArg[iszArg][0] ) && ( 'n' != rgszArg[iszArg][0] ))) {
				printf("ERROR: invalid enforce 5v0 current limit option specified\n");
				printf("specify 'y' to enable current limit, 'n' to disable it\n");
				return fFalse;
			}

			fSetEnforce5v0 = fTrue;
			fEnforce5v0 = ( 'y' == rgszArg[iszArg][0] ) ? fTrue : fFalse;
		}

		/* Check for the -enforce3v3 option. If this option is specified
		** then the user wants to set or clear the enforce 3v3 current
		** limit bit in the platform configuration register.
		*/
		else if ( 0 == strcmp(rgszArg[iszArg], "-enforce3v3") ) {
			iszArg++;
			if ( iszArg >= cszArg ) {
				printf("ERROR: no enforce 3v3 current limit option specified\n");
				printf("specify 'y' to enable current limit, 'n' to disable it\n");
				return fFalse;
			}

			if (( NULL == rgszArg[iszArg] ) ||
				( 1 != strlen(rgszArg[iszArg]) ) ||
				(( 'y' != rgszArg[iszArg][0] ) && ( 'n' != rgszArg[iszArg][0] ))) {
				printf("ERROR: invalid enforce 3v3 current limit option specified\n");
				printf("specify 'y' to enable current limit, 'n' to disable it\n");
				return fFalse;
			}

			fSetEnforce3v3 = fTrue;
			fEnforce3v3 = ( 'y' == rgszArg[iszArg][0] ) ? fTrue : fFalse;
		}

		/* Check for the -enforcevio option. If this option is specified
		** then the user wants to set or clear the enforce VIO current
		** limit bit in the platform configuration register.
		*/
		else if ( 0 == strcmp(rgszArg[iszArg], "-enforcevio") ) {
			iszArg++;
			if ( iszArg >= cszArg ) {
				printf("ERROR: no enforce VIO current limit option specified\n");
				printf("specify 'y' to enable current limit, 'n' to disable it\n");
				return fFalse;
			}

			if (( NULL == rgszArg[iszArg] ) ||
				( 1 != strlen(rgszArg[iszArg]) ) ||
				(( 'y' != rgszArg[iszArg][0] ) && ( 'n' != rgszArg[iszArg][0] ))) {
				printf("ERROR: invalid enforce VIO current limit option specified\n");
				printf("specify 'y' to enable current limit, 'n' to disable it\n");
				return fFalse;
			}

			fSetEnforceVio = fTrue;
			fEnforceVio = ( 'y' == rgszArg[iszArg][0] ) ? fTrue : fFalse;
		}

		/* Check for the -checkcrc option. If this option is specified
		** then the user wants to set or clear the check crc bit in the
		** platform configuration register.
		*/
		else if ( 0 == strcmp(rgszArg[iszArg], "-checkcrc") ) {
			iszArg++;
			if ( iszArg >= cszArg ) {
				printf("ERROR: no check crc option specified\n");
				printf("specify 'y' to enable crc check, 'n' to disable it\n");
				return fFalse;
			}

			if (( NULL == rgszArg[iszArg] ) ||
				( 1 != strlen(rgszArg[iszArg]) ) ||
				(( 'y' != rgszArg[iszArg][0] ) && ( 'n' != rgszArg[iszArg][0] ))) {
				printf("ERROR: invalid check crc option specified\n");
				printf("specify 'y' to enable crc check, 'n' to disable it\n");
				return fFalse;
			}

			fSetCrcCheck = fTrue;
			fCrcCheck = ( 'y' == rgszArg[iszArg][0] ) ? fTrue : fFalse;
		}

		/* Check for the -override option. If this option is specified then
		** then the user wants to set or clear the override bit in the
		** VADJ_n_OVERRIDE configuration register.
		*/
		else if ( 0 == strcmp(rgszArg[iszArg], "-override") ) {
			iszArg++;
			if ( iszArg >= cszArg ) {
				printf("ERROR: no override option specified\n");
				printf("specify 'y' to enable override, 'n' to disable it\n");
				return fFalse;
			}

			if (( NULL == rgszArg[iszArg] ) ||
				( 1 != strlen(rgszArg[iszArg]) ) ||
				(( 'y' != rgszArg[iszArg][0] ) && ( 'n' != rgszArg[iszArg][0] ))) {
				printf("ERROR: invalid override option specified\n");
				printf("specify 'y' to enable override, 'n' to disable it\n");
				return fFalse;
			}

			fSetOverride = fTrue;
			fOverride = ( 'y' == rgszArg[iszArg][0] ) ? fTrue : fFalse;
		}

		/* Check for the -speed option. If this option is specified then
		** then the user wants to set the speed associated with a particular
		** fan.
		*/
		else if ( 0 == strcmp(rgszArg[iszArg], "-speed") ) {
			iszArg++;
			if ( iszArg >= cszArg ) {
				printf("ERROR: no speed string specified\n");
				printf("specify \"minimum\", \"medium\", \"maximum\", or \"auto\"\n");
				return fFalse;
			}

			if ( NULL == rgszArg[iszArg] ) {
				printf("ERROR: invalid speed string specified\n");
				printf("specify \"minimum\", \"medium\", \"maximum\", or \"auto\"\n");
				return fFalse;
			}

			if ( 0 == strcmp(rgszArg[iszArg], "minimum") ) {
				fspeedSet = fancfgMinimumSpeed;
			}
			else if ( 0 == strcmp(rgszArg[iszArg], "medium") ) {
				fspeedSet = fancfgMediumSpeed;
			}
			else if ( 0 == strcmp(rgszArg[iszArg], "maximum") ) {
				fspeedSet = fancfgMaximumSpeed;
			}
			else if ( 0 == strcmp(rgszArg[iszArg], "auto") ) {
				fspeedSet = fancfgAutoSpeed;
			}
			else {
				printf("ERROR: invalid speed string specified\n");
				printf("specify \"minimum\", \"medium\", \"maximum\", or \"auto\"\n");
				return fFalse;
			}

			fSetSpeed = fTrue;
		}

		/* Check for the -probe option. If this option is specified then
		** then the user wants to set the temperature probe associated
		** with a particular fan.
		*/
		else if ( 0 == strcmp(rgszArg[iszArg], "-probe") ) {
			iszArg++;
			if ( iszArg >= cszArg ) {
				printf("ERROR: no probe string specified\n");
				printf("specify \"none\", \"p1\", \"p2\", \"p3\", \"p4\"\n");
				return fFalse;
			}

			if ( NULL == rgszArg[iszArg] ) {
				printf("ERROR: invalid probe string specified\n");
				printf("specify \"none\", \"p1\", \"p2\", \"p3\", \"p4\"\n");
				return fFalse;
			}

			if ( 0 == strcmp(rgszArg[iszArg], "none") ) {
				fprobeSet = fancfgTempProbeNone;
			}
			else if ( 0 == strcmp(rgszArg[iszArg], "p1") ) {
				fprobeSet = fancfgTempProbe1;
			}
			else if ( 0 == strcmp(rgszArg[iszArg], "p2") ) {
				fprobeSet = fancfgTempProbe2;
			}
			else if ( 0 == strcmp(rgszArg[iszArg], "p3") ) {
				fprobeSet = fancfgTempProbe3;
			}
			else if ( 0 == strcmp(rgszArg[iszArg], "p4") ) {
				fprobeSet = fancfgTempProbe4;
			}
			else {
				printf("ERROR: invalid probe string specified\n");
				printf("specify \"none\", \"p1\", \"p2\", \"p3\", \"p4\"\n");
				return fFalse;
			}

			fSetProbe = fTrue;
		}

		/* Check for the -voltage option. If this option is specified then
		** then the user wants to set the voltage in one of the
		** VADJ_n_OVERRIDE configuration registers.
		*/
		else if ( 0 == strcmp(rgszArg[iszArg], "-voltage") ) {
			iszArg++;
			if ( iszArg >= cszArg ) {
				printf("ERROR: no voltage specified\n");
				printf("specify a value in millivolts\n");
				return fFalse;
			}

			if (( NULL == rgszArg[iszArg] ) ||
				( 1 != sscanf(rgszArg[iszArg], "%hu", &vltgSet) )) {
				printf("ERROR: invalid voltage specified\n");
				printf("specify a value in millivolts\n");
				return fFalse;
			}

			fSetVoltage = fTrue;
		}

//		else if ( 0 == strcmp(rgszArg[iszArg], "-magic") ) {
//			iszArg++;
//			if ( iszArg >= cszArg ) {
//				printf("ERROR: no magic number specified\n");
//				return fFalse;
//			}
//
//			if (( NULL == rgszArg[iszArg] ) ||
//				( 1 != sscanf(rgszArg[iszArg], "%x", &dwTemp) )) {
//				printf("ERROR: invalid magic number specified\n");
//				return fFalse;
//			}
//
//			bMagic = dwTemp & 0xFF;
//			fMagic = fTrue;
//		}

//		else if ( 0 == strcmp(rgszArg[iszArg], "-dnafile") ) {
//			iszArg++;
//			if (( iszArg >= cszArg ) || ( NULL == rgszArg[iszArg] )) {
//				printf("ERROR: no DNA binary filename specified\n");
//				return fFalse;
//			}
//
//			pszDNAFile = rgszArg[iszArg];
//		}

		/* Check for the -? and --help options. These specify whether
		** or not the user wants the help command to be executed.
		*/
		else if (( 0 == strcmp(rgszArg[iszArg], "-?") ) ||
				 ( 0 == strcmp(rgszArg[iszArg], "--help") )) {
			/* The help option/command is given priority over all commands.
			** We don't need to do any more parsing because we are going
			** to execute the handler for the help command which doesn't
			** require any options.
			*/
			strcpy(szCmd, "help");
			fCmd = fTrue;
			ccmd = 1;
			goto lExit;
		}

		/* Check for the -v and --version options. These specify whether
		** or not the user wants the version command to be executed.
		*/
		else if (( 0 == strcmp(rgszArg[iszArg], "-v") ) ||
				 ( 0 == strcmp(rgszArg[iszArg], "--version") )) {
			/* The version option/command is given priority over all other
			** commands. We don't need to do any more parsing because we
			** are going to execute the handler for the version command
			** which doesn't require any options.
			*/
			strcpy(szCmd, "version");
			fCmd = fTrue;
			ccmd = 1;
			goto lExit;
		}

		/* Check for the --verbose option. If this option is specified
		** then the user wishes to enable more detailed error messages.
		*/
//		else if ( 0 == strcmp(rgszArg[iszArg], "--verbose") ) {
//			fVerbose = fTrue;
//		}

		/* Check for the -verify option. If this option is specified
		** then the user wishes to perform a read back verification of
		** data that was written.
		*/
//		else if ( 0 == strcmp(rgszArg[iszArg], "-verify") ) {
//			fVerify = fTrue;
//		}

		/* Assume that the argument is the command to be performed.
		*/
		else {
			if (( NULL == rgszArg[iszArg] ) || ( '-' == rgszArg[iszArg][0] )) {
				printf("ERROR: invalid command or option specified: ");
				if ( NULL != rgszArg[iszArg] ) {
					printf("%s", rgszArg[iszArg]);
				}
				printf("\n");
				return fFalse;
			}

			if ( ! FCheckCmd(rgszArg[iszArg]) ) {
				printf("ERROR: invalid command specified: %s\n", rgszArg[iszArg]);
				printf("\n");
				printf("  Supported commands:\n");
				PrintCommands();
				return fFalse;
			}

			strcpy(szCmd, rgszArg[iszArg]);
			fCmd = fTrue;
			ccmd++;
		}

		iszArg++;
	}

lExit:

	/* Check to make sure that the user specified at most one command.
	*/
	if ( 1 < ccmd ) {
		printf("ERROR: more than one command specified\n");
		return fFalse;
	}

	return fTrue;
}

/* ------------------------------------------------------------ */
/***    FCheckCmd
**
**  Parameters:
**      szCmdCheck - command string to check
**
**  Return Value:
**      fTrue if string is valid, fFalse otherwise.
**
**  Errors:
**      Returns fTrue if successful, fFalse if not
**
**  Description:
**      This function checks the command string to make sure that it is
**      valid and that the specified command exists.
*/
BOOL
FCheckCmd(const char* szCmdCheck) {

	DWORD   icmd;

	if (( NULL == szCmdCheck ) || ( cchCmdMax < strlen(szCmdCheck) )) {
		return fFalse;
	}

	/* Search the command table for the specified command string.
	*/
	icmd = 0;
	while ( 0 != strlen(rgcmd[icmd].szCmd) ) {
		if ( 0 == strcmp(rgcmd[icmd].szCmd, szCmdCheck) ) {
			/* We found the specified command string in the command table.
			*/
			return fTrue;
		}
		icmd++;
	}

	return fFalse;
}

/* ------------------------------------------------------------ */
/***    FHelp
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
**      Display usage and other information that the user may find
**      helpful.
*/
BOOL
FHelp() {

	printf("Usage: %s [--help] [--version] command [options]\n", pszCmd);

	printf("\n");

	printf("  Commands:\n");

	PrintCommands();

	printf("\n");

	printf("  Options:\n");

	PrintOptions();

	return fTrue;
}

/* ------------------------------------------------------------ */
/***    FVersion
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
**      Display application version information.
*/
BOOL
FVersion() {

	char    szVersion[cchVersionMax + 1];

	sprintf(szVersion, "%s", SzFromMacroArg(_APPVERS));

	printf("%s v%s\n", szAppName, szVersion);

	printf("Copyright (C) 2019 Digilent, Inc.\n");

	printf("%s\n", szContactInfo);

	return fTrue;
}

/* ------------------------------------------------------------ */
/***    PrintCommands
**
**  Parameters:
**      none
**
**  Return Value:
**      none
**
**  Errors:
**      none
**
**  Description:
**      This function prints a list of commands that are currently
**      supported by the application.
*/
void
PrintCommands() {

	DWORD	icmd;

	icmd = 0;
	while ( 0 < strlen(rgcmd[icmd].szCmd) ) {

		if (( 0 != strcmp(rgcmd[icmd].szCmd, "help") ) &&
			( 0 != strcmp(rgcmd[icmd].szCmd, "version") )) {

			printf("    %-20s    %s\n",
			rgcmd[icmd].szCmd,
			rgcmd[icmd].szDescription);
		}

		icmd++;
	}
}


/* ------------------------------------------------------------ */
/***    PrintOptions
**
**  Parameters:
**      none
**
**  Return Value:
**      none
**
**  Errors:
**      none
**
**  Description:
**      This function prints a list of the command line options that are
**      currently supported by the application to stdout.
*/
void
PrintOptions() {

	DWORD	ioptn;

	ioptn = 0;
	while ( 0 < strlen(rgoptn[ioptn].szOption) ) {

		printf("    %-20s    %s\n",
		rgoptn[ioptn].szOption,
		rgoptn[ioptn].szDescription);
		ioptn++;
	}
}
