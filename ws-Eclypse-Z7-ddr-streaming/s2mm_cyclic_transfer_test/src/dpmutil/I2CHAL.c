/************************************************************************/
/*                                                                      */
/*  I2CHAL.c - I2C Hardware Abstraction Layer functions implementation  */
/*                                                                      */
/************************************************************************/
/*  Author: Michael T. Alexander, Thomas Kappenman            		    */
/*  Copyright 2019, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  This source file contains the implementation of functions that can  */
/*  be used to communicate with the Platform MCU utilizing the I2C bus.	*/
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  08/21/2019 (MichaelA): created                                      */
/*	08/22/2019 (MIchaelA): rewrote PmcuI2cWrite based to work within	*/
/*      the limitations of the PMCU firmware                            */
/*	05/04/2020 (ThomasK): changed to I2CHAL. added baremetal support. 	*/
/*                                                                      */
/************************************************************************/

/* ------------------------------------------------------------ */
/*              Include File Definitions                        */
/* ------------------------------------------------------------ */

#include "../dpmutil/I2CHAL.h"
#if defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
const char  szI2cDeviceName[] = "pmcu-i2c";
const char	szI2cDeviceNameDefault[] = "/dev/i2c-0";
#else
#include "xiicps.h"
#define Iic_Config XIicPs_Config
#define Iic_LookupConfig XIicPs_LookupConfig
#define Iic XIicPs
#define NUMINSTANCES XPAR_XIICPS_NUM_INSTANCES
#define Iic_CfgInitialize XIicPs_CfgInitialize
static Iic IicDev;
static BOOL Iic_Init=fFalse;
#include "sleep.h"
#endif
#include <stdio.h>


/* ------------------------------------------------------------ */
/*              Miscellaneous Declarations                      */
/* ------------------------------------------------------------ */

/*
 * PS I2C Clock Rate
 */
#define IIC_SCLK_RATE 		400000

/* ------------------------------------------------------------ */
/*              Local Type Definitions                          */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Global Variables                                */
/* ------------------------------------------------------------ */

extern BOOL	dpmutilfVerbose;

/* ------------------------------------------------------------ */
/*              Local Variables                                 */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Forward Declarations                            */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Procedure Definitions                           */
/* ------------------------------------------------------------ */

#if defined(__linux__)
/* ------------------------------------------------------------ */
/***    I2CHALOpenI2cController
**
**  Parameters:
**      none
**
**  Return Values:
**      file descriptor for the I2C controller connected to the
**      Platform MCU / SYZYGY I2C bus
**
**  Errors:
**      Any value less than zero should be considered an indication
**      that we failed to open a file descriptor for the I2C controller
**
**  Description:
**      This function opens a file descriptor to the I2C controller
**      that's connected to I2C bus shared by the Platform MCU and
**      SmartVIO ports.
**
**  Notes:
**      It is the callers responsibility to close the file descriptor
**      when he/she is done using it by calling the close() function.
*/
int
I2CHALOpenI2cController() {

	DIR*			pdir;
	struct dirent*	pdirent;
	FILE*			pfile;
	char 			szFilePath[512];
	char			szDevName[cchDeviceNameMax+1];
	int				ch;
	WORD			cchRead;

	pdir = opendir("/sys/bus/i2c/devices/");
	if ( NULL == pdir ) {
		printf("ERROR: opendir failed to open \"/sys/bus/i2c/devices/\"");
		return -1;
	}

	/* Determine the name of the device node that corresponds to the I2C
	** controller that's attached to the I2C bus of the Platform MCU.
	*/
	pdirent = readdir(pdir);
	while ( NULL != pdirent ) {
		/* Skip entries that correspond to current directory or the parent
		** directory.
		*/
		if (( 0 == strcmp(pdirent->d_name, ".")) ||
			( 0 == strcmp(pdirent->d_name, "..") )) {
			pdirent = readdir(pdir);
			continue;
		}

		/* Attempt to open the "device-name" file, if it exists.
		*/
		sprintf(szFilePath, "/sys/bus/i2c/devices/%s/of_node/device-name", pdirent->d_name);

		pfile = fopen(szFilePath, "r");
		if ( NULL == pfile ) {
			pdirent = readdir(pdir);
			continue;
		}

		cchRead = 0;
		ch = fgetc(pfile);
		while (( EOF != ch ) &&
			   ( '\n' != ch ) &&
			   ( cchDeviceNameMax > cchRead )) {
			szDevName[cchRead] = (char)ch;
			cchRead++;
			ch = fgetc(pfile);
		}

		szDevName[cchRead] = '\0';

		fclose(pfile);

		if ( 0 == strcmp(szI2cDeviceName, szDevName) ) {
			break;
		}

		pdirent = readdir(pdir);
	}

	if ( NULL != pdirent ) {
		sprintf(szFilePath, "/dev/%s", pdirent->d_name);
	}
	else {
		strcpy(szFilePath, szI2cDeviceNameDefault);
	}

	closedir(pdir);
	return open(szFilePath, O_RDWR);
}
#else

