#include "./drv8305.h"

#define DRV8305_READ_BIT 0x8000
#define DRV8305_ADDRESS_SHIFT 11
#define DRV8305_FRAME_GAP_US 1             // nSCS must stay high at least 500 ns between frames
#define DRV8305_GATE_START_MS 1            // time the gate supplies need after EN_GATE rises
#define DRV8305_FAULT_PIN_SAMPLES 16
#define DRV8305_FAULT_PIN_GAP_US 10        // 16 x 10 us spans more than one 112 us warning period


void DRV8305Driver3PWM::init(SPIClass* _spi) {
	DRV8305Driver::init(_spi);
	BLDCDriver3PWM::init();
	configure();
};


bool DRV8305Driver3PWM::configure() {
	bool gateWasOn = gateOn;
	enableGate();
	DRV8305Driver::setPWMMode(DRV8305_PWMMode::PWM3_Mode);
	DRV8305Driver::setOverTemperatureShutdown(true);
	DRV8305Driver::setSenseAmplifierClamp(true);
	DRV8305Driver::setWatchdog(false);
	bool configured = isConfigured();
	if (!gateWasOn)
		disableGate();
	return configured;
};


bool DRV8305Driver3PWM::isConfigured() {
	return getPWMMode() == DRV8305_PWMMode::PWM3_Mode
		&& isOverTemperatureShutdown()
		&& isSenseAmplifierClamp()
		&& !isWatchdog();
};


void DRV8305Driver3PWM::enable() {
	enableGate();
	BLDCDriver3PWM::enable();
};


void DRV8305Driver3PWM::disable() {
	brake();
};


void DRV8305Driver3PWM::brake() {
	enableGate();
	setPwm(0, 0, 0);
};


void DRV8305Driver3PWM::coast() {
	setPwm(0, 0, 0);
	disableGate();
};


void DRV8305Driver::init(SPIClass* _spi) {
	spi = _spi;

	pinMode(cs, OUTPUT);
	digitalWrite(cs, HIGH); // deselect

	if (_isset(enGate)) {
		pinMode(enGate, OUTPUT);
		digitalWrite(enGate, LOW);
		gateOn = false;
	}

	if (_isset(nFault))
		pinMode(nFault, INPUT); // open drain output, needs an external pull-up

	//SPI has an internal SPI-device counter, it is possible to call "begin()" from different devices
	spi->begin();
};


void DRV8305Driver::enableGate() {
	if (!_isset(enGate) || gateOn)
		return;
	digitalWrite(enGate, HIGH);
	delay(DRV8305_GATE_START_MS);
	gateOn = true;
}


void DRV8305Driver::disableGate() {
	if (!_isset(enGate))
		return;
	digitalWrite(enGate, LOW);
	gateOn = false;
}


uint16_t DRV8305Driver::readSPI(uint8_t addr) {
	spi->beginTransaction(settings);
	digitalWrite(cs, LOW);
	// the reply to a read frame carries the register, the first 5 bits are don't care
	uint16_t result = spi->transfer16(DRV8305_READ_BIT | (addr << DRV8305_ADDRESS_SHIFT));
	digitalWrite(cs, HIGH);
	spi->endTransaction();
	delayMicroseconds(DRV8305_FRAME_GAP_US);
	return result & DRV8305_REGISTER_MASK;
}


uint16_t DRV8305Driver::writeSPI(uint8_t addr, uint16_t value) {
	spi->beginTransaction(settings);
	digitalWrite(cs, LOW);
	// the reply to a write frame is the previous content of the register
	uint16_t result = spi->transfer16((addr << DRV8305_ADDRESS_SHIFT) | (value & DRV8305_REGISTER_MASK));
	digitalWrite(cs, HIGH);
	spi->endTransaction();
	delayMicroseconds(DRV8305_FRAME_GAP_US);
	return result & DRV8305_REGISTER_MASK;
}


#define def_getset(name, type, address, register_type, register_name) \
type DRV8305Driver::get##name() {\
	register_type data;\
	data.reg = readSPI(address);\
	return static_cast<type>(data.register_name);\
}\
\
void DRV8305Driver::set##name(type value) {\
	register_type data;\
	data.reg = readSPI(address);\
	data.register_name = static_cast<uint16_t>(value);\
	writeSPI(address, data.reg);\
}

#define def_isset(name, address, register_type, register_name) \
bool DRV8305Driver::is##name() {\
	register_type data;\
	data.reg = readSPI(address);\
	return data.register_name;\
}\
\
void DRV8305Driver::set##name(bool value) {\
	register_type data;\
	data.reg = readSPI(address);\
	data.register_name = value;\
	writeSPI(address, data.reg);\
}


DRV8305Status DRV8305Driver::getStatus() {
	Warning_Watchdog warnings;
	OV_VDS_Faults vds;
	IC_Faults ic;
	VGS_Faults vgs;
	warnings.reg = readSPI(Warning_Watchdog_ADDR);
	vds.reg = readSPI(OV_VDS_Faults_ADDR);
	ic.reg = readSPI(IC_Faults_ADDR);
	vgs.reg = readSPI(VGS_Faults_ADDR);
	return DRV8305Status(warnings, vds, ic, vgs);
}


