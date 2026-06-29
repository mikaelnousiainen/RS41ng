#include "errors.h"

volatile uint8_t system_error_code = ERROR_NONE;

void set_error_code(uint8_t code)
{
    system_error_code = code;
}

void clear_error_code(uint8_t code)
{
    if (system_error_code == code) {
        system_error_code = ERROR_NONE;
    }
}
