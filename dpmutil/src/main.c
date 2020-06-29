/************************************************************************/
/*                                                                      */
/*  main.c - dpmutil demo application		                            */
/*                                                                      */
/************************************************************************/
/*  Author: Thomas Kappenman	                                        */
/*  Copyright 2020, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*	This file contains the demo application for dpmutil. This is used	*/
/*	to interface with the onboard power management unit	(PMU) and 		*/
/*	syzygy ports.														*/
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  05/05/20 (ThomasK): created                                	        */
/*                                                                      */
/************************************************************************/

#include "xparameters.h"
#if defined(PLATFORM_ZYNQ)
#include "xuartps.h"
#elif defined (XPAR_XUARTLITE_NUM_INSTANCES)
#include "xuartlite.h"
#else
#warning "This demo is tested with Zynq and Microblaze + Uartlite. Other UART cores are not implemented."
#endif

#include "dpmutil/dpmutil.h"
#include <stdio.h>

/* ------------------------------------------------------------ */
/*                  Miscellaneous Declarations                  */
/* ------------------------------------------------------------ */

#define cchCmdMax			64
#define cchDescriptionMax	64
#define cchOptionMax		64
#define cchMessageMax		256

/* ------------------------------------------------------------ */
/*                  Local Type Definitions                      */
/* ------------------------------------------------------------ */

typedef BOOL	(* PFNCMD)();

typedef struct {
	char	szCmd[cchCmdMax + 1];
	char	szDescription[cchDescriptionMax + 1];
} CMD;

typedef struct {
	char	szOption[cchOptionMax + 1];
	char	szDescription[cchDescriptionMax + 1];
} OPTN;

#pragma pack(push, 1)

typedef union {
	struct {
		unsigned fchanid:1;
		unsigned chanid:3;
		unsigned ffanid:1;
		unsigned fanid:3;
		unsigned fport:1;
		unsigned fenable:1;
		unsigned enable:1;
		unsigned foverride:1;
		unsigned override:1;
		unsigned fsetvoltage:1;
		unsigned fenforce5v0:1;
		unsigned enforce5v0:1;
		unsigned fenforce3v3:1;
		unsigned enforce3v3:1;
		unsigned fenforcevio:1;
		unsigned enforcevio:1;
		unsigned fcheckcrc:1;
		unsigned checkcrc:1;
		unsigned fspeed:1;
		unsigned speed:2;
		unsigned fprobe:1;
		unsigned probe:3;
		unsigned rsv:3;
	};
	UINT32 flags;
} FLAGS_t;
#pragma pack(pop)

/* ------------------------------------------------------------ */
/*                  Forward Declarations                        */
/* ------------------------------------------------------------ */

XStatus initDemo();
BOOL FHelp();
void PrintCommands();
void PrintOptions();
BOOL getLine();
int parseArguments();

/* ------------------------------------------------------------ */
/*                  Local Variables                             */
/* ------------------------------------------------------------ */


static CMD     rgcmd[] = {
	{"getinfo",      "get basic configuration and supported platform features"},
	{"getinfopower", "get 5V0, 3V3, and VIO power supply information" },
	{"getinfovio",   "get VIO supply information" },
	{"getinfo5v0",   "get 5V0 supply information" },
	{"getinfo3v3",   "get 3V3 supply information" },
    {"enum",         "enumerate SmartVio ports (discover and list)" },
	{"setplatcfg",   "set the platform configuration register" },
	{"setviocfg",    "set the VADJ_n_OVERRIDE reigster for a specific channel" },
	{"setfancfg",    "set the FAN_n_CONFIGURATION register for the specified fan" },
	{"resetpmcu",    "reset the platform mcu" },
    {"help",         "" }
};

static OPTN   rgoptn[] = {
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
    {"", ""}
};

