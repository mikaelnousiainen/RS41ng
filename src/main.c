#include "hal/system.h"
#include "hal/i2c.h"
#include "hal/spi.h"
#include "hal/usart_gps.h"
#include "hal/delay.h"
#include "drivers/ubxg6010/ubxg6010.h"
#include "drivers/si4032/si4032.h"
#include "bmp280_handler.h"
#include "si5351_handler.h"
#include "radio.h"
#include "config.h"
#include "log.h"

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

    // Blink fast until GPS fix is acquired
    if (counter % (SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND / 4) == 0)  {
        if (current_gps_data.fix >= 3) {
            if (counter == 0) {
                led_state = !led_state;
                system_set_green_led(led_state);
            }
        } else {
            led_state = !led_state;
            system_set_green_led(led_state);
        }
    }
}

int main(void)
{
    bool success;
    system_handle_timer_tick = handle_timer_tick;
    usart_gps_handle_incoming_byte = ubxg6010_handle_incoming_byte;

    log_info("System init\n");
    system_init();

    system_set_green_led(false);
    system_set_red_led(true);

    log_info("I2C init\n");
    i2c_init();
    log_info("SPI init\n");
    spi_init();

gps_init:
    log_info("GPS init\n");
    success = ubxg6010_init();
    if (!success) {
        log_error("GPS initialization failed, retrying...\n")
        goto gps_init;
    }

    log_info("Si4032 init\n");
    si4032_init();

    if (bmp280_enabled) {
        log_info("BMP280 init\n");
        bmp280_enabled = bmp280_handler_init();
    }

    if (si5351_enabled) {
        log_info("Si5351 init\n");
        si5351_enabled = si5351_handler_init();
    }

    log_info("Radio module init\n");
    radio_init();

    delay_ms(100);

    log_info("System initialized!\n");

    system_set_green_led(true);
    system_set_red_led(false);

    system_initialized = true;

    while (true) {
        radio_handle_main_loop();
        //NVIC_SystemLPConfig(NVIC_LP_SEVONPEND, DISABLE);
        //__WFI();
    }
}