/* ------------------------------------------------------------ */
/***    I2CHALInit
**
**  Parameters:
**      deviceID			-	deviceID of the I2C device to initialize
**
**  Return Value:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function initializes the I2C device on Zynq and Microblaze
**      devices running baremetal. This can be called before any other
**      dpmutil function to specify a different deviceID besides 0.
**      If not called, the dpmutil functions will initialize the I2C device
**      with deviceID 0 if it exists.
*/
BOOL
I2CHALInit(UINT32 deviceID){
	Iic_Config *pCfgPtr;
	XStatus status;

	if(Iic_Init){
		return fTrue;
	}
	pCfgPtr = Iic_LookupConfig(deviceID);
	if(pCfgPtr==NULL)return fFalse;
	status = Iic_CfgInitialize(&IicDev, pCfgPtr, pCfgPtr->BaseAddress);
	if(status != XST_SUCCESS)return fFalse;

#ifdef PLATFORM_ZYNQ
	XIicPs_SetSClk(&IicDev, IIC_SCLK_RATE);
#endif

	Iic_Init=fTrue;
	return fTrue;

}
#endif

/* ------------------------------------------------------------ */
/***    PmcuI2cRead
**
**  Parameters:
**  	fdI2cDev        - open file descriptor for underlying I2C device (linux only)
**      slaveAddr		- slave address of the device
**      addrRead        - memory address to read
**      pbRead          - pointer to a buffer to receive data
**      cbRead          - number of bytes to read
**      pcbRead         - pointer to variable to receive count of bytes
**                        read
**      uWait			- number of microseconds to wait between reads
**
**  Return Value:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function reads the specified number of bytes from the
**      Platform MCU starting at the specified address. Read operations
**      may be split into multiple transactions with a maximum of
**      cbPmcuTxMax bytes being retrieved during a single read operation.
*/
BOOL
I2CHALRead(int fdI2cDev, BYTE slaveAddr, WORD addrRead, BYTE* pbRead, BYTE cbRead, WORD* pcbRead, UINT32 uWait) {

	ssize_t			cbTrans;
	ssize_t			cb;
	BYTE			cbRecv;
	BYTE			rgbSnd[2];
	char			szErr[64];
	char			szErrDesc[128];

	cbRecv = 0;
	strcpy(szErr, "ERROR: PmcuI2cRead - ");
	szErrDesc[0] = '\0';

	/* Inform the I2C driver of the slave address.
	*/
#if defined(__linux__)
	struct timespec	tsWait;
	if ( 0 > ioctl(fdI2cDev, I2C_SLAVE, slaveAddr) ) {
		sprintf(szErrDesc, "failed to set I2C slave address");
		goto lErrorExit;
	}
	tsWait.tv_sec = 0;
	tsWait.tv_nsec = 50000;
#endif

	while ( cbRecv < cbRead ) {

		/* Transmit the memory address to the slave.
		*/
		rgbSnd[0] = (addrRead  >> 8);
		rgbSnd[1] = addrRead & 0xFF;

#if defined(__linux__)
		if ( 2 != write(fdI2cDev, rgbSnd, 2) ) {
			sprintf(szErrDesc, "failed to write memory address");
			goto lErrorExit;
		}
#elif defined(PLATFORM_ZYNQ)
		// Send the read address
		if(XST_SUCCESS != XIicPs_MasterSendPolled(&IicDev, rgbSnd, 2, slaveAddr)){
			strcpy(szErrDesc, "failed to write memory address");
			goto lErrorExit;
		}
		while (XIicPs_BusIsBusy(&IicDev)) {}
#else
		if(2 != XIic_Send(IicDev.BaseAddress, slaveAddr, rgbSnd, 2, XIIC_STOP)){
			strcpy(szErrDesc, "failed to write memory address");
			goto lErrorExit;
		}
#endif



		cbTrans = cbRead - cbRecv;
		if ( 32 < cbTrans ) {
			cbTrans = 32;
		}

		/* The Linux/Zynq I2C controller places the stop condition on the bus
		** after every call to a read or write function. The I2C controller
		** of the platform MCU requires user code to respond to the stop
		** condition and explicitly re-set the acknowledge bit in the
		** control register before it will ACK another SLA+W or SLA+R
		** request. If immediately call the read function after calling
		** the write function then the MCU firmware may not have enough
		** time to respond to the stop condition and set the acknowledge
		** bit before the start condition and SLA+R is placed on the bus.
		** In such cases the PMCU will NACK the read request. Testing has
		** shown that a minimum of 40us is required in order to guarantee
		** that the PMCU ack's SLA+R and that if we do not add a software
		** delay here the Zynq I2C controller may place the next start
		** condition and SLA+R too soon for the PMCU to respond. Therefore
		** we must delay thread execution before calling the read function.
		*/
		//


#if defined(__linux__)
		nanosleep(&tsWait, NULL);
		cb = read(fdI2cDev, &(pbRead[cbRecv]), cbTrans);
		if ( 0 >= cb ) {
			sprintf(szErrDesc, "read failed after %d bytes", cbRecv);
			goto lErrorExit;
		}
		cbRecv += cb;
		addrRead += cb;
#elif defined(PLATFORM_ZYNQ)
		usleep(uWait);
		// Receive function form the flash
		if(XST_SUCCESS != XIicPs_MasterRecvPolled(&IicDev, &(pbRead[cbRecv]), cbTrans, slaveAddr)){
			sprintf(szErrDesc, "read failed after %d bytes", cbRecv);
			goto lErrorExit;
		}
		while (XIicPs_BusIsBusy(&IicDev)) {}
		cbRecv += cbTrans;
		addrRead += cbTrans;
#else
		usleep(uWait);
		cb = XIic_Recv(IicDev.BaseAddress, slaveAddr, &(pbRead[cbRecv]), cbTrans, XIIC_STOP);
		if(0 >= cb){
			sprintf(szErrDesc, "read failed after %d bytes", cbRecv);
			goto lErrorExit;
		}
		cbRecv += cbTrans;
		addrRead += cbTrans;
#endif

	}

	if ( NULL != pcbRead ) {
		*pcbRead = cbRecv;
	}

	return fTrue;

lErrorExit:

	if ( NULL != pcbRead ) {
		*pcbRead = cbRecv;
	}

	if ( dpmutilfVerbose ) {
		printf("%s%s\n", szErr, szErrDesc);
	}

	return fFalse;
}