#if defined(PLATFORM_ZYNQ)
static XUartPs Uart;
#elif defined (XPAR_XUARTLITE_NUM_INSTANCES)
static XUartLite Uart;
#endif
static char chUartMsg[cchMessageMax];
static WORD voltageset;
static BYTE port;
static FLAGS_t flags;
static dpmutildevInfo_t deviceInfo;
static dpmutilPowerInfo_t powerInfo[8];
static dpmutilPortInfo_t portInfo[8];
extern BOOL dpmutilfVerbose;

int main()
{
	int cmd;

	dpmutilfVerbose = fTrue;//Turn verbose on for the demo.

	if(initDemo()!= XST_SUCCESS){
		xil_printf("Error during initialization!\r\nStopping...\r\n");
		return 0;
	}
	FHelp();
	xil_printf("\r\nEnter command:");

	while(1){
		if(getLine()){
			cmd = parseArguments();

			switch(cmd){
			case 0://getinfo
				dpmutilFGetInfo(&deviceInfo);
				break;
			case 1://getinfopower
				if(flags.fchanid==0){
					dpmutilFGetInfoPower(-1, powerInfo);
				}
				else{
					xil_printf("%d\r\n",flags.chanid);
					dpmutilFGetInfoPower(flags.chanid, powerInfo);
				}
				break;
			case 2://getinfovio
				if(flags.fchanid==0){
					dpmutilFGetInfoVio(-1, powerInfo);
				}
				else{
					xil_printf("%d\r\n",flags.chanid);
					dpmutilFGetInfoVio(flags.chanid, powerInfo);
				}
				break;
			case 3://getinfo5v0
				if(flags.fchanid==0){
					dpmutilFGetInfo5V0(-1, powerInfo);
				}
				else{
					xil_printf("%d\r\n",flags.chanid);
					dpmutilFGetInfo5V0(flags.chanid, powerInfo);
				}
				break;
			case 4://getinfo3v3
				if(flags.fchanid==0){
					dpmutilFGetInfo3V3(-1, powerInfo);
				}
				else{
					xil_printf("%d\r\n",flags.chanid);
					dpmutilFGetInfo3V3(flags.chanid, powerInfo);
				}
				break;
			case 5://enum
				dpmutilFEnum(flags.fcheckcrc, flags.checkcrc, portInfo);
				break;
			case 6://setplatcfg
				dpmutilFSetPlatformConfig(&deviceInfo, flags.fenforce5v0, flags.enforce5v0,
						flags.fenforce3v3, flags.enforce3v3, flags.fenforcevio,
						flags.enforcevio, flags.fcheckcrc, flags.checkcrc);
				break;
			case 7://setviocfg
				dpmutilFSetVioConfig(flags.chanid, flags.fenable,
						flags.enable, flags.foverride, flags.override,
						flags.fsetvoltage, voltageset);
				break;
			case 8://setfancfg
				dpmutilFSetFanConfig(flags.fanid, flags.fenable,
						flags.enable, flags.fspeed, flags.speed,
						flags.fprobe, flags.probe);
				break;
			case 9://resetpmcu
				dpmutilFResetPMCU();
				break;
			case 10://help
				FHelp();
				break;
			default:
				//xil_printf("Invalid command %s\r\n", arg);
				break;
			}
			cmd=-1;
			xil_printf("\r\nEnter command:");
		}
	}


	return 0;
}

