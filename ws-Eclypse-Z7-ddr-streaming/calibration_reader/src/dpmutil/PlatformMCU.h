/************************************************************************/
/*                                                                      */
/*  PlatformMCU.h - PlatformMCU definitions and function declarations   */
/*                                                                      */
/************************************************************************/
/*  Author: Michael T. Alexander                                        */
/*  Copyright 2019, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  This header file contains the declarations for functions that can   */
/*  be used to communicate with the Eclypse Platform MCU over the I2C	*/
/*  bus.                                                                */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  08/21/2019 (MichaelA): created                                      */
/*	08/23/2019 (MichaelA): added declaration of PLATFORM_CONFIG			*/
/*                                                                      */
/************************************************************************/

#ifndef PLATFORMMCU_H_
#define PLATFORMMCU_H_

/* ------------------------------------------------------------ */
/*                  Miscellaneous Declarations                  */
/* ------------------------------------------------------------ */

/* Define the I2C address of the Eclypse Platform MCU.
*/
#define addrPlatformMcuI2c	0x60

/* Define the address of the firmware registers.
*/
#define regaddrPDID						0x0000
#define regaddrFirmwareVersion			0x0004
#define regaddrSoftwareReset			0x7FFF

/* Define the address of each configuration register.
*/
#define regaddrReserved1				0x8000
#define regaddrConfigurationVersion     0x8002
#define regaddrPlatformConfig           0x8004
#define regaddrTempProbeCount           0x8006
#define regaddrFanCount                 0x8007
#define regaddr5v0GroupCount            0x8008
#define regaddr3v3GroupCount            0x8009
#define regaddrVadjGroupCount           0x800A
#define regaddrPortCount                0x800B
#define regaddrTemp1Attributes          0x800C
#define regaddrTemp1                    0x800D
#define regaddrTemp2Attributes          0x800F
#define regaddrTemp2                    0x8010
#define regaddrTemp3Attributes          0x8012
#define regaddrTemp3                    0x8013
#define regaddrTemp4Attributes          0x8015
#define regaddrTemp4                    0x8016
#define regaddrFan1Capabilities         0x8018
#define regaddrFan1Config               0x8019
#define regaddrFan1Rpm                  0x801A
#define regaddrFan2Capabilities         0x801C
#define regaddrFan2Config               0x801D
#define regaddrFan2Rpm                  0x801E
#define regaddrFan3Capabilities         0x8020
#define regaddrFan3Config               0x8021
#define regaddrFan3Rpm                  0x8022
#define regaddrFan4Capabilities         0x8024
#define regaddrFan4Config               0x8025
#define regaddrFan4Rpm                  0x8026

#define regaddr5v0ACurrentAllowed       0x8028
#define regaddr5v0ACurrentRequested     0x802A
#define regaddr5v0BCurrentAllowed       0x802C
#define regaddr5v0BCurrentRequested     0x802E
#define regaddr5v0CCurrentAllowed       0x8030
#define regaddr5v0CCurrentRequested     0x8032
#define regaddr5v0DCurrentAllowed       0x8034
#define regaddr5v0DCurrentRequested     0x8036

#define regaddr3v3ACurrentAllowed       0x8038
#define regaddr3v3ACurrentRequested     0x803A
#define regaddr3v3BCurrentAllowed       0x803C
#define regaddr3v3BCurrentRequested     0x803E
#define regaddr3v3CCurrentAllowed       0x8040
#define regaddr3v3CCurrentRequested     0x8042
#define regaddr3v3DCurrentAllowed       0x8044
#define regaddr3v3DCurrentRequested     0x8046

#define regaddrVadjAVoltage             0x8048
#define regaddrVadjAOverride            0x804A
#define regaddrVadjACurrentAllowed      0x804C
#define regaddrVadjACurrentRequested    0x804E

#define regaddrVadjBVoltage             0x8050
#define regaddrVadjBOverride            0x8052
#define regaddrVadjBCurrentAllowed      0x8054
#define regaddrVadjBCurrentRequested    0x8056

#define regaddrVadjCVoltage             0x8058
#define regaddrVadjCOverride            0x805A
#define regaddrVadjCCurrentAllowed      0x805C
#define regaddrVadjCCurrentRequested    0x805E

#define regaddrVadjDVoltage             0x8060
#define regaddrVadjDOverride            0x8062
#define regaddrVadjDCurrentAllowed      0x8064
#define regaddrVadjDCurrentRequested    0x8066

