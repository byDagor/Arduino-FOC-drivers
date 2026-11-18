#ifndef SIMPLEFOC_DRV8305
#define SIMPLEFOC_DRV8305


#include "Arduino.h"
#include <SPI.h>
#include <drivers/BLDCDriver3PWM.h>

#include "./drv8305_registers.h"


enum class DRV8305_PWMMode {
	PWM6_Mode = 0b00,
	PWM3_Mode = 0b01,
	PWM1_Mode = 0b10
};

enum class DRV8305_DeadTime {
	DeadTime_35ns = 0b000,
	DeadTime_52ns = 0b001,
	DeadTime_88ns = 0b010,
	DeadTime_440ns = 0b011,
	DeadTime_880ns = 0b100,
	DeadTime_1760ns = 0b101,
	DeadTime_3520ns = 0b110,
	DeadTime_5280ns = 0b111
};

enum class DRV8305_Time {  // VDS blanking (TBLANK) and VDS deglitch (TVDS)
	Time_0us = 0b00,
	Time_1us75 = 0b01,
	Time_3us5 = 0b10,
	Time_7us = 0b11
};

enum class DRV8305_GateDriveTime {  // peak gate current time (TDRIVEN, TDRIVEP)
	GateDriveTime_220ns = 0b00,
	GateDriveTime_440ns = 0b01,
	GateDriveTime_880ns = 0b10,
	GateDriveTime_1780ns = 0b11
};

enum class DRV8305_IDRIVEP {  // gate source current
	IDRIVEP_10mA = 0b0000,
	IDRIVEP_20mA = 0b0001,
	IDRIVEP_30mA = 0b0010,
	IDRIVEP_40mA = 0b0011,
	IDRIVEP_50mA = 0b0100,
	IDRIVEP_60mA = 0b0101,
	IDRIVEP_70mA = 0b0110,
	IDRIVEP_125mA = 0b0111,
	IDRIVEP_250mA = 0b1000,
	IDRIVEP_500mA = 0b1001,
	IDRIVEP_750mA = 0b1010,
	IDRIVEP_1000mA = 0b1011
};

enum class DRV8305_IDRIVEN {  // gate sink current
	IDRIVEN_20mA = 0b0000,
	IDRIVEN_30mA = 0b0001,
	IDRIVEN_40mA = 0b0010,
	IDRIVEN_50mA = 0b0011,
	IDRIVEN_60mA = 0b0100,
	IDRIVEN_70mA = 0b0101,
	IDRIVEN_80mA = 0b0110,
	IDRIVEN_250mA = 0b0111,
	IDRIVEN_500mA = 0b1000,
	IDRIVEN_750mA = 0b1001,
	IDRIVEN_1000mA = 0b1010,
	IDRIVEN_1250mA = 0b1011
};

enum class DRV8305_WatchdogDelay {
	WatchdogDelay_10ms = 0b00,
	WatchdogDelay_20ms = 0b01,
	WatchdogDelay_50ms = 0b10,
	WatchdogDelay_100ms = 0b11
};

enum class DRV8305_CSAGain {  // shunt amplifier gain
	CSAGain_10 = 0b00,
	CSAGain_20 = 0b01,
	CSAGain_40 = 0b10,
	CSAGain_80 = 0b11
};

enum class DRV8305_CSABlank {  // shunt amplifier blanking time
	CSABlank_0ns = 0b00,
	CSABlank_500ns = 0b01,
	CSABlank_2us5 = 0b10,
	CSABlank_10us = 0b11
};

enum class DRV8305_VDSMode {
	VDSMode_LatchedShutdown = 0b000,
	VDSMode_ReportOnly = 0b001,
	VDSMode_Disabled = 0b010
};

enum class DRV8305_VDSLevel {  // overcurrent threshold across each MOSFET
	VDSLevel_60mV = 0b00000,
	VDSLevel_68mV = 0b00001,
	VDSLevel_76mV = 0b00010,
	VDSLevel_86mV = 0b00011,
	VDSLevel_97mV = 0b00100,
	VDSLevel_109mV = 0b00101,
	VDSLevel_123mV = 0b00110,
	VDSLevel_138mV = 0b00111,
	VDSLevel_155mV = 0b01000,
	VDSLevel_175mV = 0b01001,
	VDSLevel_197mV = 0b01010,
	VDSLevel_222mV = 0b01011,
	VDSLevel_250mV = 0b01100,
	VDSLevel_282mV = 0b01101,
	VDSLevel_317mV = 0b01110,
	VDSLevel_358mV = 0b01111,
	VDSLevel_403mV = 0b10000,
	VDSLevel_454mV = 0b10001,
	VDSLevel_511mV = 0b10010,
	VDSLevel_576mV = 0b10011,
	VDSLevel_648mV = 0b10100,
	VDSLevel_730mV = 0b10101,
	VDSLevel_822mV = 0b10110,
	VDSLevel_926mV = 0b10111,
	VDSLevel_1043mV = 0b11000,
	VDSLevel_1175mV = 0b11001,
	VDSLevel_1324mV = 0b11010,
	VDSLevel_1491mV = 0b11011,
	VDSLevel_1679mV = 0b11100,
	VDSLevel_1892mV = 0b11101,
	VDSLevel_2131mV = 0b11110
};