XStatus initDemo(){
	XStatus status;
#if defined(PLATFORM_ZYNQ)
	XUartPs_Config* UartConfig;

	UartConfig = XUartPs_LookupConfig(XPAR_PS7_UART_0_DEVICE_ID);
	if (!UartConfig){
		return XST_FAILURE;
	}
	status =  XUartPs_CfgInitialize(&Uart, UartConfig, UartConfig->BaseAddress);
	if(status!=XST_SUCCESS){
		return status;
	}
	status = XUartPs_SetBaudRate(&Uart, 115200);
	if(status!=XST_SUCCESS){
		return status;
	}
	status = XUartPs_SelfTest(&Uart);
	if(status!=XST_SUCCESS){
		return status;
	}
#elif defined (XPAR_XUARTLITE_NUM_INSTANCES)
	XUartLite_Config* UartConfig;

	UartConfig = XUartLite_LookupConfig(XPAR_AXI_UARTLITE_0_DEVICE_ID);
	if (!UartConfig){
		return XST_FAILURE;
	}
	status =  XUartLite_CfgInitialize(&Uart, UartConfig, UartConfig->RegBaseAddr);
	if(status!=XST_SUCCESS){
		return status;
	}
	status = XUartLite_SelfTest(&Uart);
	if(status!=XST_SUCCESS){
		return status;
	}
#endif

	return XST_SUCCESS;
}