#define regaddrVadjEVoltage             0x8068
#define regaddrVadjEOverride            0x806A
#define regaddrVadjECurrentAllowed      0x806C
#define regaddrVadjECurrentRequested    0x806E

#define regaddrVadjFVoltage             0x8070
#define regaddrVadjFOverride            0x8072
#define regaddrVadjFCurrentAllowed      0x8074
#define regaddrVadjFCurrentRequested    0x8076

#define regaddrVadjGVoltage             0x8078
#define regaddrVadjGOverride            0x807A
#define regaddrVadjGCurrentAllowed      0x807C
#define regaddrVadjGCurrentRequested    0x807E

#define regaddrVadjHVoltage             0x8080
#define regaddrVadjHOverride            0x8082
#define regaddrVadjHCurrentAllowed      0x8084
#define regaddrVadjHCurrentRequested    0x8086

#define regaddrVadjStatus               0x8088

#define regaddrPortAI2cAddress          0x808A
#define regaddrPortA5v0Group            0x808B
#define regaddrPortA3v3Group            0x808C
#define regaddrPortAVioGroup            0x808D
#define regaddrPortAType                0x808E
#define regaddrPortAStatus              0x808F

#define regaddrPortBI2cAddress          0x8090
#define regaddrPortB5v0Group            0x8091
#define regaddrPortB3v3Group            0x8092
#define regaddrPortBVioGroup            0x8093
#define regaddrPortBType                0x8094
#define regaddrPortBStatus              0x8095

#define regaddrPortCI2cAddress          0x8096
#define regaddrPortC5v0Group            0x8097
#define regaddrPortC3v3Group            0x8098
#define regaddrPortCVioGroup            0x8099
#define regaddrPortCType                0x809A
#define regaddrPortCStatus              0x809B

#define regaddrPortDI2cAddress          0x809C
#define regaddrPortD5v0Group            0x809D
#define regaddrPortD3v3Group            0x809E
#define regaddrPortDVioGroup            0x809F
#define regaddrPortDType                0x80A0
#define regaddrPortDStatus              0x80A1

#define regaddrPortEI2cAddress          0x80A2
#define regaddrPortE5v0Group            0x80A3
#define regaddrPortE3v3Group            0x80A4
#define regaddrPortEVioGroup            0x80A5
#define regaddrPortEType                0x80A6
#define regaddrPortEStatus              0x80A7

#define regaddrPortFI2cAddress          0x80A8
#define regaddrPortF5v0Group            0x80A9
#define regaddrPortF3v3Group            0x80AA
#define regaddrPortFVioGroup            0x80AB
#define regaddrPortFType                0x80AC
#define regaddrPortFStatus              0x80AD

#define regaddrPortGI2cAddress          0x80AE
#define regaddrPortG5v0Group            0x80AF
#define regaddrPortG3v3Group            0x80B0
#define regaddrPortGVioGroup            0x80B1
#define regaddrPortGType                0x80B2
#define regaddrPortGStatus              0x80B3

#define regaddrPortHI2cAddress          0x80B4
#define regaddrPortH5v0Group            0x80B5
#define regaddrPortH3v3Group            0x80B6
#define regaddrPortHVioGroup            0x80B7
#define regaddrPortHType                0x80B8
#define regaddrPortHStatus              0x80B9

/* Define the size (in bytes) of each configuration register.
*/
#define cbFirmwareVersion       2
#define cbConfigurationVersion  2
#define cbPlatformConfig        2
#define cbTempProbeCount        1
#define cbFanCount              1
#define cb5v0GroupCount         1
#define cb3v3GroupCount         1
#define cbVadjGroupCount        1
#define cbPortCount             1
#define cbTemp1Attributes       1
#define cbTemp1                 2
#define cbTemp2Attributes       1
#define cbTemp2                 2
#define cbTemp3Attributes       1
#define cbTemp3                 2
#define cbTemp4Attributes       1
#define cbTemp4                 2
#define cbFan1Capabilities      1
#define cbFan1Config            1
#define cbFan1Rpm               2
#define cbFan2Capabilities      1
#define cbFan2Config            1
#define cbFan2Rpm               2
#define cbFan3Capabilities      1
#define cbFan3Config            1
#define cbFan3Rpm               2
#define cbFan4Capabilities      1
#define cbFan4Config            1
#define cbFan4Rpm               2

