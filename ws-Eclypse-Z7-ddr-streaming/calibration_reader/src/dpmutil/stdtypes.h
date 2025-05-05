/************************************************************************/
/*                                                                      */
/*  stdtypes.h	--  Digilent Standard Type Declarations                 */
/*                                                                      */
/************************************************************************/
/*  Author: Gene Apperson                                               */
/*  Copyright 2005, Digilent Inc.                                       */
/************************************************************************/
/*  File Description:                                                   */
/*                                                                      */
/* This header file contains declarations for standard Digilent data	*/
/* types and constants for use with avr-gcc.                            */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  01/04/2005(GeneA): created                                          */
/*  08/19/2019(MichaelA): modified to support Petalinux                 */
/*                                                                      */
/************************************************************************/

#if !defined(_STDTYPES_INC)
#define _STDTYPES_INC

#include <stdint.h>

/* ------------------------------------------------------------ */
/*                  General Type Declarations                   */
/* ------------------------------------------------------------ */

#if defined(__cplusplus)
    const bool fFalse = false;
    const bool fTrue = true;
#else
    #define	fFalse  0
    #define	fTrue   (!fFalse)
#endif

typedef signed short    INT16;
typedef unsigned short  UINT16;
typedef int32_t         INT32;
typedef uint32_t        UINT32;
typedef int64_t         INT64;
typedef uint64_t        UINT64;
typedef	unsigned char   BYTE;
typedef	unsigned short  WORD;
typedef UINT32          DWORD;
typedef	unsigned long   ULONG;
typedef unsigned short  USHORT;
typedef int             BOOL;

typedef char            CHAR;
typedef unsigned char   UCHAR;
typedef CHAR*           PCHAR;
typedef short           SHORT;

typedef DWORD           HANDLE;

typedef char            TCHAR;

/* ------------------------------------------------------------ */

#endif

/************************************************************************/
