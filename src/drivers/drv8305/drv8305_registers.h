#pragma once

#include <stdint.h>

// DRV8305 register map, TI datasheet SLVSCX2B section 7.6.
// Every register is 11 bits wide (D10..D0). Bit fields below are listed from D0 upwards.

#define Warning_Watchdog_ADDR   0x1
#define OV_VDS_Faults_ADDR      0x2
#define IC_Faults_ADDR          0x3
#define VGS_Faults_ADDR         0x4
#define HS_Gate_Drive_ADDR      0x5
#define LS_Gate_Drive_ADDR      0x6
#define Gate_Drive_Control_ADDR 0x7
#define IC_Operation_ADDR       0x9
#define Shunt_Amplifier_ADDR    0xA
#define Voltage_Regulator_ADDR  0xB
#define VDS_Sense_ADDR          0xC

#define DRV8305_REGISTER_MASK   0x07FF


// ---- Status registers (read only) ----

typedef union {
	struct {
		uint16_t OTW:1;          // overtemperature warning
		uint16_t TEMP_FLAG3:1;   // junction above about 135 degC
		uint16_t TEMP_FLAG2:1;   // junction above about 125 degC
		uint16_t TEMP_FLAG1:1;   // junction above about 105 degC
		uint16_t VCPH_UVFL:1;    // charge pump undervoltage warning
		uint16_t VDS_STATUS:1;   // real time OR of all VDS overcurrent monitors
		uint16_t PVDD_OVFL:1;    // PVDD overvoltage warning
		uint16_t PVDD_UVFL:1;    // PVDD undervoltage warning
		uint16_t TEMP_FLAG4:1;   // junction above about 175 degC
		uint16_t :1;
		uint16_t FAULT:1;        // a latched fault is present, nFAULT is held low
	};
	uint16_t reg;
} Warning_Watchdog;

typedef union {
	struct {
		uint16_t SNS_A_OCP:1;
		uint16_t SNS_B_OCP:1;
		uint16_t SNS_C_OCP:1;
		uint16_t :2;
		uint16_t VDS_LC:1;
		uint16_t VDS_HC:1;
		uint16_t VDS_LB:1;
		uint16_t VDS_HB:1;
		uint16_t VDS_LA:1;
		uint16_t VDS_HA:1;
	};
	uint16_t reg;
} OV_VDS_Faults;

typedef union {
	struct {
		uint16_t VCPH_OVLO_ABS:1;
		uint16_t VCPH_OVLO:1;
		uint16_t VCPH_UVLO2:1;
		uint16_t :1;
		uint16_t VCP_LSD_UVLO2:1;
		uint16_t AVDD_UVLO:1;
		uint16_t VREG_UV:1;
		uint16_t :1;
		uint16_t OTSD:1;
		uint16_t WD_FAULT:1;
		uint16_t PVDD_UVLO2:1;
	};
	uint16_t reg;
} IC_Faults;

typedef union {
	struct {
		uint16_t :5;
		uint16_t VGS_LC:1;
		uint16_t VGS_HC:1;
		uint16_t VGS_LB:1;
		uint16_t VGS_HB:1;
		uint16_t VGS_LA:1;
		uint16_t VGS_HA:1;
	};
	uint16_t reg;
} VGS_Faults;


// ---- Control registers (read / write) ----

typedef union {
	struct {
		uint16_t IDRIVEP_HS:4;
		uint16_t IDRIVEN_HS:4;
		uint16_t TDRIVEN:2;
		uint16_t :1;
	};
	uint16_t reg;
} HS_Gate_Drive;

typedef union {
	struct {
		uint16_t IDRIVEP_LS:4;
		uint16_t IDRIVEN_LS:4;
		uint16_t TDRIVEP:2;
		uint16_t :1;
	};
	uint16_t reg;
} LS_Gate_Drive;

typedef union {
	struct {
		uint16_t TVDS:2;
		uint16_t TBLANK:2;
		uint16_t DEAD_TIME:3;
		uint16_t PWM_MODE:2;
		uint16_t COMM_OPTION:1;  // only used in 1-PWM mode
		uint16_t :1;
	};
	uint16_t reg;
} Gate_Drive_Control;

typedef union {
	struct {
		uint16_t SET_VCPH_UV:1;
		uint16_t CLR_FLTS:1;     // self clearing
		uint16_t SLEEP:1;
		uint16_t WD_EN:1;
		uint16_t DIS_SNS_OCP:1;
		uint16_t WD_DLY:2;
		uint16_t EN_SNS_CLAMP:1;
		uint16_t DIS_GDRV_FAULT:1;
		uint16_t DIS_PVDD_UVLO2:1;
		uint16_t FLIP_OTSD:1;    // 1 = overtemperature shutdown enabled
	};
	uint16_t reg;
} IC_Operation;

typedef union {
	struct {
		uint16_t GAIN_CS1:2;
		uint16_t GAIN_CS2:2;
		uint16_t GAIN_CS3:2;
		uint16_t CS_BLANK:2;
		uint16_t DC_CAL_CH1:1;
		uint16_t DC_CAL_CH2:1;
		uint16_t DC_CAL_CH3:1;
	};
	uint16_t reg;
} Shunt_Amplifier;

typedef union {
	struct {
		uint16_t VREG_UV_LEVEL:2;
		uint16_t DIS_VREG_PWRGD:1;
		uint16_t SLEEP_DLY:2;
		uint16_t :3;
		uint16_t VREF_SCALE:2;
		uint16_t :1;
	};
	uint16_t reg;
} Voltage_Regulator;

typedef union {
	struct {
		uint16_t VDS_MODE:3;
		uint16_t VDS_LEVEL:5;
		uint16_t :3;
	};
	uint16_t reg;
} VDS_Sense;
