/**
 * RadSens - universal dosimeter-radiometer module
 *
 * Driver code adapted from https://github.com/climateguard/RadSens
 * Original license GPL 3.0.
 */

#ifndef __RADSENS_H
#define __RADSENS_H

#include <stdint.h>

#include "hal/i2c.h"

#define RS_REG_COUNT 21

// Default RadSens I2C device address
#define RS_DEFAULT_I2C_ADDRESS 0x66

// Device id, default value: 0x7D
// Size: 8 bit
#define RS_REG_DEVICE_ID 0x00

// Firmware version
// Size: 8 bit
#define RS_REG_FIRMWARE_VER 0x01

// Radiation intensity (dynamic period T < 123 sec)
// Size: 24 bit
#define RS_REG_RAD_INTENSITY_DYNAMIC 0x03

// Radiation intensity (static period T = 500 sec)
// Size: 24 bit
#define RS_REG_RAD_INTENSITY_STATIC 0x06

// Contains the accumulated number of pulses registered by the module since the last I2C data reading.
// The value is reset each time it is read. Allows you to process directly the pulses from the Geiger counter
// and implement other algorithms. The value is updated when each pulse is registered.
// Size: 16 bit
#define RS_REG_PULSE_COUNTER 0x09

// This register is used to change the device address when multiple devices need to be connected
// to the same line at the same time. By default, it contains the value 0x66. At the end of recording, the new
// value is stored in the non-volatile memory of the microcontroller.
// Size: 8 bit
// Access: R/W
#define RS_REG_DEVICE_ADDRESS 0x10

// Control register for a high-voltage voltage Converter. By default, it is in the enabled state.
// To enable the HV generator, write 1 to the register, and 0 to disable it. If you try to write other
// values, the command is ignored.
// Size: 8 bit
// Access: R/W
#define RS_REG_HV_GENERATOR 0x11

// Contains the value coefficient used for calculating the radiation intensity.
// If necessary (for example, when installing a different type of counter), the necessary sensitivity value in
// imp/MKR is entered in the register. The default value is 105 imp/MKR. At the end of
// recording, the new value is stored in the non-volatile memory of the microcontroller.
// Size: 16 bit
// Access: R/W
#define RS_REG_SENSITIVITY 0x12

// Control register for an indication diode. By default, it is in the enabled state. To enable the indication,
// write 1 to the register, and 0 to disable it. If you try to write other values, the command is ignored.
// Size: 8 bit
// Access: R/W
#define RS_REG_LED_CONTROL 0x14

class RadSens {
private:
    i2c_port *_port;
    uint8_t _sensor_address;
    uint8_t _chip_id = 0;
    uint8_t _firmware_ver = 0;
    uint32_t _pulse_count = 0;

    bool i2c_read(uint8_t addr, uint8_t *dest, uint8_t num);
    bool i2c_write(uint8_t reg, uint8_t data);

    bool updatePulses();

public:
    RadSens(i2c_port *port, uint8_t sensor_address);

    ~RadSens();

    bool init();
    uint8_t getChipId();
    uint8_t getFirmwareVersion();
    float getRadIntensityDynamic();
    float getRadIntensityStatic();
    int32_t getNumberOfPulses();
    uint8_t getSensorAddress();
    bool getHVGeneratorState();
    bool getLedState();
    int16_t getSensitivity();
    bool setHVGeneratorState(bool state);
    bool setSensitivity(uint16_t sens);
    bool setLedState(bool state);
};

#endif