// What the nFAULT pin is doing: a warning is a repeating 56 us low / 56 us high pulse train,
// a fault holds the pin low until it is cleared.
enum class DRV8305_FaultPin {
	Idle,
	Warning,
	Fault
};


class DRV8305Warnings {
public:
	DRV8305Warnings(Warning_Watchdog status) : warning_reg(status) {};
	~DRV8305Warnings() {};

	bool isFault() { return warning_reg.FAULT; };
	bool isOverTemperatureWarning() { return warning_reg.OTW; };
	bool isTemperatureFlag105C() { return warning_reg.TEMP_FLAG1; };
	bool isTemperatureFlag125C() { return warning_reg.TEMP_FLAG2; };
	bool isTemperatureFlag135C() { return warning_reg.TEMP_FLAG3; };
	bool isTemperatureFlag175C() { return warning_reg.TEMP_FLAG4; };
	bool isChargePumpUndervoltageWarning() { return warning_reg.VCPH_UVFL; };
	bool isOverCurrentWarning() { return warning_reg.VDS_STATUS; };  // real time, all MOSFETs
	bool isOverVoltageWarning() { return warning_reg.PVDD_OVFL; };
	bool isUnderVoltageWarning() { return warning_reg.PVDD_UVFL; };

	Warning_Watchdog warning_reg;
};


class DRV8305OverCurrent {
public:
	DRV8305OverCurrent(OV_VDS_Faults status) : vds_reg(status) {};
	~DRV8305OverCurrent() {};

	bool isOverCurrent_Ah() { return vds_reg.VDS_HA; };
	bool isOverCurrent_Al() { return vds_reg.VDS_LA; };
	bool isOverCurrent_Bh() { return vds_reg.VDS_HB; };
	bool isOverCurrent_Bl() { return vds_reg.VDS_LB; };
	bool isOverCurrent_Ch() { return vds_reg.VDS_HC; };
	bool isOverCurrent_Cl() { return vds_reg.VDS_LC; };
	bool isSenseOverCurrent_A() { return vds_reg.SNS_A_OCP; };
	bool isSenseOverCurrent_B() { return vds_reg.SNS_B_OCP; };
	bool isSenseOverCurrent_C() { return vds_reg.SNS_C_OCP; };

	OV_VDS_Faults vds_reg;
};


class DRV8305ICFaults {
public:
	DRV8305ICFaults(IC_Faults status) : ic_reg(status) {};
	~DRV8305ICFaults() {};

	bool isUnderVoltage() { return ic_reg.PVDD_UVLO2; };
	bool isChargePumpUnderVoltage() { return ic_reg.VCPH_UVLO2; };
	bool isChargePumpOverVoltage() { return ic_reg.VCPH_OVLO; };
	bool isChargePumpOverVoltageAbsolute() { return ic_reg.VCPH_OVLO_ABS; };
	bool isLowSideGateSupplyUnderVoltage() { return ic_reg.VCP_LSD_UVLO2; };
	bool isAVDDUnderVoltage() { return ic_reg.AVDD_UVLO; };
	bool isVREGUnderVoltage() { return ic_reg.VREG_UV; };
	bool isOverTemperatureShutdown() { return ic_reg.OTSD; };
	bool isWatchdogFault() { return ic_reg.WD_FAULT; };

	IC_Faults ic_reg;
};


class DRV8305GateDriveFaults {
public:
	DRV8305GateDriveFaults(VGS_Faults status) : vgs_reg(status) {};
	~DRV8305GateDriveFaults() {};

	bool isGateDriveFault_Ah() { return vgs_reg.VGS_HA; };
	bool isGateDriveFault_Al() { return vgs_reg.VGS_LA; };
	bool isGateDriveFault_Bh() { return vgs_reg.VGS_HB; };
	bool isGateDriveFault_Bl() { return vgs_reg.VGS_LB; };
	bool isGateDriveFault_Ch() { return vgs_reg.VGS_HC; };
	bool isGateDriveFault_Cl() { return vgs_reg.VGS_LC; };

	VGS_Faults vgs_reg;
};


class DRV8305Status : public DRV8305Warnings, public DRV8305OverCurrent, public DRV8305ICFaults, public DRV8305GateDriveFaults {
	public:
		DRV8305Status(Warning_Watchdog warnings, OV_VDS_Faults vds, IC_Faults ic, VGS_Faults vgs) :
			DRV8305Warnings(warnings), DRV8305OverCurrent(vds), DRV8305ICFaults(ic), DRV8305GateDriveFaults(vgs) {};
		~DRV8305Status() {};
};


#define decl_getset(name, type) type get##name(); void set##name(type);
#define decl_isset(name) bool is##name(); void set##name(bool);

class DRV8305Driver {

