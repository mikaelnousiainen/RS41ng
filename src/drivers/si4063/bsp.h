/*! @file bsp.h
 * @brief This file contains application specific definitions and includes.
 *
 * @b COPYRIGHT
 * @n Silicon Laboratories Confidential
 * @n Copyright 2012 Silicon Laboratories, Inc.
 * @n http://www.silabs.com
 */

#ifndef BSP_H
#define BSP_H

/*------------------------------------------------------------------------*/
/*            Application specific modificaitons for DFM17                */
/*------------------------------------------------------------------------*/

#include <stdint.h>
// kd2eat - add config.h to pick up any necessary defines
#include <config.h>
#include <gpio.h>
// kd2eat - define SILABS_RADIO_SI446X to enable support for Si4x6x chips.
#define SILABS_RADIO_SI446X
typedef unsigned char bit;
typedef bit BIT;
#define BITS(bitArray, bitPos)  BIT bitArray ## bitPos
#define SI446X_USER_CONFIG_USE_FRR_ABC_FOR_NIRQ


/*------------------------------------------------------------------------*/
/*            Application specific global definitions                     */
/*------------------------------------------------------------------------*/
/*! Platform definition */
/* Note: Plaform is defined in Silabs IDE project file as
 * a command line flag for the compiler. */
//#define SILABS_PLATFORM_WMB930

/*! Extended driver support 
 * Known issues: Some of the example projects 
 * might not build with some extended drivers 
 * due to data memory overflow */
#undef  RADIO_DRIVER_EXTENDED_SUPPORT
#undef  RADIO_DRIVER_FULL_SUPPORT
#undef  SPI_DRIVER_EXTENDED_SUPPORT
#undef  HMI_DRIVER_EXTENDED_SUPPORT
#undef  TIMER_DRIVER_EXTENDED_SUPPORT
#undef  UART_DRIVER_EXTENDED_SUPPORT

/*------------------------------------------------------------------------*/
/*            Application specific includes                               */
/*------------------------------------------------------------------------*/

//#include "drivers\compiler_defs.h"
//#include "platform_defs.h"
//#include "hardware_defs.h"

//#include "application\application_defs.h"

//#include "drivers\cdd_common.h"
//#include "drivers\spi.h"
//#include "drivers\control_IO.h"
//#include "drivers\smbus.h"
//#include "drivers\uart.h"

#if ((defined SILABS_PLATFORM_LCDBB) || (defined SILABS_MCU_DC_EMIF_F930) || (defined SILABS_PLATFORM_WMB))
/* LCD driver includes */
#include "drivers\ascii5x7.h"
#include "drivers\dog_glcd.h"
#include "drivers\pictures.h"
#endif

//#include "application\radio.h"
//#include "application\radio_config.h"

//#include "drivers\radio\radio_hal.h"
#include "radio_comm.h"

#ifdef SILABS_RADIO_SI446X
#include "si446x_api_lib.h"
#include "si446x_defs.h"
#include "si446x_nirq.h"
#include "si446x_patch.h"
#endif

#ifdef SILABS_RADIO_SI4455
#include "drivers\radio\Si4455\si4455_api_lib.h"
#include "drivers\radio\Si4455\si4455_defs.h"
#include "drivers\radio\Si4455\si4455_nirq.h"
#endif

#endif //BSP_H
