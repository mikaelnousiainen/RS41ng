#include "hal/system.h"
#include "hal/spi.h"
#include "hal/usart_gps.h"
#include "hal/usart_ext.h"
#include "hal/delay.h"
#include "hal/datatimer.h"
#include "drivers/ubxg6010/ubxg6010.h"
#include "drivers/pulse_counter/pulse_counter.h"
#include "bmp280_handler.h"
#include "radsens_handler.h"
#include "si5351_handler.h"
#include "radio.h"
#include "config.h"
#include "log.h"

#ifdef RS41
#include "hal/i2c.h"
#include "drivers/si4032/si4032.h"
#endif

#ifdef DFM17
#include "hal/clock_calibration.h"
#include "drivers/si4063/si4063.h"
#endif

uint32_t counter = 0;
bool led_state = true;

gps_data current_gps_data;

void handle_timer_tick()
{
    if (!system_initialized) {
        return;
    }

    radio_handle_timer_tick();

    counter = (counter + 1) % SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND;
    if (counter == 0) {
        ubxg6010_get_current_gps_data(&current_gps_data);
    }

    if (leds_enabled) {
        // Blink fast until GPS fix is acquired
        if (counter % (SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND / 4) == 0) {
            if (GPS_HAS_FIX(current_gps_data)) {
                if (counter == 0) {
                    led_state = !led_state;
                    set_green_led(led_state);
                }
            } else {
                led_state = !led_state;
                set_green_led(led_state);
            }
        }
    }
}

void set_green_led(bool enabled)
{
    if ((LEDS_DISABLE_ALTITUDE_METERS > 0) && (current_gps_data.altitude_mm / 1000 > LEDS_DISABLE_ALTITUDE_METERS)) {
        enabled = false;
    }

    system_set_green_led(enabled);
}

void set_red_led(bool enabled)
{
    if ((LEDS_DISABLE_ALTITUDE_METERS > 0) && (current_gps_data.altitude_mm / 1000 > LEDS_DISABLE_ALTITUDE_METERS)) {
        enabled = false;
    }

    system_set_red_led(enabled);
}

#ifdef DFM17
void set_yellow_led(bool enabled)
{
    if ((LEDS_DISABLE_ALTITUDE_METERS > 0) && (current_gps_data.altitude_mm / 1000 > LEDS_DISABLE_ALTITUDE_METERS)) {
        enabled = false;
    }

    system_set_yellow_led(enabled);
}
#endif

int main(void)
{
    bool success;

    // Set up interrupt handlers
    system_handle_timer_tick = handle_timer_tick;
    system_handle_data_timer_tick = radio_handle_data_timer_tick;
    usart_gps_handle_incoming_byte = ubxg6010_handle_incoming_byte;

    log_info("System init\n");
    system_init();

    set_green_led(false);
    set_red_led(true);

    if (gps_nmea_output_enabled) {
        log_info("External USART init\n");
        usart_ext_init(EXTERNAL_SERIAL_PORT_BAUD_RATE);
    } else if (pulse_counter_enabled) {
        log_info("Pulse counter init\n");
        pulse_counter_init(PULSE_COUNTER_PIN_MODE, PULSE_COUNTER_INTERRUPT_EDGE);
    } else {
#ifdef RS41
        // Only RS41 uses the I2C bus
        log_info("I2C init: clock speed %d kHz\n", I2C_BUS_CLOCK_SPEED / 1000);
        i2c_init(I2C_BUS_CLOCK_SPEED);
#endif
    }

    log_info("SPI init\n");
    spi_init();

    gps_init:
    log_info("GPS init\n");
    success = ubxg6010_init();
    if (!success) {
        log_error("GPS initialization failed, retrying...\n");
        delay_ms(1000);
        goto gps_init;
    }

#ifdef DFM17
    log_info("Timepulse init\n");
    timepulse_init();
#endif

#if defined(RS41)
    log_info("Si4032 init\n");
    si4032_init();
#elif defined(DFM17)
    log_info("Si4063 init\n");
    si4063_init();
#endif

    if (bmp280_enabled) {
        for (int i = 0; i < 3; i++) {
            log_info("BMP280 init\n");
            success = bmp280_handler_init();
            if (success) {
                break;
            }
            log_error("BMP280 init failed, retrying...");
        }
    }

    if (radsens_enabled) {
        for (int i = 0; i < 3; i++) {
            log_info("RadSens init\n");
            success = radsens_handler_init();
            if (success) {
                break;
            }
            log_error("RadSens init failed, retrying...");
        }
    }

    if (si5351_enabled) {
        for (int i = 0; i < 3; i++) {
            log_info("Si5351 init\n");
            success = si5351_handler_init();
            if (success) {
                break;
            }
            log_error("Si5351 init failed, retrying...");
        }
    }

    log_info("Radio module init\n");
    radio_init();

    delay_ms(100);

    log_info("System initialized!\n");

    if (leds_enabled) {
        set_green_led(true);
        set_red_led(false);
    } else {
        set_green_led(false);
        set_red_led(false);
    }

    system_initialized = true;

    while (true) {
        radio_handle_main_loop();
#ifdef DFM17
        clock_calibration_adjust();
#endif
        //NVIC_SystemLPConfig(NVIC_LP_SEVONPEND, DISABLE);
        //__WFI();
    }
}

// The following definition is a workaround to allow compilation and linking to succeed after some recent changes in GCC (at least in Fedora Linux).
// See discussion in: https://github.com/mikaelnousiainen/RS41ng/issues/23
void* __dso_handle;
