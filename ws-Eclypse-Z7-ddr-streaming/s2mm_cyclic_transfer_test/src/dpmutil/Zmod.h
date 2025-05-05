/************************************************************************/
/*                                                                      */
/*  Zmod.h - function declarations for Zmod identification functions    */
/*                                                                      */
/************************************************************************/
/*  Author: Arthur Brown                                                */
/*  Copyright 2024, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  This header file contains declarations for generic Zmod             */
/*  identification functions, that can be used to tell which Zmod       */
/*  family an installed module is part of                               */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  02/21/2024 (ArtVVB): created                                        */
/*                                                                      */
/************************************************************************/

#ifndef ZMOD_H_
#define ZMOD_H_

typedef enum {
	ZMOD_FAMILY_ADC=0,
	ZMOD_FAMILY_DAC,
	ZMOD_FAMILY_DIGITIZER,
	ZMOD_FAMILY_UNSUPPORTED
} ZMOD_FAMILY;

BOOL	FZmodReadPdid(int fdI2cDev, BYTE addrI2cSlave, DWORD *pPdid);
BOOL	FGetZmodFamily(DWORD Pdid, ZMOD_FAMILY *pFamily);

#endif