	public:
		/**
		 * @param cs     SPI chip select pin (nSCS)
		 * @param enGate EN_GATE pin: high enables the gate driver, low switches all gates off (coast)
		 * @param nFault nFAULT pin (open drain, needs a pull-up), optional
		 */
		DRV8305Driver(int cs, int enGate = NOT_SET, int nFault = NOT_SET, uint32_t spi_clock = 1000000) :
			cs(cs), enGate(enGate), nFault(nFault), spi(&SPI), settings(spi_clock, MSBFIRST, SPI_MODE1) {};
		virtual ~DRV8305Driver() {};

		/** Sets up the pins and the SPI bus. EN_GATE stays low. */
		virtual void init(SPIClass* _spi = &SPI);

		/** Raises EN_GATE and waits for the gate supplies to come up (1 ms). */
		void enableGate();
		/** Pulls EN_GATE low: all gates are high impedance and the motor freewheels. */
		void disableGate();

		/** Reads the four status registers. Reading them also ends a warning pulse train on nFAULT. */
		DRV8305Status getStatus();
		/** Samples nFAULT for longer than one warning period, so call it before getStatus(). */
		DRV8305_FaultPin getFaultPin();
		/** Releases a latched fault once its cause is gone. */
		void clearFault();

		//Gate drive control
		decl_getset(PWMMode, DRV8305_PWMMode);
		decl_getset(DeadTime, DRV8305_DeadTime);
		decl_getset(VDSBlankingTime, DRV8305_Time);
		decl_getset(VDSDeglitchTime, DRV8305_Time);
		decl_isset(ActiveFreewheeling);  // 1-PWM mode only

		//Gate drive strength
		decl_getset(HighSideSourceCurrent, DRV8305_IDRIVEP);
		decl_getset(HighSideSinkCurrent, DRV8305_IDRIVEN);
		decl_getset(HighSideDriveTime, DRV8305_GateDriveTime);
		decl_getset(LowSideSourceCurrent, DRV8305_IDRIVEP);
		decl_getset(LowSideSinkCurrent, DRV8305_IDRIVEN);
		decl_getset(LowSideDriveTime, DRV8305_GateDriveTime);

		//IC operation and protections
		decl_isset(OverTemperatureShutdown);
		decl_isset(SenseAmplifierClamp);
		decl_isset(Watchdog);
		decl_getset(WatchdogDelay, DRV8305_WatchdogDelay);
		decl_isset(SenseOverCurrentDisabled);
		decl_isset(GateDriveFaultDisabled);
		decl_isset(UnderVoltageFaultDisabled);
		decl_isset(ChargePumpLowThreshold);  // 4.6 V instead of 4.9 V

		//VDS overcurrent monitor
		decl_getset(VDSMode, DRV8305_VDSMode);
		decl_getset(VDSLevel, DRV8305_VDSLevel);

		//Shunt amplifiers
		decl_getset(CurrentSenseGain_A, DRV8305_CSAGain);
		decl_getset(CurrentSenseGain_B, DRV8305_CSAGain);
		decl_getset(CurrentSenseGain_C, DRV8305_CSAGain);
		void setCurrentSenseGain(DRV8305_CSAGain gain);  // all three channels
		decl_getset(CurrentSenseBlanking, DRV8305_CSABlank);

	protected:
		uint16_t readSPI(uint8_t addr);
		uint16_t writeSPI(uint8_t addr, uint16_t value);

		int cs;
		int enGate;
		int nFault;
		SPIClass* spi;
		SPISettings settings;
		bool gateOn = false;
};

#undef decl_getset
#undef decl_isset


/**
 * DRV8305 with its three PWM inputs, for use with a SimpleFOC BLDCMotor.
 * INLx are ignored by the DRV8305 in 3-PWM mode, so only INHA/INHB/INHC are driven.
 */
class DRV8305Driver3PWM : public DRV8305Driver, public BLDCDriver3PWM {

	public:
		DRV8305Driver3PWM(int phA, int phB, int phC, int cs, int enGate = NOT_SET, int nFault = NOT_SET, uint32_t spi_clock = 1000000) :
			DRV8305Driver(cs, enGate, nFault, spi_clock), BLDCDriver3PWM(phA, phB, phC) {};
		virtual ~DRV8305Driver3PWM() {};

		/** Configures the DRV8305 for 3-PWM operation and starts the PWM. The gate driver is left off until enable(). */
		virtual void init(SPIClass* _spi = &SPI) override;

		/**
		 * Writes the settings 3-PWM operation needs: 3-PWM mode, over-temperature shutdown on, sense amplifier clamp on, watchdog off.
		 * init() calls it. Call it again if isConfigured() turns false, which happens when the chip lost its supply and reset.
		 * The gate driver keeps the state it had before the call.
		 * @return true if the chip accepted the settings
		 */
		bool configure();
		/** Reads the registers and checks that they still hold the settings written by configure(). */
		bool isConfigured();

		/** Enables the gate driver and starts with zero voltage. */
		void enable() override;
		/** Same as brake(), this is what BLDCMotor::disable() calls. */
		void disable() override;
		/** Zero voltage on all phases: the low-side MOSFETs short the motor, which damps its motion. */
		void brake();
		/** Gate driver off: the motor spins freely. */
		void coast();

};

#endif
