/**
 * Open loop velocity control of a BLDC motor with a DRV8305 in 3-PWM mode.
 *
 * Open loop applies the voltage limit to the motor at all times, so keep it low: a motor with a
 * low winding resistance can draw a lot of current from even a small voltage.
 *
 * Serial commands: r = run, b = brake, c = coast, f = clear a fault
 */

#include "Arduino.h"
#include <SimpleFOC.h>
#include <SimpleFOCDrivers.h>
#include "drivers/drv8305/drv8305.h"

// Use the right pins and values for your setup!
const int MOTOR_POLE_PAIRS = 7;
const float SUPPLY_VOLTAGE = 12.0f;        // [V]
const float OPEN_LOOP_VOLTAGE = 0.6f;      // [V] kept low on purpose
const float TARGET_VELOCITY = 5.0f;        // [rad/s]
const float VELOCITY_LIMIT = 20.0f;        // [rad/s]
const unsigned long STATUS_PERIOD_MS = 1000;

const int PIN_INH_A = 27;
const int PIN_INH_B = 26;
const int PIN_INH_C = 25;
const int PIN_CS = 5;
const int PIN_EN_GATE = 2;
const int PIN_NFAULT = 4;

// The SPI pins (SCK, MISO, MOSI) are not set here: the driver uses the default SPI bus of your board.
// If your wiring differs, call SPI.begin(sck, miso, mosi) before driver.init() where your board supports it,
// or pass another SPIClass to driver.init(&mySPI).

BLDCMotor motor = BLDCMotor(MOTOR_POLE_PAIRS);
DRV8305Driver3PWM driver = DRV8305Driver3PWM(PIN_INH_A, PIN_INH_B, PIN_INH_C, PIN_CS, PIN_EN_GATE, PIN_NFAULT);

bool running = true;
unsigned long lastStatusMs = 0;


void printStatus() {
	// Sample nFAULT before reading the registers: reading them ends a warning pulse train.
	DRV8305_FaultPin pin = driver.getFaultPin();
	if (pin == DRV8305_FaultPin::Idle) {
		Serial.println("DRV8305: ok");
		return;
	}
	Serial.println(pin == DRV8305_FaultPin::Fault ? "DRV8305: FAULT" : "DRV8305: warning");
	DRV8305Status status = driver.getStatus();
	if (status.isUnderVoltageWarning()) Serial.println("  PVDD undervoltage warning");
	if (status.isOverVoltageWarning()) Serial.println("  PVDD overvoltage warning");
	if (status.isChargePumpUndervoltageWarning()) Serial.println("  Charge pump undervoltage warning");
	if (status.isOverTemperatureWarning()) Serial.println("  Overtemperature warning");
	if (status.isUnderVoltage()) Serial.println("  PVDD undervoltage fault");
	if (status.isOverTemperatureShutdown()) Serial.println("  Overtemperature shutdown");
	if (status.isGateDriveFault_Ah() || status.isGateDriveFault_Al() || status.isGateDriveFault_Bh()
		|| status.isGateDriveFault_Bl() || status.isGateDriveFault_Ch() || status.isGateDriveFault_Cl())
		Serial.println("  Gate drive fault");
	if (status.isOverCurrent_Ah() || status.isOverCurrent_Al() || status.isOverCurrent_Bh()
		|| status.isOverCurrent_Bl() || status.isOverCurrent_Ch() || status.isOverCurrent_Cl())
		Serial.println("  VDS overcurrent fault");
}


void setup() {
	Serial.begin(115200);

	driver.voltage_power_supply = SUPPLY_VOLTAGE;
	driver.init();
	Serial.println(driver.isConfigured() ? "DRV8305: configured" : "DRV8305: not responding");

	motor.linkDriver(&driver);
	motor.controller = MotionControlType::velocity_openloop;
	motor.voltage_limit = OPEN_LOOP_VOLTAGE;
	motor.velocity_limit = VELOCITY_LIMIT;
	motor.init();
}


void loop() {
	if (Serial.available()) {
		switch (Serial.read()) {
			case 'r':
				// after a supply dip the chip resets and forgets its settings
				if (!driver.isConfigured()) {
					Serial.println("DRV8305: lost its configuration, configuring again");
					driver.configure();
				}
				running = true;
				driver.enable();
				break;
			case 'b': running = false; driver.brake(); break;
			case 'c': running = false; driver.coast(); break;
			case 'f': driver.clearFault(); break;
		}
	}

	if (running) {
		motor.loopFOC();  // applies the voltage, also in open loop
		motor.move(TARGET_VELOCITY);
	}

	if (millis() - lastStatusMs >= STATUS_PERIOD_MS) {
		lastStatusMs = millis();
		printStatus();
	}
}
