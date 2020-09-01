#include <stdint.h>

#define OPENOCD_SYS_WRITEC 0x03
#define OPENOCD_SYS_WRITE0 0x04
#define OPENOCD_SYS_WRITE  0x05

/**
 * TODO:
 * I've used the following code to check for a connected debugger in the past with an STM32F4xx series MCU
 * (when the only choice was the StdPeriph library -- perhaps this has changed with HAL/LL, but the hardware
 * register and corresponding bit is obviously the same):
 * if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
 * {
 *     // Debugger is connected
 * }
 */

void openocd_send_command(int command, void *message)
{

    asm("mov r0, %[cmd];"
        "mov r1, %[msg];"
        "bkpt #0xAB"
    :
    : [cmd] "r"(command), [msg] "r"(message)
    : "r0", "r1", "memory");
}

void openocd_write(int fd, int length, char *data)
{
    uint32_t message[] = {fd, (uint32_t) data, length};
    openocd_send_command(OPENOCD_SYS_WRITE, message);
}

void openocd_print_string(char *string)
{
    uint32_t message[] = {(uint32_t) string};
    openocd_send_command(OPENOCD_SYS_WRITE0, message);
}

void openocd_print_char(char c)
{
    uint32_t message[] = {(uint32_t) &c};
    openocd_send_command(OPENOCD_SYS_WRITEC, message);
}
