#include "drivers/hal/system.h"
#include "drivers/hal/i2c.h"
#include "drivers/hal/spi.h"
#include "drivers/hal/usart_gps.h"
#include "drivers/hal/usart_ext.h"
#include "drivers/hal/delay.h"
#include "drivers/hal/datatimer.h"
#include "drivers/gps/gps_driver.h"
#include "drivers/pulse_counter/pulse_counter.h"
#include "bmp280_handler.h"
#include "bme68x_handler.h"
#include "bme690_handler.h"
#include "radsens_handler.h"
#include "si5351_handler.h"
#include "radio.h"
#include "config.h"
#include "log.h"

#ifdef RS41
#include "drivers/si4032/si4032.h"
#endif

#ifdef DFM17
#include "drivers/hal/clock_calibration.h"
#include "drivers/si4063/si4063.h"
#endif

uint32_t counter = 0;
bool led_state = true;
uint32_t red_led_strobe_ticks = 0;
bool red_led_strobe_state = false;

gps_data current_gps_data;

void handle_timer_tick()
{
    if(counter % 100 == 0)
        usart_gps_drain_dma();

    if (!system_initialized) {		// Timer may pop before everything fully initialized
        return;
    }

    radio_handle_timer_tick();

#if ALLOW_POWER_OFF
    if(counter % 10 == 0)
        system_handle_button();
#endif

    counter = (counter + 1) % SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND;
    if (counter == 0) {
        gps_driver_get_current_gps_data(&current_gps_data);
    }

#if LEDS_ENABLE && !ENABLE_FOX_MODE
    // Green LED: solid on when GPS acquired, 2Hz blink when no fix (suppressed during red strobe)
    if (counter % (SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND / 4) == 0) {
        if (red_led_strobe_ticks > 0) {
            set_green_led(false);
        } else if (GPS_HAS_FIX(current_gps_data)) {
            set_green_led(true);
        } else {
            led_state = !led_state;
            set_green_led(led_state);
        }
    }

    // Red LED: strobe at 2Hz for 5 seconds on error
    if (red_led_strobe_ticks > 0) {
        if (counter % (SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND / 4) == 0) {
            red_led_strobe_state = !red_led_strobe_state;
            system_set_red_led(red_led_strobe_state);
        }
        red_led_strobe_ticks--;
        if (red_led_strobe_ticks == 0) {
            red_led_strobe_state = false;
            system_set_red_led(false);
        }
    }
#endif
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

#if LEDS_ENABLE && !ENABLE_FOX_MODE
    if (enabled) {
        red_led_strobe_ticks = 5 * SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND;
        red_led_strobe_state = false;
        return;
    } else {
        red_led_strobe_ticks = 0;
        red_led_strobe_state = false;
    }
#endif

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
    bool success __attribute__((unused));

    // Set up interrupt handlers
    system_handle_timer_tick = handle_timer_tick;
    system_handle_data_timer_tick = radio_handle_data_timer_tick;
    usart_gps_handle_incoming_byte = gps_driver_handle_incoming_byte;

    //log_info("System init\n");
    system_init();

    set_green_led(true);
    set_red_led(false);

#if GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE 
    log_info("External USART init\n");
    usart_ext_init(EXTERNAL_SERIAL_PORT_BAUD_RATE);
#elif PULSE_COUNTER_ENABLE
    log_info("Pulse counter init\n");
    pulse_counter_init(PULSE_COUNTER_PIN_MODE, PULSE_COUNTER_INTERRUPT_EDGE);
#else
    #if SENSOR_BMP280_ENABLE || SENSOR_BME68X_ENABLE || SENSOR_BME690_ENABLE || SENSOR_RADSENS_ENABLE || RADIO_SI5351_ENABLE
        i2c_init();
    #endif
#endif

    //log_info("SPI init\n");
    spi_init();

#if defined(RS41)
    //log_info("Si4032 init\n");
    si4032_init();
#elif defined(DFM17)
    // Init Si4063 early: powers up the radio and enables 12.8 MHz TCXO clock output
    // on GPIO2/PD0 (OSC_IN), then switch SYSCLK from HSI to HSE bypass.
    log_info("Si4063 init\n");
    si4063_init();

    log_info("Switching to HSE PLL (12.8 MHz input from Si4063)\n");
    system_switch_to_hse_bypass();
#endif

#if !ENABLE_FOX_MODE
        gps_init:
        log_info("GPS init\n");
        success = gps_driver_init();
        if (!success) {
            set_green_led(false);
            set_red_led(true);
            log_error("GPS initialization failed, retrying...\n");
            delay_ms(1000);
            goto gps_init;
        }
        set_red_led(false);

    #ifdef DFM17
        log_info("Timepulse init\n");
        timepulse_init();
    #endif
#else
        // Fox mode: init USART at GPS default baud rate and immediately deep sleep
        log_info("GPS deep sleep (fox mode)\n");
        gps_driver_init_and_sleep();
#endif

#if SENSOR_BMP280_ENABLE
    for (int i = 0; i < 3; i++) {
        log_info("BMP280 init\n");
        success = bmp280_handler_init();
        if (success) {
            break;
        }
        log_error("BMP280 init failed, retrying...\n");
    }
#endif

#if SENSOR_BME68X_ENABLE
    for (int i = 0; i < 3; i++) {
        log_info("BME68X init\n");
        success = bme68x_handler_init();
        if (success) {
            break;
        }
        log_error("BME68X init failed, retrying...\n");
    }
#endif

#if SENSOR_BME690_ENABLE
    for (int i = 0; i < 3; i++) {
        log_info("BME690 init\n");
        success = bme690_handler_init();
        if (success) {
            break;
        }
        log_error("BME690 init failed, retrying...\n");
    }
#endif

#if SENSOR_RADSENS_ENABLE
    for (int i = 0; i < 3; i++) {
        log_info("RadSens init\n");
        success = radsens_handler_init();
        if (success) {
            break;
        }
        log_error("RadSens init failed, retrying...\n");
    }
#endif

#if RADIO_SI5351_ENABLE
    for (int i = 0; i < 3; i++) {
        log_info("Si5351 init\n");
        success = si5351_handler_init();
        if (success) {
            break;
        }
        log_error("Si5351 init failed, retrying...\n");
    }
#endif 

    //log_info("Radio module init\n");
    radio_init();

    delay_ms(100);

    log_info("System initialized!\n");

#if LEDS_ENABLE
    #if ENABLE_FOX_MODE
        set_green_led(false);
        set_red_led(false);
    #else
        set_green_led(true);
        set_red_led(false);
        #ifdef DFM17
            set_yellow_led(true);
        #endif
    #endif
#else
    set_green_led(false);
    set_red_led(false);
#endif

    system_initialized = true;

    while (true) {
        usart_gps_drain_dma();
        radio_handle_main_loop();
        //NVIC_SystemLPConfig(NVIC_LP_SEVONPEND, DISABLE);
        //__WFI();
    }
}

// The following definition is a workaround to allow compilation and linking to succeed after some recent changes in GCC (at least in Fedora Linux).
// See discussion in: https://github.com/mikaelnousiainen/RS41ng/issues/23
void* __dso_handle;
