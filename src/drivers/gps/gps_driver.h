#ifndef __GPS_DRIVER_H
#define __GPS_DRIVER_H

/**
 * GPS driver abstraction layer.
 *
 * Include this header instead of a specific driver header.
 * Select the driver in config.h by defining one of:
 *   GPS_DRIVER_UBXG6010   - u-blox Gen 6 (original RS41, legacy CFG-* messages)
 *   GPS_DRIVER_UBXM10050  - u-blox Gen 10 (UBX-M10050/M10150, CFG-VALSET)
 *
 * If neither is defined, UBXG6010 is used as the default for backward
 * compatibility with existing RS41 hardware.
 *
 * All drivers expose the same API through these macros:
 *   gps_driver_init()
 *   gps_driver_enable_power_save_mode()
 *   gps_driver_request_gpstime()
 *   gps_driver_get_current_gps_data(data)
 *   gps_driver_handle_incoming_byte(byte)
 *   gps_driver_reset_parser()
 */

#include "config.h"

/* Default to G6010 if nothing specified */
#if !defined(RS41_RSM4x4)
    #define GPS_DRIVER_UBXG6010
#else
    #define GPS_DRIVER_UBXM10050
#endif

/* -------------------------------------------------------------------------
 * u-blox G6010 (Generation 6) driver
 * ------------------------------------------------------------------------- */
#ifdef GPS_DRIVER_UBXG6010

#include "drivers/gps/ubxg6010/ubxg6010.h"

#define gps_driver_init()                       ubxg6010_init()
#define gps_driver_enable_power_save_mode()     ubxg6010_enable_power_save_mode()
#define gps_driver_request_gpstime()            ubxg6010_request_gpstime()
#define gps_driver_get_current_gps_data(data)   ubxg6010_get_current_gps_data(data)
#define gps_driver_handle_incoming_byte         ubxg6010_handle_incoming_byte
#define gps_driver_reset_parser()               ubxg6010_reset_parser()

#endif /* GPS_DRIVER_UBXG6010 */

/* -------------------------------------------------------------------------
 * u-blox M10050 (Generation 10) driver
 * ------------------------------------------------------------------------- */
#ifdef GPS_DRIVER_UBXM10050

#include "drivers/gps/ubxm10050/ubxm10050.h"

#define gps_driver_init()                       ubxm10050_init()
#define gps_driver_enable_power_save_mode()     ubxm10050_enable_power_save_mode()
#define gps_driver_request_gpstime()            ubxm10050_request_gpstime()
#define gps_driver_get_current_gps_data(data)   ubxm10050_get_current_gps_data(data)
#define gps_driver_handle_incoming_byte         ubxm10050_handle_incoming_byte
#define gps_driver_reset_parser()               ubxm10050_reset_parser()

#endif /* GPS_DRIVER_UBXM10050 */

#endif /* __GPS_DRIVER_H */
