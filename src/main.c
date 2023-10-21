#include "hal/system.h"
#include "hal/i2c.h"
#include "hal/spi.h"
#include "hal/usart_gps.h"
#include "hal/usart_ext.h"
#include "hal/delay.h"
#include "hal/datatimer.h"
#include "drivers/ubxg6010/ubxg6010.h"
#include "drivers/si4032/si4032.h"
#include "drivers/pulse_counter/pulse_counter.h"
#include "bmp280_handler.h"
#include "radsens_handler.h"
#include "si5351_handler.h"
#include "radio.h"
#include "config.h"
#include "log.h"

uint32_t counter = 0;
bool led_state = true;
uint32_t noFixCounter = 0;
bool inuse_handle_timer_tick = false;
bool led_state2 = true;

gps_data current_gps_data;

void handle_timer_tick()
{
    if (inuse_handle_timer_tick) { return; } 
    else { 
        inuse_handle_timer_tick = true; 
    }
    if (!system_initialized) {
        inuse_handle_timer_tick = false; 
        return;
    }
    radio_handle_timer_tick();
 
    counter = (counter + 1) % SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND;
    if (counter == 0) {
        ubxg6010_get_current_gps_data(&current_gps_data);
        if (GPS_REBOOT_MISSING_GPS_FIX_ENABLE) {
            if (!GPS_HAS_FIX(current_gps_data)) {
                noFixCounter = noFixCounter + 1;
                //led_state2 = !led_state2;
                //set_red_led(!led_state2);
                if (noFixCounter >= GPS_REBOOT_MISSING_GPS_FIX_SECONDS) {
                    noFixCounter = 0;
                    // unproofed: secure: stop interrupt calls
                    //system_handle_timer_tick = NULL;
                    //system_handle_data_timer_tick = NULL;
                    //usart_gps_handle_incoming_byte = NULL;

                    // request cold boot on the STM32 processor
                    inuse_handle_timer_tick = false; 
                    nvic_cold_start();
                    return;
                }
            }
        } else { noFixCounter = 0; }
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
    inuse_handle_timer_tick = false;
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

int main(void)
{
    bool success;
    uint8_t gps_init_fail_counter = 0;
    
    system_initialized = false;

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
        log_info("I2C init: clock speed %d kHz\n", I2C_BUS_CLOCK_SPEED / 1000);
        i2c_init(I2C_BUS_CLOCK_SPEED);
    }

    log_info("SPI init\n");
    spi_init();

    /* ---- not in use now because log_info only work with dongle, so mV is not helpfull to know ------
    // look for Voltage
    // maybe interessting to check if reset only helpfull after mV climbed up > 1500 mV by solar panel source
    uint16_t batteryV = system_get_battery_voltage_millivolts();
    float batteryVf = (float)batteryV / 5.0f;
    uint8_t volts_scaled = (uint8_t)(255 * batteryVf);
    // on debug with dongle the VCC is only 170 mV because board feed by dongle, not on battery.
    log_info("Startup Voltage: %d mV \n",volts_scaled);
    ------------------------------ */
    gps_init:
    log_info("GPS init\n");
    success = ubxg6010_init();
    if (!success) {
        // on use of solar panels at sunrise, the power is not fully available. 
        // So the ubxg6010_init() maybe fail. The fail will be shown as flicker red led two times.
        // Counter increases and after 10 times a system restart will be fired.
        // if system starts new the sunrise meanwhile will give more energy until it is enough for a successfull ubxg6010_init()
        log_error("GPS initialization failed, retrying...\n");
        delay_ms(1000);
        system_flicker_red_led(2);
        gps_init_fail_counter++;
        if (gps_init_fail_counter >= 10) {
            system_flicker_red_led(5);
            // dont think here to stop some interrupts before. In practis the cold start will not work.
            nvic_cold_start();
        }
        goto gps_init;
    }

    log_info("Si4032 init\n");
    si4032_init();

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
//        set_red_led(true); delay_ms(50);set_red_led(false);
        radio_handle_main_loop();
        //NVIC_SystemLPConfig(NVIC_LP_SEVONPEND, DISABLE);
        //__WFI();
    }
}

// The following definition is a workaround to allow compilation and linking to succeed after some recent changes in GCC (at least in Fedora Linux).
// See discussion in: https://github.com/mikaelnousiainen/RS41ng/issues/23
void* __dso_handle;
