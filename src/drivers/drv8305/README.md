# DRV8305 SimpleFOC driver

The DRV8305 is a three-phase gate driver IC from Texas Instruments for external N-channel MOSFETs. It has three current sense amplifiers, a charge pump, an SPI interface for configuration and for reading detailed warning and fault status, and many protections.

This driver includes a DRV8305 SPI driver and a subclass of SimpleFOC's `BLDCDriver3PWM` for 3-PWM operation. The code is designed to be "Arduino compatible", so it should work on any board with SPI.

The 6-PWM and 1-PWM modes of the chip are not supported yet.

## Hardware setup

To use the DRV8305 in 3-PWM mode you will have to connect the following:

- GND
- SPI MOSI (SDI)
- SPI MISO (SDO)
- SPI CLK (SCLK)
- SPI nCS (nSCS)
- INHA, INHB, INHC - connect to motor PWM pins
- EN_GATE - digital out

INLA, INLB and INLC are ignored by the chip in 3-PWM mode (the low side is generated internally, with the programmed dead time). Tie them together to any digital pin or to a fixed level.

Optionally, but recommended:

- nFAULT - digital in, open drain output of the chip, it needs a pull-up resistor to the MCU supply

## Usage

See also the [example](../../../examples/drivers/drv8305/drv8305_3pwm/drv8305_3pwm.ino).

```c++
#include "Arduino.h"
#include <SimpleFOC.h>
#include "SimpleFOCDrivers.h"
#include "drivers/drv8305/drv8305.h"

BLDCMotor motor = BLDCMotor(7);
//                                      INHA INHB INHC nSCS EN_GATE nFAULT
DRV8305Driver3PWM driver = DRV8305Driver3PWM(27, 26, 25, 5, 2, 4);

void setup() {
	driver.voltage_power_supply = 12;
	driver.init();
	motor.linkDriver(&driver);
	motor.init();
	//... normal SimpleFOC init code...
}
```

`init()` sets up the SPI bus on the default `SPI` object of your board (or the `SPIClass*` you pass to it), switches the chip to 3-PWM mode, enables the over-temperature shutdown and the sense amplifier clamp, and switches the watchdog off. `init()` calls `configure()`, which reads the registers back: `driver.isConfigured()` tells you if the chip answered and holds the settings. The gate driver is left off until `enable()` is called, which `BLDCMotor::init()` does.

SimpleFOC 2.4 applies the voltage in `loopFOC()`, also in open loop, so call it before `move()`.

## Supply dips

If the supply of the chip falls below about 4 V, the DRV8305 resets and returns to its default settings, without any message. The gate driver then still switches on and off, but it no longer reads the PWM inputs as 3-PWM. Check before you enable the motor, and configure again if needed:

```c++
if (!driver.isConfigured())     // reads the registers
	driver.configure();           // writes the settings of init() again
driver.enable();
```

`configure()` leaves the gate driver in the state it had.

## Brake and coast

- `disable()` and `brake()` set zero voltage on all phases and leave the gate driver on. The low-side MOSFETs short the motor windings, which damps its motion. `BLDCMotor::disable()` calls `disable()`.
- `coast()` pulls EN_GATE low. All gates become high impedance and the motor spins freely.
- `enable()` raises EN_GATE and waits the 1 ms the gate supplies need to start.

The configuration is kept while EN_GATE is low, and the SPI interface can still be used.

## Warnings and faults

The chip separates warnings from faults:

- A **warning** is report only, the chip takes no action. It is reported on the nFAULT pin as a repeating 56 us low, 56 us high pulse train until a status register is read, and it sets a bit in status register 0x1. Examples: PVDD under- and overvoltage flags, charge pump undervoltage flag, over-temperature warning.
- A **fault** switches all gate outputs off by itself, holds nFAULT low and sets the FAULT bit in register 0x1. It stays latched until its cause is gone and you call `clearFault()`.

```c++
DRV8305_FaultPin pin = driver.getFaultPin();    // Idle, Warning or Fault
if (pin != DRV8305_FaultPin::Idle) {
	DRV8305Status status = driver.getStatus();    // reading the registers ends the warning pulse train
	if (status.isUnderVoltageWarning()) { /* ... */ }
}
if (pin == DRV8305_FaultPin::Fault) {
	// act on it, then when the cause is removed:
	driver.clearFault();
}
```

Call `getFaultPin()` before `getStatus()`: it samples nFAULT for about 160 us, which is longer than one warning period. The driver reports, what to do about a warning or fault is up to your code.

Two exceptions in the chip: the temperature flags (`isTemperatureFlag105C()` etc.) only set a real time status bit and do not touch nFAULT, and with the VDS overcurrent mode set to report only, overcurrent events are warnings instead of faults.

## Settings

All settings are read and written with `get`/`set` pairs, for example:

```c++
driver.setDeadTime(DRV8305_DeadTime::DeadTime_52ns);
driver.setHighSideSourceCurrent(DRV8305_IDRIVEP::IDRIVEP_250mA);
driver.setVDSLevel(DRV8305_VDSLevel::VDSLevel_250mV);
driver.setCurrentSenseGain(DRV8305_CSAGain::CSAGain_40);
```

Call them after `init()`. The chip needs nSCS high for at least 500 ns between SPI frames, the driver takes care of that.

Control registers are cleared on a power up and in sleep mode only, not by `clearFault()` or by pulling EN_GATE low.