/* ------------------------------------------------------------ */
/***    PmcuI2cWrite
**
**  Parameters:
**  	fdI2cDev        - open file descriptor for underlying I2C device (linux only)
**      addrWrite       - memory address to write
**      pbWrite         - pointer to a buffer to containing data to transmit
**      cbWrite         - number of bytes to write
**      pcbWrite        - pointer to variable to receive count of bytes
**                        written
**      uWait			- number of microseconds to wait between writes
**
**  Return Value:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function writes the specified number of bytes to the
**      Platform MCU starting at the specified address. Write operations
**      may be split into multiple transactions with a maximum of
**      cbPmcuRxMax bytes being written during a single write operation.
*/
BOOL
I2CHALWrite(int fdI2cDev, BYTE slaveAddr, WORD addrWrite, BYTE* pbWrite, BYTE cbWrite, INT32 cbDevRxMax, WORD* pcbWritten, INT32 uWait) {

	ssize_t	cb;
	BYTE	ib;
	BYTE	cbTrans;
	BYTE	cbSent;
	BYTE	rgbSnd[32];
	char	szErr[64];
	char	szErrDesc[128];

	cbSent = 0;
	strcpy(szErr, "ERROR: PmcuI2cWrite - ");
	szErrDesc[0] = '\0';

	/* Inform the I2C driver of the slave address.
	*/
#if defined(__linux__)
	struct timespec tsWait;
	if ( 0 > ioctl(fdI2cDev, I2C_SLAVE, slaveAddr) ) {
		sprintf(szErrDesc, "failed to set I2C slave address");
		goto lErrorExit;
	}
#endif

	while ( cbSent < cbWrite ) {
		/* Determine how many bytes to transfer to the Eclypse PMCU for
		** this transaction.
		*/
		cbTrans = 2 + (cbWrite - cbSent);
		if ( cbDevRxMax < cbTrans ) {
			cbTrans = cbDevRxMax;
		}

		/* Populate the buffer with the memory address to be written
		** and any data that's being written to that address during
		** this transfer.
		*/
		rgbSnd[0] = (addrWrite  >> 8);
		rgbSnd[1] = addrWrite & 0xFF;
		for ( ib = 2; ib < cbTrans; ib++ ) {
			rgbSnd[ib] = *pbWrite;
			pbWrite++;
		}

		/* Transmit the memory address and data to the slave.
		*/
#if defined(__linux__)
		cb = write(fdI2cDev, rgbSnd, cbTrans);
		if (cb != cbTrans ) {
			sprintf(szErrDesc, "write failed after %d bytes", cbSent);
			goto lErrorExit;
		}
#elif defined(PLATFORM_ZYNQ)
		// Send the data to the flash
		if(XST_SUCCESS != XIicPs_MasterSendPolled(&IicDev, rgbSnd, cbTrans, slaveAddr)){
			sprintf(szErrDesc, "write failed after %d bytes", cbSent);
			goto lErrorExit;
		}
		while (XIicPs_BusIsBusy(&IicDev)) {}
#else
		cb = XIic_Send(IicDev.BaseAddress, slaveAddr, rgbSnd, cbTrans, XIIC_STOP);
		if (cb != cbTrans ) {
			sprintf(szErrDesc, "write failed after %d bytes", cbSent);
			goto lErrorExit;
		}
#endif


		cbSent += (cbTrans-2);
		addrWrite += (cbTrans-2);

		if ( cbSent < cbWrite ) {
#if defined(__linux__)
			tsWait.tv_sec = 1;
			tsWait.tv_nsec = 0;
			nanosleep(&tsWait, NULL);
#else
			usleep(uWait);
#endif

		}
	}

	if ( NULL != pcbWritten ) {
		*pcbWritten = cbSent;
	}

	return fTrue;

lErrorExit:

	if ( NULL != pcbWritten ) {
		*pcbWritten = cbSent;
	}

	if ( dpmutilfVerbose ) {
		printf("%s%s\n", szErr, szErrDesc);
	}

	return fFalse;
}