#define cb5v0ACurrentAllowed    2
#define cb5v0ACurrentRequested  2
#define cb5v0BCurrentAllowed    2
#define cb5v0BCurrentRequested  2
#define cb5v0CCurrentAllowed    2
#define cb5v0CCurrentRequested  2
#define cb5v0DCurrentAllowed    2
#define cb5v0DCurrentRequested  2

#define cb3v3ACurrentAllowed    2
#define cb3v3ACurrentRequested  2
#define cb3v3BCurrentAllowed    2
#define cb3v3BCurrentRequested  2
#define cb3v3CCurrentAllowed    2
#define cb3v3CCurrentRequested  2
#define cb3v3DCurrentAllowed    2
#define cb3v3DCurrentRequested  2

#define cbVadjAVoltage          2
#define cbVadjAOverride         2
#define cbVadjACurrentAllowed   2
#define cbVadjACurrentRequested 2

#define cbVadjBVoltage          2
#define cbVadjBOverride         2
#define cbVadjBCurrentAllowed   2
#define cbVadjBCurrentRequested 2

#define cbVadjCVoltage          2
#define cbVadjCOverride         2
#define cbVadjCCurrentAllowed   2
#define cbVadjCCurrentRequested 2

#define cbVadjDVoltage          2
#define cbVadjDOverride         2
#define cbVadjDCurrentAllowed   2
#define cbVadjDCurrentRequested 2

#define cbVadjEVoltage          2
#define cbVadjEOverride         2
#define cbVadjECurrentAllowed   2
#define cbVadjECurrentRequested 2

#define cbVadjFVoltage          2
#define cbVadjFOverride         2
#define cbVadjFCurrentAllowed   2
#define cbVadjFCurrentRequested 2

#define cbVadjGVoltage          2
#define cbVadjGOverride         2
#define cbVadjGCurrentAllowed   2
#define cbVadjGCurrentRequested 2

#define cbVadjHVoltage          2
#define cbVadjHOverride         2
#define cbVadjHCurrentAllowed   2
#define cbVadjHCurrentRequested 2

#define cbVadjStatus            2

#define cbPortAI2cAddress       1
#define cbPortA5v0Group         1
#define cbPortA3v3Group         1
#define cbPortAVioGroup         1
#define cbPortAType             1
#define cbPortAStatus           1

#define cbPortBI2cAddress       1
#define cbPortB5v0Group         1
#define cbPortB3v3Group         1
#define cbPortBVioGroup         1
#define cbPortBType             1
#define cbPortBStatus           1

#define cbPortCI2cAddress       1
#define cbPortC5v0Group         1
#define cbPortC3v3Group         1
#define cbPortCVioGroup         1
#define cbPortCType             1
#define cbPortCStatus           1

#define cbPortDI2cAddress       1
#define cbPortD5v0Group         1
#define cbPortD3v3Group         1
#define cbPortDVioGroup         1
#define cbPortDType             1
#define cbPortDStatus           1

#define cbPortEI2cAddress       1
#define cbPortE5v0Group         1
#define cbPortE3v3Group         1
#define cbPortEVioGroup         1
#define cbPortEType             1
#define cbPortEStatus           1

#define cbPortFI2cAddress       1
#define cbPortF5v0Group         1
#define cbPortF3v3Group         1
#define cbPortFVioGroup         1
#define cbPortFType             1
#define cbPortFStatus           1

#define cbPortGI2cAddress       1
#define cbPortG5v0Group         1
#define cbPortG3v3Group         1
#define cbPortGVioGroup         1
#define cbPortGType             1
#define cbPortGStatus           1

#define cbPortHI2cAddress       1
#define cbPortH5v0Group         1
#define cbPortH3v3Group         1
#define cbPortHVioGroup         1
#define cbPortHType             1
#define cbPortHStatus           1

/* Define offsets used for reading registers of the same type.
*/
#define offsetPortReg			(regaddrPortBI2cAddress - regaddrPortAI2cAddress)
#define offset5v0Reg			(regaddr5v0BCurrentAllowed - regaddr5v0ACurrentAllowed)
#define offset3v3Reg			(regaddr3v3BCurrentAllowed - regaddr3v3ACurrentAllowed)
#define offsetVadjReg			(regaddrVadjBVoltage - regaddrVadjAVoltage)
#define offsetFanReg            (regaddrFan2Capabilities - regaddrFan1Capabilities)
#define offsetTemperatureReg	(regaddrTemp2Attributes - regaddrTemp1Attributes)

