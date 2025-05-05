/************************************************************************/
/*                                                                      */
/*  Zmod.h - function implementations for Zmod identification functions */
/*                                                                      */
/************************************************************************/
/*  Author: Arthur Brown                                                */
/*  Copyright 2024, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  Implements functions used to identify which family a Zmod is a      */
/*  member of                                                           */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  02/21/2024 (ArtVVB): created                                        */
/*                                                                      */
/************************************************************************/

/* ------------------------------------------------------------ */
/*              Include File Definitions                        */
/* ------------------------------------------------------------ */

#if defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <time.h>
#endif
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "stdtypes.h"
#include "../dpmutil/syzygy.h"
#include "../dpmutil/Zmod.h"
#include "../dpmutil/ZmodADC.h"
#include "../dpmutil/ZmodDAC.h"
#include "../dpmutil/ZmodDigitizer.h"


/* ------------------------------------------------------------ */
/*              Miscellaneous Declarations                      */
/* ------------------------------------------------------------ */

/* Define the start address of the Zmod PDID word
*/
#define addrZmodPdidStart	0x80fc

/* ------------------------------------------------------------ */
/*              Local Type Definitions                          */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Global Variables                                */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Local Variables                                 */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Forward Declarations                            */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Procedure Definitions                           */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/***    ZmodReadPdid
**
**  Parameters:
**  	fdI2cDev        - open file descriptor for underlying I2C device (linux only)
**      addrI2cSlave    - I2C bus address for the slave
**      pPdid			- pointer to return product ID information through
**
**  Return Value:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function reads the product ID information from Zmod DNA
*/
BOOL
FZmodReadPdid(int fdI2cDev, BYTE addrI2cSlave, DWORD *pPdid) {
    WORD            cbRead;

    if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrZmodPdidStart, (BYTE*)pPdid, sizeof(DWORD), &cbRead) ) {
        printf("Error: failed to read PDID from 0x%02X\n", addrI2cSlave);
        printf("Error: received %d of %d bytes\n", cbRead, sizeof(ZMOD_DIGITIZER_CAL));
        return fFalse;
    }

    return fTrue;
}

/* ------------------------------------------------------------ */
/***    FGetZmodFamily
**
**  Parameters:
**      Pdid			- product ID, read with FZmodReadPdid
**      pFamily			- Pointer to return the Zmod family by argument
**
**  Return Value:
**      Bool, false if the variant is unsupported, true otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function uses the product ID to identify a Zmod Family
*/
BOOL
FGetZmodFamily(DWORD Pdid, ZMOD_FAMILY *pFamily) {
	if (FZmodIsADC(Pdid)) {
		*pFamily = ZMOD_FAMILY_ADC;
		return fTrue;
	}
	if (FZmodIsDAC(Pdid)) {
		*pFamily = ZMOD_FAMILY_DAC;
		return fTrue;
	}
	if (FZmodIsDigitizer(Pdid)) {
		*pFamily = ZMOD_FAMILY_DIGITIZER;
		return fTrue;
	}
	*pFamily = ZMOD_FAMILY_UNSUPPORTED;
	return fTrue;
}
