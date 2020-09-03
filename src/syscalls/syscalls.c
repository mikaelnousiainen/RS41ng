/**************************************************************************//*****
 * @file     stdio.c
 * @brief    Implementation of newlib syscall
 ********************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "log.h"
#include "semihosting.h"

#undef errno
extern int errno;
extern int  _end;
extern int __HeapLimit;

/*This function is used for handle heap option*/
__attribute__ ((used))
caddr_t _sbrk ( int incr )
{
    static unsigned char *heap = NULL;
    unsigned char *prev_heap;

    if (heap == NULL) {
        heap = (unsigned char *)&_end;
    }
    prev_heap = heap;

    if (heap + incr > (unsigned char *) &__HeapLimit) {
        #ifdef SEMIHOSTING_ENABLE
        extern void abort(void);
        openocd_write(1, 15, "Out of memory!\n");
        abort();
        #else
        errno = ENOMEM;
        return (caddr_t) -1;
        #endif
    }
    // Need to align heap to word boundary, else will get
    // hard faults on Cortex-M0. So we assume that heap starts on
    // word boundary, hence make sure we always add a multiple of
    // 4 to it.
    incr = (incr + 7) & (~7); // align value to 8
    heap += incr;

    return (caddr_t) prev_heap;
}

__attribute__ ((used))
int link(char *old, char *new)
{
    return -1;
}

__attribute__ ((used))
int _close(int file)
{
    return -1;
}

__attribute__ ((used))
int _fstat(int file, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}

__attribute__ ((used))
int _isatty(int file)
{
    return 1;
}

__attribute__ ((used))
int _lseek(int file, int ptr, int dir)
{
    return 0;
}

/*Low layer read(input) function*/
__attribute__ ((used))
int _read(int file, char *ptr, int len)
{

#if 0
     //user code example
     int i;
     (void)file;

     for(i = 0; i < len; i++)
     {
        // UART_GetChar is user's basic input function
        *ptr++ = UART_GetChar();
     }

#endif

    return len;
}


/*Low layer write(output) function*/
__attribute__ ((used))
int _write(int file, char *ptr, int len)
{

    #ifdef SEMIHOSTING_ENABLE
    openocd_write(file, len, ptr);
    #endif

#if 0
     //user code example

     int i;
     (void)file;

     for(i = 0; i < len; i++)
     {
        // UART_PutChar is user's basic output function
        UART_PutChar(*ptr++);
     }
#endif

    return len;
}

__attribute__ ((used))
void abort(void)
{
    /* Abort called */
    while(1);
}

/* --------------------------------- End Of File ------------------------------ */
