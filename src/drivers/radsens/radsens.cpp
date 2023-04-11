/**
 * RadSens - universal dosimeter-radiometer module
 *
 * Driver code adapted from https://github.com/climateguard/RadSens
 * Original license GPL 3.0.
 */

#include "radsens.h"
#include "../../hal/delay.h"

RadSens::RadSens(i2c_port *port, uint8_t sensor_address)
{
    _port = port;
    _sensor_address = sensor_address;
}

RadSens::~RadSens()
{
}

/**
 * Initialization function and sensor connection. Returns false if the sensor is not connected to the I2C bus.
 */
bool RadSens::init()
{
    uint8_t res[2];
    if (!i2c_read(RS_REG_DEVICE_ID, res, 2)) {
        return false;
    }
    _chip_id = res[0];
    _firmware_ver = res[1];
    updatePulses();
    return true;
}

/**
 * Get chip id, default value: 0x7D.
 */
uint8_t RadSens::getChipId()
{
    return _chip_id;
}

/**
 * Get firmware version.
 */
uint8_t RadSens::getFirmwareVersion()
{
    return _firmware_ver;
}

/**
 * Get radiation intensity (dynamic period T < 123 sec).
 */
float RadSens::getRadIntensityDynamic()
{
    // It seems any I²C command will reset the pulse counter, so it must be read first
    updatePulses();
    uint8_t res[3];
    if (!i2c_read(RS_REG_RAD_INTENSITY_DYNAMIC, res, 3)) {
        return -1;
    }
    return (((uint32_t) res[0] << 16) | ((uint16_t) res[1] << 8) | res[2]) / 10.0;
}

/**
 * Get radiation intensity (static period T = 500 sec).
 */
float RadSens::getRadIntensityStatic()
{
    // It seems any I²C command will reset the pulse counter, so it must be read first
    updatePulses();
    uint8_t res[3];
    if (!i2c_read(RS_REG_RAD_INTENSITY_STATIC, res, 3)) {
        return -1;
    }
    return (((uint32_t) res[0] << 16) | ((uint16_t) res[1] << 8) | res[2]) / 10.0;
}

bool RadSens::updatePulses()
{
    uint8_t res[2];
    if (!i2c_read(RS_REG_PULSE_COUNTER, res, 2)) {
        return false;
    }
    _pulse_count += (res[0] << 8) | res[1];
    return true;
}

/**
 * Get the accumulated number of pulses registered by the module since the last I2C data reading.
 */
int32_t RadSens::getNumberOfPulses()
{
    if (!updatePulses()) {
        return -1;
    }
    return _pulse_count;
}

/**
 * Get sensor address.
 */
uint8_t RadSens::getSensorAddress()
{
    uint8_t res;
    if (!i2c_read(RS_REG_DEVICE_ADDRESS, &res, 1)) {
        return 0;
    }
    _sensor_address = res;
    return _sensor_address;
}

/**
 * Get state of high-voltage voltage Converter.
 */
bool RadSens::getHVGeneratorState()
{
    uint8_t res;
    if (!i2c_read(RS_REG_HV_GENERATOR, &res, 1)) {
        return false;
    }
    return res == 1;
}

/**
 * Get the value coefficient used for calculating the radiation intensity.
 */
int16_t RadSens::getSensitivity()
{
    uint8_t res[2];
    if (!i2c_read(RS_REG_SENSITIVITY, res, 2)) {
        return -1;
    }
    return res[1] * 256 + res[0];
}

/**
 * Control register for a high-voltage voltage Converter. By default, it is in the enabled state.
 * To enable the HV generator, write 1 to the register, and 0 to disable it. If you try to write other
 * values, the command is ignored.
 * @param state  true - generator on / false - generator off
 */
bool RadSens::setHVGeneratorState(bool state)
{
    return i2c_write(RS_REG_HV_GENERATOR, state ? 1 : 0);
}

/**
 * Contains the value coefficient used for calculating the radiation intensity.
 * If necessary (for example, when installing a different type of counter), the necessary sensitivity value in
 * Imp / uR is entered in the register. The default value is 105 Imp / uR. At the end of
 * recording, the new value is stored in the non-volatile memory of the microcontroller.
 * @param sens sensitivity coefficient in Impulse / uR
 */
bool RadSens::setSensitivity(uint16_t sens)
{
    if (!i2c_write(RS_REG_SENSITIVITY, (uint8_t)(sens & 0xFF))) {
        return false;
    }
    delay_ms(15);

    if (!i2c_write(RS_REG_SENSITIVITY + 1, (uint8_t)(sens >> 8))) {
        return false;
    }
    delay_ms(15);

    return true;
}

/**
 * Control register for an indication diode. By default, it is in the enabled state. To enable the indication,
 * write 1 to the register, and 0 to disable it. If you try to write other values, the command is ignored.
 * @param state  true - diode on / false - diode off
 */
bool RadSens::setLedState(bool state)
{
    bool result = i2c_write(RS_REG_LED_CONTROL, state ? 1 : 0);
    delay_ms(15);
    return result;
}

/*Get state of led indication.*/
bool RadSens::getLedState()
{
    uint8_t res;
    if (!i2c_read(RS_REG_LED_CONTROL, &res, 1)) {
        return false;
    }
    return res == 1;
}

/**
 * Read block of data
 * @param reg - address of starting register
 * @param dest - destination array
 * @param num - number of bytes to read
 */
bool RadSens::i2c_read(uint8_t reg, uint8_t *dest, uint8_t num)
{
    return i2c_read_bytes(_port, _sensor_address, reg, num, dest) == HAL_OK;
}

/**
 * Write a byte of data
 * @param reg - address of starting register
 * @param data - byte of data to write
 */
bool RadSens::i2c_write(uint8_t reg, uint8_t data)
{
    return i2c_write_byte(_port, _sensor_address, reg, data) == HAL_OK;
}