/* Define the different types of SmartVIO ports.
*/
#define ptypeNone		0
#define ptypeSyzygyStd	1
#define ptypeSyzygyTxr2	2
#define ptypeSyzygyTxr4	3

/* Define the configurations that a fan may support.
*/
#define fancfgEnable			1
#define fancfgDisable			0
#define fancfgMinimumSpeed		0
#define fancfgMediumSpeed		1
#define fancfgMaximumSpeed 		2
#define fancfgAutoSpeed			3
#define fancfgTempProbeNone		0
#define fancfgTempProbe1		1
#define fancfgTempProbe2		2
#define fancfgTempProbe3		3
#define fancfgTempProbe4		4

/* Define the attributes that a temperature probe can have.
*/
#define tprobePresent			1
#define tprobeNotPresent		0
#define tlocationFpgaCpu1		0
#define tlocationFpgaCpu2		1
#define tlocationExternal1		2
#define tlocationExternal2		3
#define tformatDegCDecimal		0
#define tformatDegCFixedPoint	1
#define tformatDegFDecimal		2
#define tformatDegFFixedPoint	3

/* ------------------------------------------------------------ */
/*                  General Type Declarations                   */
/* ------------------------------------------------------------ */

#pragma pack(push, 1)

typedef union {
	struct {
		unsigned fEnforce5v0CurLimit:1;
		unsigned fEnforce3v3CurLimit:1;
		unsigned fEnforceVioCurLimit:1;
		unsigned fPerformCrcCheck:1;
		unsigned rsv1:12;
	};
	WORD fsConfig;
} PLATFORM_CONFIG;

typedef union {
	struct {
		unsigned fPresent:1;
		unsigned fDW:1;
		unsigned f5v0InLimit:1;
		unsigned f3v3InLimit:1;
		unsigned fVioInLimit:1;
		unsigned rsv1:1;
		unsigned rsv2:1;
		unsigned fAllowVioEnable:1;
	};
	BYTE fsStatus;
} PmcuPortStatus;

typedef struct {
	union {
		struct {
			unsigned fVadjAEn:1;
			unsigned fVadjBEn:1;
			unsigned fVadjCEn:1;
			unsigned fVadjDEn:1;
			unsigned fVadjEEn:1;
			unsigned fVadjFEn:1;
			unsigned fVadjGEn:1;
			unsigned fVadjHEn:1;
		};
		BYTE fsEn;
	};
	union {
		struct {
			unsigned fVadjAPgood:1;
			unsigned fVadjBPgood:1;
			unsigned fVadjCPgood:1;
			unsigned fVadjDPgood:1;
			unsigned fVadjEPgood:1;
			unsigned fVadjFPgood:1;
			unsigned fVadjGPgood:1;
			unsigned fVadjHPgood:1;
		};
		BYTE fsPgood;
	};
} VADJ_STATUS;

typedef union {
	struct {
		unsigned vltgSet:10;
		unsigned rsv1:4;
		unsigned fEnable:1;
		unsigned fOverride:1;
	};
	WORD fs;
} VADJ_OVERRIDE;

typedef union {
	struct {
		unsigned fcapEnable:1;
		unsigned fcapSetSpeed:1;
		unsigned fcapAutoSpeed:1;
		unsigned fcapMeasureRpm:1;
		unsigned rsv1:4;
	};
	BYTE fs;
} FAN_CAPABILITIES;

typedef union {
	struct {
		unsigned fEnable:1;
		unsigned fspeed:2;
		unsigned tempsrc:3;
		unsigned rsv1:2;
	};
	BYTE fs;
} FAN_CONFIGURATION;

typedef union {
	struct {
		unsigned fPresent:1;
		unsigned tlocation:3;
		unsigned tformat:2;
		unsigned rsv1:2;
	};
	BYTE fs;
} TEMPERATURE_ATTRIBUTES;

#pragma pack(pop)

/* ------------------------------------------------------------ */
/*                  Variable Declarations                       */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*                  Procedure Declarations                      */
/* ------------------------------------------------------------ */

BOOL	PmcuI2cRead(int fdI2cDev, WORD addrRead, BYTE* pbRead, BYTE cbRead, WORD* pcbRead);
BOOL	PmcuI2cWrite(int fdI2cDev, WORD addrWrite, BYTE* pbWrite, BYTE cbWrite, WORD* pcbWritten);

/* ------------------------------------------------------------ */

#endif /* PLATFORMMCU_H_ */
