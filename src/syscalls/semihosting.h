#ifndef __SEMIHOSTING_H
#define __SEMIHOSTING_H

void openocd_send_command(int command, void *message);
void openocd_write(int fd, int length, char *data);
void openocd_print_string(char *string);
void openocd_print_char(char c);

#endif
