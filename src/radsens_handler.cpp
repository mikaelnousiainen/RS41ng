#include <stdio.h>
#include "drivers/radsens/radsens.h"
#include "radsens_handler.h"
#include "hal/delay.h"
#include "log.h"

RadSens *radsens = NULL;

static bool radsens_initialization_required = true;

static bool radsens_handler_init_sensor();

bool radsens_handler_init()
{
    if (radsens == NULL) {
        radsens = new RadSens(&DEFAULT_I2C_PORT, SENSOR_RADSENS_I2C_ADDRESS);
    }
    return radsens_handler_init_sensor();
}

static bool radsens_handler_init_sensor()
{
    uint16_t sensitivity;

    bool success = radsens->init();
    radsens_initialization_required = !success;
    if (!success) {
        log_error("RadSens init failed\n");
        return false;
    }

    success = radsens->getChipId() == SENSOR_RADSENS_CHIP_ID;
    radsens_initialization_required = !success;
    if (!success) {
        log_error("RadSens invalid chip ID\n");
        return false;
    }

    sensitivity = radsens->getSensitivity();
    success = sensitivity > 0;
    radsens_initialization_required = !success;
    if (!success) {
        log_error("RadSens get sensitivity failed\n");
        return false;
    }
    log_info("RadSens initial sensitivity: %d\n", sensitivity);

    // Give the sensor some time to start up
    delay_ms(50);

    success = radsens->setLedState(true);
    radsens_initialization_required = !success;
    if (!success) {
        log_error("RadSens enable LED failed\n");
        return false;
    }

    delay_ms(200);

#if defined(SENSOR_RADSENS_SENSITIVITY)
    success = radsens->setSensitivity(SENSOR_RADSENS_SENSITIVITY);
    radsens_initialization_required = !success;
    if (!success) {
        log_error("RadSens set sensitivity failed\n");
        return false;
    }

    sensitivity = radsens->getSensitivity();
    success = sensitivity > 0;
    radsens_initialization_required = !success;
    if (!success) {
        log_error("RadSens get sensitivity failed\n");
        return false;
    }
    log_info("RadSens final sensitivity: %d\n", sensitivity);
#endif

    success = radsens->setHVGeneratorState(true);
    radsens_initialization_required = !success;
    if (!success) {
        log_error("RadSens HV generator enable failed\n");
        return false;
    }

    delay_ms(50);

    bool enabled = radsens->getHVGeneratorState();
    radsens_initialization_required = !enabled;
    if (!enabled) {
        log_error("RadSens HV generator is not enabled\n");
        return false;
    }

    delay_ms(50);

    radsens->getNumberOfPulses();

    return success;
}

bool radsens_read(uint16_t *pulse_count, float *dynamic_intensity, float *static_intensity)
{
    float radiation_intensity_static;
    float radiation_intensity_dynamic;
    uint32_t radiation_pulse_counter;

    if (static_intensity) {
        radiation_intensity_static = radsens->getRadIntensityStatic();
        if (radiation_intensity_static < 0) {
            radsens_initialization_required = true;
            log_error("Failed to read RadSens static radiation intensity\n");
            return false;
        }
        *static_intensity = radiation_intensity_static;
    }

    if (dynamic_intensity) {
        radiation_intensity_dynamic = radsens->getRadIntensityDynamic();
        if (radiation_intensity_dynamic < 0) {
            radsens_initialization_required = true;
            log_error("Failed to read RadSens dynamic radiation intensity\n");
            return false;
        }
        *dynamic_intensity = radiation_intensity_dynamic;
    }

    if (pulse_count) {
        radiation_pulse_counter = radsens->getNumberOfPulses();
        if (radiation_pulse_counter < 0) {
            radsens_initialization_required = true;
            log_error("Failed to read RadSens pulse counter\n");
            return false;
        }
        *pulse_count = (uint16_t) (radiation_pulse_counter % 0x10000);
    }

    // log_info("PC: %d RI: %d\n", radiation_pulse_counter, (int) radiation_intensity_dynamic);

    return true;
}

bool radsens_read_telemetry(telemetry_data *data)
{
    bool success;

    if (radsens_initialization_required) {
        log_info("RadSens re-init\n");
        success = radsens_handler_init_sensor();
        log_info("RadSens re-init: %d\n", success);
        if (!success) {
            // Pulse counter does not need to be zeroed
            data->radiation_intensity_uR_h = 0;
            return false;
        }
    }

    success = radsens_read(&data->pulse_count, &data->radiation_intensity_uR_h, NULL);

    if (!success) {
        log_info("RadSens re-init\n");
        success = radsens_handler_init_sensor();
        log_info("RadSens re-init: %d\n", success);

        if (success) {
            success = radsens_read(&data->pulse_count, &data->radiation_intensity_uR_h, NULL);
        } else {
            // Pulse counter does not need to be zeroed
            data->radiation_intensity_uR_h = 0;
        }
    }

    radsens_initialization_required = !success;

    return success;
}