DRV8305_FaultPin DRV8305Driver::getFaultPin() {
	if (!_isset(nFault))
		return DRV8305_FaultPin::Idle;
	bool sawLow = false;
	bool sawHigh = false;
	for (int i = 0; i < DRV8305_FAULT_PIN_SAMPLES; i++) {
		if (digitalRead(nFault) == LOW)
			sawLow = true;
		else
			sawHigh = true;
		delayMicroseconds(DRV8305_FAULT_PIN_GAP_US);
	}
	if (!sawLow)
		return DRV8305_FaultPin::Idle;
	return sawHigh ? DRV8305_FaultPin::Warning : DRV8305_FaultPin::Fault;
}


void DRV8305Driver::clearFault() {
	IC_Operation data;
	data.reg = readSPI(IC_Operation_ADDR);
	data.CLR_FLTS = 1;
	writeSPI(IC_Operation_ADDR, data.reg);
}


def_getset(PWMMode, DRV8305_PWMMode, Gate_Drive_Control_ADDR, Gate_Drive_Control, PWM_MODE)
def_getset(DeadTime, DRV8305_DeadTime, Gate_Drive_Control_ADDR, Gate_Drive_Control, DEAD_TIME)
def_getset(VDSBlankingTime, DRV8305_Time, Gate_Drive_Control_ADDR, Gate_Drive_Control, TBLANK)
def_getset(VDSDeglitchTime, DRV8305_Time, Gate_Drive_Control_ADDR, Gate_Drive_Control, TVDS)
def_isset(ActiveFreewheeling, Gate_Drive_Control_ADDR, Gate_Drive_Control, COMM_OPTION)

def_getset(HighSideSourceCurrent, DRV8305_IDRIVEP, HS_Gate_Drive_ADDR, HS_Gate_Drive, IDRIVEP_HS)
def_getset(HighSideSinkCurrent, DRV8305_IDRIVEN, HS_Gate_Drive_ADDR, HS_Gate_Drive, IDRIVEN_HS)
def_getset(HighSideDriveTime, DRV8305_GateDriveTime, HS_Gate_Drive_ADDR, HS_Gate_Drive, TDRIVEN)
def_getset(LowSideSourceCurrent, DRV8305_IDRIVEP, LS_Gate_Drive_ADDR, LS_Gate_Drive, IDRIVEP_LS)
def_getset(LowSideSinkCurrent, DRV8305_IDRIVEN, LS_Gate_Drive_ADDR, LS_Gate_Drive, IDRIVEN_LS)
def_getset(LowSideDriveTime, DRV8305_GateDriveTime, LS_Gate_Drive_ADDR, LS_Gate_Drive, TDRIVEP)

def_isset(OverTemperatureShutdown, IC_Operation_ADDR, IC_Operation, FLIP_OTSD)
def_isset(SenseAmplifierClamp, IC_Operation_ADDR, IC_Operation, EN_SNS_CLAMP)
def_isset(Watchdog, IC_Operation_ADDR, IC_Operation, WD_EN)
def_getset(WatchdogDelay, DRV8305_WatchdogDelay, IC_Operation_ADDR, IC_Operation, WD_DLY)
def_isset(SenseOverCurrentDisabled, IC_Operation_ADDR, IC_Operation, DIS_SNS_OCP)
def_isset(GateDriveFaultDisabled, IC_Operation_ADDR, IC_Operation, DIS_GDRV_FAULT)
def_isset(UnderVoltageFaultDisabled, IC_Operation_ADDR, IC_Operation, DIS_PVDD_UVLO2)
def_isset(ChargePumpLowThreshold, IC_Operation_ADDR, IC_Operation, SET_VCPH_UV)

def_getset(VDSMode, DRV8305_VDSMode, VDS_Sense_ADDR, VDS_Sense, VDS_MODE)
def_getset(VDSLevel, DRV8305_VDSLevel, VDS_Sense_ADDR, VDS_Sense, VDS_LEVEL)

def_getset(CurrentSenseGain_A, DRV8305_CSAGain, Shunt_Amplifier_ADDR, Shunt_Amplifier, GAIN_CS1)
def_getset(CurrentSenseGain_B, DRV8305_CSAGain, Shunt_Amplifier_ADDR, Shunt_Amplifier, GAIN_CS2)
def_getset(CurrentSenseGain_C, DRV8305_CSAGain, Shunt_Amplifier_ADDR, Shunt_Amplifier, GAIN_CS3)
def_getset(CurrentSenseBlanking, DRV8305_CSABlank, Shunt_Amplifier_ADDR, Shunt_Amplifier, CS_BLANK)


void DRV8305Driver::setCurrentSenseGain(DRV8305_CSAGain gain) {
	Shunt_Amplifier data;
	data.reg = readSPI(Shunt_Amplifier_ADDR);
	data.GAIN_CS1 = static_cast<uint16_t>(gain);
	data.GAIN_CS2 = static_cast<uint16_t>(gain);
	data.GAIN_CS3 = static_cast<uint16_t>(gain);
	writeSPI(Shunt_Amplifier_ADDR, data.reg);
}


void DRV8305Driver::setCurrentSenseCalibration(bool enable) {
	Shunt_Amplifier data;
	data.reg = readSPI(Shunt_Amplifier_ADDR);
	data.DC_CAL_CH1 = enable;
	data.DC_CAL_CH2 = enable;
	data.DC_CAL_CH3 = enable;
	writeSPI(Shunt_Amplifier_ADDR, data.reg);
}