BOOL getLine(){
	static int cbRecv;
	static int iCmdChar=0;
#if defined(PLATFORM_ZYNQ)
	while((cbRecv = XUartPs_Recv(&Uart, (u8*)&chUartMsg[iCmdChar], 1))){
#elif defined (XPAR_XUARTLITE_NUM_INSTANCES)
	while((cbRecv = XUartLite_Recv(&Uart, (u8*)&chUartMsg[iCmdChar], 1))){
#endif
		//Newline (Enter)
		if (chUartMsg[iCmdChar] == '\n' || chUartMsg[iCmdChar] == '\r'){
			xil_printf("\r\n");
			chUartMsg[iCmdChar] = '\0';
			strcpy(chUartMsg, strlwr(chUartMsg));
			if(iCmdChar==0)return fFalse;
			iCmdChar = 0;
			return fTrue;
		}
		//Acceptable characters
		else if((tolower(chUartMsg[iCmdChar])>='a' && tolower(chUartMsg[iCmdChar])<='z') ||
				(chUartMsg[iCmdChar]>'0' && chUartMsg[iCmdChar]<'9') ||
				chUartMsg[iCmdChar]=='-' || chUartMsg[iCmdChar] == '?'|| chUartMsg[iCmdChar] == ' '){
			xil_printf("%c", chUartMsg[iCmdChar]);

			iCmdChar++;
			if (iCmdChar> cchMessageMax){
				iCmdChar=0;
			}
		}
		//Backspace
		else if(chUartMsg[iCmdChar]=='\b' && iCmdChar>0){
			xil_printf("\b \b");//Destructive backspace
			iCmdChar--;
		}
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

	xil_printf("  Commands:\r\n");

	PrintCommands();

	xil_printf("\r\n");

	xil_printf("  Options:\r\n");

	PrintOptions();

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

			xil_printf("    %-20s    %s\r\n",
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

		xil_printf("    %-20s    %s\r\n",
		rgoptn[ioptn].szOption,
		rgoptn[ioptn].szDescription);
		ioptn++;
	}
}


int parseArguments(){
	char* arg;
	u32 i;
	int cmd=-1;
	voltageset=0;
	port=0;
	memset(&flags, 0, sizeof(flags));

	arg = strtok(chUartMsg, " \r\n");
	for(i=0; i<(sizeof(rgcmd)/sizeof(CMD)); i++){
		if(strcmp(arg, rgcmd[i].szCmd)==0){
			cmd=i;
			break;
		}
	}
	if (cmd==-1){
		xil_printf("Invalid command %s\r\n", arg);
		return -1;
	}
	while((arg = strtok(NULL, " \r\n")) != NULL){
		/* Check for the -chanid option. If this option is specified
		** then the user wants to specify the channel associated with
		** a get or set command.
		*/
		if ( 0 == strcmp(arg, "-chanid") ) {
			arg = strtok(NULL, " \r\n");
			if ( arg == NULL || ( 1 != strlen(arg) )) {
				xil_printf("ERROR: invalid channel identifier character specified\r\n");
				xil_printf("specify '0' to '7', 'a' to 'f', or 'A' to 'F'\r\n");
				return -1;
			}

			switch ( arg[0] ) {
				case '0':
				case 'a':
				case 'A':
					flags.chanid = 0;
					break;
				case '1':
				case 'b':
				case 'B':
					flags.chanid = 1;
					break;
				case '2':
				case 'c':
				case 'C':
					flags.chanid = 2;
					break;
				case '3':
				case 'd':
				case 'D':
					flags.chanid = 3;
					break;
				case '4':
				case 'e':
				case 'E':
					flags.chanid = 4;
					break;
				case '5':
				case 'f':
				case 'F':
					flags.chanid = 5;
					break;
				case '6':
				case 'g':
				case 'G':
					flags.chanid = 6;
					break;
				case '7':
				case 'h':
				case 'H':
					flags.chanid = 7;
					break;
				default:
					xil_printf("ERROR: invalid channel identifier character specified\r\n");
					xil_printf("specify '0' to '7', 'a' to 'f', or 'A' to 'F'\r\n");
					return -1;
			}

			flags.fchanid = 1;
		}

		/* Check for the -fanid option. If this option is specified
		** then the user wants to specify the fan associated with
		** a get or set command.
		*/
		else if ( 0 == strcmp(arg, "-fanid") ) {
			arg = strtok(NULL, " \r\n");
			if (( NULL == arg) || ( 1 != strlen(arg) )) {
				xil_printf("ERROR: invalid fan identifier specified\r\n");
				xil_printf("specify '1' to '4'\r\n");
				return -1;
			}

			switch ( arg[0] ) {
				case '1':
				case 'a':
				case 'A':
					flags.fanid = 0;
					break;
				case '2':
				case 'b':
				case 'B':
					flags.fanid = 1;
					break;
				case '3':
				case 'c':
				case 'C':
					flags.fanid = 2;
					break;
				case '4':
				case 'd':
				case 'D':
					flags.fanid = 3;
					break;
				default:
					xil_printf("ERROR: invalid fan identifier specified\r\n");
					xil_printf("specify '1' to '4'\r\n");
					return -1;
			}

			flags.ffanid = 1;
		}

		/* Check for the -port option. If this option is specified
		** then the user wants to specify the SVIO port identifier.
		*/
		else if ( 0 == strcmp(arg, "-port") ) {
			arg = strtok(NULL, " \r\n");
			if ( NULL == arg ) {
				xil_printf("ERROR: invalid port identifier specified\r\n");
				xil_printf("specify 'a' to 'z', 'A' to 'Z', or '0' to '25'\r\n");
				return -1;
			}

			if ( 1 == strlen(arg) ) {
				if (( 'a' <= arg[0] ) && ( 'z' >= arg[0] )) {
					port = arg[0] - 'a';
				}
				else if (( '0' <= arg[0] ) && ( '9' >= arg[0] )) {
					port = arg[0] - '0';
				}
				else {
					xil_printf("ERROR: invalid port identifier specified\r\n");
					xil_printf("specify 'a' to 'z', 'A' to 'Z', or '0' to '25'\r\n");
					return -1;
				}
			}
			else {
				if ( 1 != sscanf(arg,"%" "hhu" ,&port) ) {
					xil_printf("ERROR: invalid port identifier specified\r\n");
					xil_printf("specify 'a' to 'z', 'A' to 'Z', or '0' to '25'\r\n");
					return -1;
				}
			}


			flags.fport = 1;
		}

		/* Check for the -enable option. If this option is specified then
		** then the user wants to set or clear the enable bit in one of
		** the configuration registers.
		*/
		else if ( 0 == strcmp(arg, "-enable") ) {
			arg = strtok(NULL, " \r\n");

			if (( NULL == arg ) ||
				( 1 != strlen(arg) ) ||
				(( 'y' != arg[0] ) && ( 'n' != arg[0] ))) {
				xil_printf("ERROR: invalid enable option specified\r\n");
				xil_printf("specify 'y' to enable the function, 'n' to disable it\r\n");
				return -1;
			}

			flags.fenable = 1;
			flags.enable = ( 'y' == arg[0] ) ? 1 : 0;
		}

		/* Check for the -enforce5v0 option. If this option is specified
		** then the user wants to set or clear the enforce 5v0 current
		** limit bit in the platform configuration register.
		*/
		else if ( 0 == strcmp(arg, "-enforce5v0") ) {
			arg = strtok(NULL, " \r\n");

			if (( NULL == arg ) ||
				( 1 != strlen(arg) ) ||
				(( 'y' != arg[0] ) && ( 'n' != arg[0] ))) {
				xil_printf("ERROR: invalid enforce 5v0 current limit option specified\r\n");
				xil_printf("specify 'y' to enable current limit, 'n' to disable it\r\n");
				return -1;
			}

			flags.fenforce5v0 = 1;
			flags.enforce5v0 = ( 'y' == arg[0] ) ? 1 : 0;
		}

		/* Check for the -enforce3v3 option. If this option is specified
		** then the user wants to set or clear the enforce 3v3 current
		** limit bit in the platform configuration register.
		*/
		else if ( 0 == strcmp(arg, "-enforce3v3") ) {
			arg = strtok(NULL, " \r\n");

			if (( NULL == arg ) ||
				( 1 != strlen(arg) ) ||
				(( 'y' != arg[0] ) && ( 'n' != arg[0] ))) {
				xil_printf("ERROR: invalid enforce 3v3 current limit option specified\r\n");
				xil_printf("specify 'y' to enable current limit, 'n' to disable it\r\n");
				return -1;
			}

			flags.fenforce3v3 = 1;
			flags.enforce3v3 = ( 'y' == arg[0] ) ? 1 : 0;
		}

		/* Check for the -enforcevio option. If this option is specified
		** then the user wants to set or clear the enforce VIO current
		** limit bit in the platform configuration register.
		*/
		else if ( 0 == strcmp(arg, "-enforcevio") ) {
			arg = strtok(NULL, " \r\n");

			if (( NULL == arg ) ||
				( 1 != strlen(arg) ) ||
				(( 'y' != arg[0] ) && ( 'n' != arg[0] ))) {
				xil_printf("ERROR: invalid enforce VIO current limit option specified\r\n");
				xil_printf("specify 'y' to enable current limit, 'n' to disable it\r\n");
				return -1;
			}

			flags.fenforcevio = 1;
			flags.enforcevio = ( 'y' == arg[0] ) ? 1 : 0;
		}

		/* Check for the -checkcrc option. If this option is specified
		** then the user wants to set or clear the check crc bit in the
		** platform configuration register.
		*/
		else if ( 0 == strcmp(arg, "-checkcrc") ) {
			arg = strtok(NULL, " \r\n");

			if (( NULL == arg ) ||
				( 1 != strlen(arg) ) ||
				(( 'y' != arg[0] ) && ( 'n' != arg[0] ))) {
				xil_printf("ERROR: invalid check crc option specified\r\n");
				xil_printf("specify 'y' to enable crc check, 'n' to disable it\r\n");
				return -1;
			}

			flags.fcheckcrc = 1;
			flags.checkcrc = ( 'y' == arg[0] ) ? 1 : 0;
		}

		/* Check for the -override option. If this option is specified then
		** then the user wants to set or clear the override bit in the
		** VADJ_n_OVERRIDE configuration register.
		*/
		else if ( 0 == strcmp(arg, "-override") ) {
			arg = strtok(NULL, " \r\n");

			if (( NULL == arg ) ||
				( 1 != strlen(arg) ) ||
				(( 'y' != arg[0] ) && ( 'n' != arg[0] ))) {
				xil_printf("ERROR: invalid override option specified\r\n");
				xil_printf("specify 'y' to enable override, 'n' to disable it\r\n");
				return -1;
			}

			flags.foverride = 1;
			flags.override = ( 'y' == arg[0] ) ? 1 : 0;
		}

		/* Check for the -speed option. If this option is specified then
		** then the user wants to set the speed associated with a particular
		** fan.
		*/
		else if ( 0 == strcmp(arg, "-speed") ) {
			arg = strtok(NULL, " \r\n");

			if ( NULL == arg ) {
				xil_printf("ERROR: invalid speed string specified\r\n");
				xil_printf("specify \"minimum\", \"medium\", \"maximum\", or \"auto\"\r\n");
				return -1;
			}

			if ( 0 == strcmp(arg, "minimum") ) {
				flags.speed = fancfgMinimumSpeed;
			}
			else if ( 0 == strcmp(arg, "medium") ) {
				flags.speed = fancfgMediumSpeed;
			}
			else if ( 0 == strcmp(arg, "maximum") ) {
				flags.speed = fancfgMaximumSpeed;
			}
			else if ( 0 == strcmp(arg, "auto") ) {
				flags.speed = fancfgAutoSpeed;
			}
			else {
				xil_printf("ERROR: invalid speed string specified\r\n");
				xil_printf("specify \"minimum\", \"medium\", \"maximum\", or \"auto\"\r\n");
				return -1;
			}

			flags.fspeed = 1;
		}

		/* Check for the -probe option. If this option is specified then
		** then the user wants to set the temperature probe associated
		** with a particular fan.
		*/
		else if ( 0 == strcmp(arg, "-probe") ) {
			arg = strtok(NULL, " \r\n");

			if ( NULL == arg ) {
				xil_printf("ERROR: invalid probe string specified\r\n");
				xil_printf("specify \"none\", \"p1\", \"p2\", \"p3\", \"p4\"\r\n");
				return -1;
			}

			if ( 0 == strcmp(arg, "none") ) {
				flags.probe = fancfgTempProbeNone;
			}
			else if ( 0 == strcmp(arg, "p1") ) {
				flags.probe = fancfgTempProbe1;
			}
			else if ( 0 == strcmp(arg, "p2") ) {
				flags.probe = fancfgTempProbe2;
			}
			else if ( 0 == strcmp(arg, "p3") ) {
				flags.probe = fancfgTempProbe3;
			}
			else if ( 0 == strcmp(arg, "p4") ) {
				flags.probe = fancfgTempProbe4;
			}
			else {
				xil_printf("ERROR: invalid probe string specified\r\n");
				xil_printf("specify \"none\", \"p1\", \"p2\", \"p3\", \"p4\"\r\n");
				return -1;
			}

			flags.fprobe = 1;
		}

		/* Check for the -voltage option. If this option is specified then
		** then the user wants to set the voltage in one of the
		** VADJ_n_OVERRIDE configuration registers.
		*/
		else if ( 0 == strcmp(arg, "-voltage") ) {
			arg = strtok(NULL, " \r\n");

			if (( NULL == arg ) ||
				( 1 != sscanf(arg, "%hu", &voltageset) )) {
				xil_printf("ERROR: invalid voltage specified\r\n");
				xil_printf("specify a value in millivolts\r\n");
				return -1;
			}

			flags.fsetvoltage = fTrue;
		}

		/* Check for the -? and --help options. These specify whether
		** or not the user wants the help command to be executed.
		*/
		else if (( 0 == strcmp(arg, "-?") ) ||
				 ( 0 == strcmp(arg, "--help") )) {
			/* The help option/command is given priority over all commands.
			** We don't need to do any more parsing because we are going
			** to execute the handler for the help command which doesn't
			** require any options.
			*/
			return 10;
		}

		/* Check for the --verbose option. If this option is specified
		** then the user wishes to enable more detailed error messages.
		*/
//		else if ( 0 == strcmp(arg, "--verbose") ) {
//			fVerbose = fTrue;
//		}


	}
return cmd;
}
