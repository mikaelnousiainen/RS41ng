/**
 * Driver for u-blox M10 generation GPS chips (UBX-M10050, UBX-M10150).
 * Protocol version 34 (SPG 5.10 firmware).
 *
 * Key differences from the ubxg6010 (Gen6) driver:
 *   - Configuration via CFG-VALSET (0x06 0x8A) with 32-bit key IDs.
 *     Legacy CFG-PRT / CFG-MSG / CFG-NAV5 / CFG-RXM are not available.
 *   - Multiple config items can be batched into a single CFG-VALSET packet.
 *   - Power save via CFG-PM-OPERATEMODE set to PSMCT (cyclic tracking).
 *   - NAV-PVT (0x01 0x07) provides position + velocity + time in one message.
 *   - UBX frame format and checksum algorithm are unchanged.
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "drivers/hal/system.h"
#include "drivers/hal/usart_gps.h"
#include "drivers/hal/delay.h"
#if GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE
#include "drivers/hal/usart_ext.h"
#endif

#include "ubxm10050.h"
#include "log.h"
#include "config.h"

/* -------------------------------------------------------------------------
 * Protocol constants
 * ------------------------------------------------------------------------- */

#define UBX_SYNC1               0xB5
#define UBX_SYNC2               0x62

/* Message classes */
#define UBX_CLASS_NAV           0x01
#define UBX_CLASS_RXM           0x02
#define UBX_CLASS_CFG           0x06
#define UBX_CLASS_MON           0x0A
#define UBX_CLASS_ACK           0x05

/* Message IDs */
#define UBX_NAV_PVT             0x07    /* Position, Velocity, Time */
#define UBX_NAV_TIMEGPS         0x20    /* GPS Time */
#define UBX_NAV_TIMEUTC         0x21    /* UTC Time */
#define UBX_RXM_PMREQ           0x41    /* Power management request */
#define UBX_CFG_VALSET          0x8A    /* Set configuration items */
#define UBX_CFG_RST             0x04    /* Reset receiver */
#define UBX_ACK_ACK             0x01
#define UBX_ACK_NAK             0x00

/* CFG-VALSET layers bitmask */
#define VALSET_LAYER_RAM        0x01    /* Volatile RAM - lost on power cycle */
#define VALSET_LAYER_BBR        0x02    /* Battery-backed RAM */
#define VALSET_LAYER_FLASH      0x04    /* Non-volatile flash */
#define VALSET_LAYER_ALL        0x07

/* CFG-VALSET key IDs - M10 generation
 * Format: 0xSSGGIIII where SS=size, GG=group, IIII=item
 * Size: 0x10=bit, 0x20=U1, 0x30=U2, 0x40=U4, 0x50=U8
 */
#define CFG_UART1_BAUDRATE      0x40520001UL  /* U4 - UART1 baud rate */
#define CFG_UART1_STOPBITS      0x20520002UL  /* U1 - Stop bits */
#define CFG_UART1_DATABITS      0x20520003UL  /* U1 - Data bits */
#define CFG_UART1_PARITY        0x20520004UL  /* U1 - Parity */
#define CFG_UART1_ENABLED       0x10520005UL  /* L  - Enable UART1 */
#define CFG_UART1INPROT_UBX     0x10730001UL  /* L  - UBX input on UART1 */
#define CFG_UART1INPROT_NMEA    0x10730002UL  /* L  - NMEA input on UART1 */
#define CFG_UART1OUTPROT_UBX    0x10740001UL  /* L  - UBX output on UART1 */
#define CFG_UART1OUTPROT_NMEA   0x10740002UL  /* L  - NMEA output on UART1 */

/* CFG-MSGOUT keys - output rate of messages on UART1, U1 */
#define CFG_MSGOUT_NAV_PVT_UART1     0x20910007UL  /* NAV-PVT on UART1 */
#define CFG_MSGOUT_NAV_TIMEGPS_UART1 0x20910048UL  /* NAV-TIMEGPS on UART1 */
#define CFG_MSGOUT_NAV_TIMEUTC_UART1 0x2091005BUL  /* NAV-TIMEUTC on UART1 */
#define CFG_MSGOUT_NAV_STATUS_UART1  0x2091001BUL  /* NAV-STATUS on UART1 */

/* CFG-MSGOUT keys - NMEA standard sentences on UART1, U1 */
#define CFG_MSGOUT_NMEA_GGA_UART1    0x209100BBUL  /* NMEA-GGA on UART1 */
#define CFG_MSGOUT_NMEA_GLL_UART1    0x209100CAUL  /* NMEA-GLL on UART1 */
#define CFG_MSGOUT_NMEA_GSA_UART1    0x209100C0UL  /* NMEA-GSA on UART1 */
#define CFG_MSGOUT_NMEA_GSV_UART1    0x209100C5UL  /* NMEA-GSV on UART1 */
#define CFG_MSGOUT_NMEA_RMC_UART1    0x209100ACUL  /* NMEA-RMC on UART1 */
#define CFG_MSGOUT_NMEA_VTG_UART1    0x209100B1UL  /* NMEA-VTG on UART1 */

/* CFG-NAVSPG keys */
#define CFG_NAVSPG_DYNMODEL     0x20110021UL  /* U1 - Dynamic model */

/* CFG-PM keys - Power Management (group 0xD0)
 * Key format: (size << 28) | (group << 16) | item
 */
#define CFG_PM_OPERATEMODE      0x20D00001UL  /* E1 - Operating mode: 0=full, 1=PSMOO, 2=PSMCT */

/* Dynamic model values */
#define DYNMODEL_PORTABLE       0
#define DYNMODEL_STATIONARY     2
#define DYNMODEL_PEDESTRIAN     3
#define DYNMODEL_AUTOMOTIVE     4
#define DYNMODEL_SEA            5
#define DYNMODEL_AIR1G          6   /* Airborne < 1g - use for balloon */
#define DYNMODEL_AIR2G          7   /* Airborne < 2g */
#define DYNMODEL_AIR4G          8   /* Airborne < 4g */

/* ACK timeout in ms */
#define UBX_ACK_TIMEOUT_MS      1000

/* -------------------------------------------------------------------------
 * Packet structures
 * ------------------------------------------------------------------------- */

typedef struct __attribute__((packed)) {
    uint8_t  sync1;         /* 0xB5 */
    uint8_t  sync2;         /* 0x62 */
    uint8_t  msgClass;
    uint8_t  msgId;
    uint16_t payloadLen;
} UbxHeader;

typedef struct __attribute__((packed)) {
    uint8_t ck_a;
    uint8_t ck_b;
} UbxChecksum;

/* CFG-RST payload */
typedef struct __attribute__((packed)) {
    uint16_t navBbrMask;    /* BBR sections to clear, 0xFFFF = cold start */
    uint8_t  resetMode;     /* 0x01 = controlled SW reset */
    uint8_t  reserved;
} UbxCfgRstPayload;

/* CFG-VALSET header (before key-value pairs) */
typedef struct __attribute__((packed)) {
    uint8_t version;        /* 0x00 */
    uint8_t layers;         /* bitmask: RAM=1, BBR=2, Flash=4 */
    uint8_t reserved[2];
} UbxCfgValsetHeader;

/* NAV-PVT payload (92 bytes, protocol v27+) */
typedef struct __attribute__((packed)) {
    uint32_t iTOW;          /* GPS time of week [ms] */
    uint16_t year;          /* Year (UTC) */
    uint8_t  month;         /* Month 1..12 */
    uint8_t  day;           /* Day 1..31 */
    uint8_t  hour;          /* Hour 0..23 */
    uint8_t  min;           /* Minute 0..59 */
    uint8_t  sec;           /* Second 0..60 */
    uint8_t  valid;         /* Validity flags: bit0=validDate, bit1=validTime, bit2=fullyResolved */
    uint32_t tAcc;          /* Time accuracy [ns] */
    int32_t  nano;          /* Fraction of second [-1e9..1e9 ns] */
    uint8_t  fixType;       /* 0=no fix, 2=2D, 3=3D, 4=GNSS+DR, 5=time only */
    uint8_t  flags;         /* Fix flags: bit0=gnssFixOK */
    uint8_t  flags2;
    uint8_t  numSV;         /* Satellites used */
    int32_t  lon;           /* Longitude [1e-7 deg] */
    int32_t  lat;           /* Latitude [1e-7 deg] */
    int32_t  height;        /* Height above ellipsoid [mm] */
    int32_t  hMSL;          /* Height above MSL [mm] */
    uint32_t hAcc;          /* Horizontal accuracy [mm] */
    uint32_t vAcc;          /* Vertical accuracy [mm] */
    int32_t  velN;          /* NED north velocity [mm/s] */
    int32_t  velE;          /* NED east velocity [mm/s] */
    int32_t  velD;          /* NED down velocity [mm/s] */
    int32_t  gSpeed;        /* Ground speed [mm/s] */
    int32_t  headMot;       /* Heading of motion [1e-5 deg] */
    uint32_t sAcc;          /* Speed accuracy [mm/s] */
    uint32_t headAcc;       /* Heading accuracy [1e-5 deg] */
    uint16_t pDOP;          /* Position DOP [0.01] */
    uint8_t  flags3;
    uint8_t  reserved1[5];
    int32_t  headVeh;       /* Heading of vehicle [1e-5 deg] */
    int16_t  magDec;        /* Magnetic declination [1e-2 deg] */
    uint16_t magAcc;        /* Magnetic declination accuracy [1e-2 deg] */
} UbxNavPvtPayload;

/* NAV-TIMEGPS payload */
typedef struct __attribute__((packed)) {
    uint32_t iTOW;
    int32_t  fTOW;
    int16_t  week;
    int8_t   leapS;
    uint8_t  valid;         /* bit2 = leapS valid */
    uint32_t tAcc;
} UbxNavTimeGpsPayload;

/* RXM-PMREQ payload (M10 version - 16 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t  version;       /* 0x00 */
    uint8_t  reserved1[3];
    uint32_t duration;      /* Duration [ms], 0 = indefinite */
    uint32_t flags;         /* bit1=backup mode */
    uint32_t wakeupSources; /* bit3=UART1 wakeup */
} UbxRxmPmreqPayload;

/* -------------------------------------------------------------------------
 * Module state
 * ------------------------------------------------------------------------- */

static gps_data m10_current_gps_data;
static bool     m10_initialized = false;
static volatile bool ack_received  = false;
static volatile bool nack_received = false;

/* Parser state */
static uint8_t  parse_sync     = 0;
static uint8_t  parse_buf[256 + sizeof(UbxHeader) + sizeof(UbxChecksum)];
static uint16_t parse_pos      = 0;

#if GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE
/* NMEA forwarding state:
 *   0 = idle, waiting for '$'
 *   1 = got '$', waiting for 'G'
 *   2 = got '$G', waiting for talker ID letter
 *   3 = forwarding sentence body until \n
 */
static volatile uint8_t nmea_sync = 0;
#endif

/* Raw-byte capture: filled by ubxm10050_handle_incoming_byte before
 * the parser runs, so we can log exactly what the GPS chip sends us. */
#define RAW_CAPTURE_SIZE 80
static uint8_t  raw_capture_buf[RAW_CAPTURE_SIZE];
static volatile uint16_t raw_capture_pos = 0;

/* Count of NAV-PVT packets received (reset on init) */
static uint16_t pvt_packet_count = 0;

/* -------------------------------------------------------------------------
 * Checksum
 * ------------------------------------------------------------------------- */

static UbxChecksum ubxm10050_checksum(uint8_t msgClass, uint8_t msgId,
                                       const uint8_t *payload, uint16_t len)
{
    UbxChecksum cs = {0, 0};
    cs.ck_a += msgClass;  cs.ck_b += cs.ck_a;
    cs.ck_a += msgId;     cs.ck_b += cs.ck_a;
    cs.ck_a += (uint8_t)(len & 0xFF);        cs.ck_b += cs.ck_a;
    cs.ck_a += (uint8_t)((len >> 8) & 0xFF); cs.ck_b += cs.ck_a;
    for (uint16_t i = 0; i < len; i++) {
        cs.ck_a += payload[i];
        cs.ck_b += cs.ck_a;
    }
    return cs;
}

/* -------------------------------------------------------------------------
 * Send raw UBX packet
 * ------------------------------------------------------------------------- */

static void ubxm10050_send_raw(uint8_t msgClass, uint8_t msgId,
                                const uint8_t *payload, uint16_t payloadLen)
{
    UbxChecksum cs = ubxm10050_checksum(msgClass, msgId, payload, payloadLen);

    usart_gps_send_byte(UBX_SYNC1);
    usart_gps_send_byte(UBX_SYNC2);
    usart_gps_send_byte(msgClass);
    usart_gps_send_byte(msgId);
    usart_gps_send_byte((uint8_t)(payloadLen & 0xFF));
    usart_gps_send_byte((uint8_t)((payloadLen >> 8) & 0xFF));
    for (uint16_t i = 0; i < payloadLen; i++) {
        usart_gps_send_byte(payload[i]);
    }
    usart_gps_send_byte(cs.ck_a);
    usart_gps_send_byte(cs.ck_b);
}

/* -------------------------------------------------------------------------
 * Wait for ACK/NAK with timeout
 * ------------------------------------------------------------------------- */

static bool ubxm10050_wait_for_ack(void)
{
    uint32_t start = HAL_GetTick();
    ack_received  = false;
    nack_received = false;

    while ((HAL_GetTick() - start) < UBX_ACK_TIMEOUT_MS) {
        usart_gps_drain_dma();
        if (ack_received)  return true;
        if (nack_received) return false;
    }

    log_info("GPS M10: ACK timeout (gps_ints=%lu ok=%u bad=%u)\n",
             gps_ints,
             m10_current_gps_data.ok_packets,
             m10_current_gps_data.bad_packets);
    return false;
}

/* -------------------------------------------------------------------------
 * CFG-VALSET helpers
 *
 * The M10 uses a key-value store. A CFG-VALSET packet carries:
 *   [version:1][layers:1][reserved:2][key0:4][val0:N]...[keyN:4][valN:M]
 *
 * Key size is encoded in bits [31:28] of the key ID:
 *   0x1 = 1 bit (bool)
 *   0x2 = 1 byte (U1/E1)
 *   0x3 = 2 bytes (U2)
 *   0x4 = 4 bytes (U4)
 *   0x5 = 8 bytes (U8)
 *
 * We build packets on the stack and send them directly.
 * ------------------------------------------------------------------------- */

/* One key+value pair: key(4) + U1 value(1) = 5 bytes */
typedef struct __attribute__((packed)) {
    uint32_t key;
    uint8_t  val;
} ValsetU1;

/* One key+value pair: key(4) + U4 value(4) = 8 bytes */
typedef struct __attribute__((packed)) {
    uint32_t key;
    uint32_t val;
} ValsetU4;

/* Send a single CFG-VALSET with multiple U1 items */
static bool ubxm10050_valset_u1_multi(uint8_t layers,
                                       const ValsetU1 *items, uint8_t count)
{
    /* Max ~20 items fits comfortably in 128 bytes */
    uint8_t buf[4 + count * sizeof(ValsetU1)];

    buf[0] = 0x00;   /* version */
    buf[1] = layers;
    buf[2] = 0x00;   /* reserved */
    buf[3] = 0x00;

    for (uint8_t i = 0; i < count; i++) {
        uint16_t off = 4 + i * sizeof(ValsetU1);
        buf[off + 0] = (uint8_t)(items[i].key & 0xFF);
        buf[off + 1] = (uint8_t)((items[i].key >> 8) & 0xFF);
        buf[off + 2] = (uint8_t)((items[i].key >> 16) & 0xFF);
        buf[off + 3] = (uint8_t)((items[i].key >> 24) & 0xFF);
        buf[off + 4] = items[i].val;
    }

    /* Drain any pending bytes and reset parser to avoid stale state
     * from NMEA or partial UBX packets consuming our ACK sync bytes. */
    usart_gps_drain_dma();
    parse_sync = 0;
    parse_pos  = 0;

    ubxm10050_send_raw(UBX_CLASS_CFG, UBX_CFG_VALSET, buf, sizeof(buf));
    return ubxm10050_wait_for_ack();
}

/* Send a single CFG-VALSET with one U4 item */
static bool ubxm10050_valset_u4(uint8_t layers, uint32_t key, uint32_t val)
{
    uint8_t buf[4 + sizeof(ValsetU4)];

    buf[0] = 0x00; buf[1] = layers; buf[2] = 0x00; buf[3] = 0x00;

    /* key little-endian */
    buf[4] = (uint8_t)(key & 0xFF);
    buf[5] = (uint8_t)((key >> 8) & 0xFF);
    buf[6] = (uint8_t)((key >> 16) & 0xFF);
    buf[7] = (uint8_t)((key >> 24) & 0xFF);

    /* value little-endian */
    buf[8]  = (uint8_t)(val & 0xFF);
    buf[9]  = (uint8_t)((val >> 8) & 0xFF);
    buf[10] = (uint8_t)((val >> 16) & 0xFF);
    buf[11] = (uint8_t)((val >> 24) & 0xFF);

    /* Drain any pending bytes and reset parser to avoid stale state
     * from NMEA or partial UBX packets consuming our ACK sync bytes. */
    usart_gps_drain_dma();
    parse_sync = 0;
    parse_pos  = 0;

    ubxm10050_send_raw(UBX_CLASS_CFG, UBX_CFG_VALSET, buf, sizeof(buf));
    return ubxm10050_wait_for_ack();
}

/* -------------------------------------------------------------------------
 * Cold reset
 * ------------------------------------------------------------------------- */

static void ubxm10050_cold_reset(void)
{
    UbxCfgRstPayload rst = {
        .navBbrMask = 0xFFFF,   /* Clear all BBR */
        .resetMode  = 0x02,     /* Controlled SW reset (GNSS + host) */
        .reserved   = 0x00,
    };
    ubxm10050_send_raw(UBX_CLASS_CFG, UBX_CFG_RST,
                       (const uint8_t *)&rst, sizeof(rst));
    /* No ACK expected for reset */
}

/* -------------------------------------------------------------------------
 * Debug helpers
 * ------------------------------------------------------------------------- */

__attribute__((unused)) static const char *fix_type_str(uint8_t fix)
{
    switch (fix) {
        case 0:  return "NONE";
        case 1:  return "DR";
        case 2:  return "2D";
        case 3:  return "3D";
        case 4:  return "GNSS+DR";
        case 5:  return "TIME";
        default: return "???";
    }
}

/* -------------------------------------------------------------------------
 * Packet handler (called from parser when a complete valid packet arrives)
 * ------------------------------------------------------------------------- */

static void ubxm10050_handle_packet(uint8_t msgClass, uint8_t msgId,
                                     const uint8_t *payload, uint16_t len)
{
    if (msgClass == UBX_CLASS_ACK) {
        if (msgId == UBX_ACK_ACK)  { ack_received  = true; return; }
        if (msgId == UBX_ACK_NAK)  { nack_received = true; return; }
    }

    if (msgClass == UBX_CLASS_NAV && msgId == UBX_NAV_PVT) {
        if (len < sizeof(UbxNavPvtPayload)) return;
        const UbxNavPvtPayload *pvt = (const UbxNavPvtPayload *)payload;

        m10_current_gps_data.ok_packets++;
        pvt_packet_count++;
        m10_current_gps_data.time_of_week_millis          = pvt->iTOW;
        m10_current_gps_data.year                         = pvt->year;
        m10_current_gps_data.month                        = pvt->month;
        m10_current_gps_data.day                          = pvt->day;
        m10_current_gps_data.hours                        = pvt->hour;
        m10_current_gps_data.minutes                      = pvt->min;
        m10_current_gps_data.seconds                      = pvt->sec;
        m10_current_gps_data.fix                          = pvt->fixType;
        m10_current_gps_data.fix_ok                       = (pvt->flags & 0x01) != 0;
        m10_current_gps_data.satellites_visible           = pvt->numSV;
        m10_current_gps_data.time_valid_flags             = pvt->valid;
        m10_current_gps_data.time_accuracy_ns             = pvt->tAcc;
        m10_current_gps_data.latitude_degrees_10000000     = pvt->lat;
        m10_current_gps_data.longitude_degrees_10000000    = pvt->lon;
        m10_current_gps_data.altitude_mm                  = pvt->hMSL;
        m10_current_gps_data.ground_speed_cm_per_second   = pvt->gSpeed / 10;
        m10_current_gps_data.heading_degrees_100000       = pvt->headMot;
        m10_current_gps_data.climb_cm_per_second          = -(pvt->velD / 10);
        m10_current_gps_data.position_dilution_of_precision = pvt->pDOP;
        /* PSM state from flags bits [4:2] — values match gps.h constants directly */
        m10_current_gps_data.power_safe_mode_state = (pvt->flags >> 2) & 0x07;

        m10_current_gps_data.updated                      = true;

        /* Debug: log every PVT packet so we can see communication, fix, time,
         * and satellite status in real time.
         *   valid bits: bit0=date valid  bit1=time valid  bit2=fully resolved
         *   fix types:  0=none 2=2D 3=3D 4=GNSS+DR 5=time-only
         */
        // log_info("GPS PVT #%u: fix=%s fixOK=%d sats=%u "
        //          "%04u-%02u-%02u %02u:%02u:%02u valid=0x%02X tAcc=%luus "
        //          "ok=%u bad=%u\n",
        //          pvt_packet_count,
        //          fix_type_str(pvt->fixType), (pvt->flags & 0x01),
        //          pvt->numSV,
        //          pvt->year, pvt->month, pvt->day,
        //          pvt->hour, pvt->min, pvt->sec,
        //          pvt->valid,
        //          (unsigned long)(pvt->tAcc / 1000),  /* ns -> us for readability */
        //          m10_current_gps_data.ok_packets,
        //          m10_current_gps_data.bad_packets);
        return;
    }

    if (msgClass == UBX_CLASS_NAV && msgId == UBX_NAV_TIMEGPS) {
        if (len < sizeof(UbxNavTimeGpsPayload)) return;
        const UbxNavTimeGpsPayload *t = (const UbxNavTimeGpsPayload *)payload;

        m10_current_gps_data.time_of_week_millis = t->iTOW;
        m10_current_gps_data.week                = t->week;
        if (t->valid & 0x04) {
            m10_current_gps_data.leap_seconds    = t->leapS;
        }
        m10_current_gps_data.updated = true;

        /* Debug: log GPS time info - leapS valid is bit2 of valid byte.
         * When leapS becomes valid, UTC = GPS_time - leapS is accurate. */
        // log_info("GPS TIMEGPS: iTOW=%lu week=%d leapS=%d leapValid=%d tAcc=%luns\n",
        //          (unsigned long)t->iTOW, t->week, t->leapS,
        //          (t->valid & 0x04) ? 1 : 0,
        //          (unsigned long)t->tAcc);
        return;
    }

    /* Silently ignore other messages (MON-VER etc.) */
    (void)len;
}

/* -------------------------------------------------------------------------
 * Byte-level parser
 *
 * State machine:
 *   parse_sync == 0: waiting for 0xB5
 *   parse_sync == 1: waiting for 0x62
 *   parse_sync == 2: accumulating bytes into parse_buf
 *
 * parse_buf layout: [class][id][lenL][lenH][payload...][ck_a][ck_b]
 * parse_pos counts bytes stored after the two sync bytes.
 * ------------------------------------------------------------------------- */

void ubxm10050_reset_parser(void)
{
    parse_sync = 0;
    parse_pos  = 0;
#if GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE
    nmea_sync  = 0;
#endif
}

#if GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE
static void ubxm10050_handle_nmea_sentence_start(uint8_t data)
{
    if (nmea_sync == 0 && data == '$') {
        nmea_sync = 1;
    } else if (nmea_sync == 1) {
        if (data == 'G') {
            nmea_sync = 2;
        } else {
            nmea_sync = 0;
        }
    } else if (nmea_sync == 2) {
        if (data >= 'A' && data <= 'Z') {
            usart_ext_send_byte('$');
            usart_ext_send_byte('G');
            usart_ext_send_byte(data);
            nmea_sync = 3;
        } else {
            nmea_sync = 0;
        }
    }
}

static void ubxm10050_handle_nmea_output(uint8_t data)
{
    usart_ext_send_byte(data);
    if (data == '\r') {
        nmea_sync = 3;
    } else if (nmea_sync == 3 && data == '\n') {
        nmea_sync = 0;
    }
}
#endif

void ubxm10050_handle_incoming_byte(uint8_t data, uint8_t reset)
{
    if (reset) {
       parse_sync = 0;
       parse_pos = 0;
#if GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE
       nmea_sync = 0;
#endif
    }
    if (raw_capture_pos < RAW_CAPTURE_SIZE) {
        raw_capture_buf[raw_capture_pos++] = data;
    }

#if GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE
    /* If we are in the middle of forwarding an NMEA sentence, keep forwarding */
    if (nmea_sync >= 3) {
        ubxm10050_handle_nmea_output(data);
        return;
    }
#endif

    if (parse_sync == 0) {
        if (data == UBX_SYNC1) {
            parse_sync = 1;
            parse_pos = 0;
        }
#if GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE
        else {
            ubxm10050_handle_nmea_sentence_start(data);
        }
#endif
        return;
    }

    if (parse_sync == 1) {
        if (data == UBX_SYNC2) {
            parse_sync = 2;
        } else {
            parse_sync = 0; /* false start */
#if GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE
            ubxm10050_handle_nmea_sentence_start(data);
#endif
        }
        return;
    }

    /* parse_sync == 2: accumulating */
    if (parse_pos < sizeof(parse_buf)) {
        parse_buf[parse_pos++] = data;
    } else {
        /* Buffer overflow - abandon packet */
        parse_sync = 0;
        parse_pos  = 0;
        return;
    }

    /* Need at least class + id + len(2) = 4 bytes before we know total length */
    if (parse_pos < 4) return;

    uint16_t payloadLen = (uint16_t)parse_buf[2] | ((uint16_t)parse_buf[3] << 8);

    /* Guard against corrupted length: payload must fit in parse_buf
     * alongside the 4-byte header and 2-byte checksum.  Without this,
     * a large payloadLen causes uint16_t totalLen to wrap, bypassing
     * the accumulation check and passing a huge len to checksum(). */
    if (payloadLen > sizeof(parse_buf) - 6) {
        parse_sync = 0;
        parse_pos  = 0;
        m10_current_gps_data.bad_packets++;
        return;
    }

    uint16_t totalLen   = 4 + payloadLen + 2; /* class+id+len + payload + checksum */

    if (parse_pos < totalLen) return; /* still accumulating */

    /* Complete packet: verify checksum */
    uint8_t  msgClass  = parse_buf[0];
    uint8_t  msgId     = parse_buf[1];
    const uint8_t *payload = &parse_buf[4];

    UbxChecksum expected = ubxm10050_checksum(msgClass, msgId, payload, payloadLen);
    uint8_t ck_a = parse_buf[4 + payloadLen];
    uint8_t ck_b = parse_buf[4 + payloadLen + 1];

    if (expected.ck_a == ck_a && expected.ck_b == ck_b) {
        ubxm10050_handle_packet(msgClass, msgId, payload, payloadLen);
    } else {
        m10_current_gps_data.bad_packets++;
    }

    parse_sync = 0;
    parse_pos  = 0;
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

bool ubxm10050_get_current_gps_data(gps_data *data)
{
    bool was_updated = m10_current_gps_data.updated;
    *data = m10_current_gps_data;
    m10_current_gps_data.updated = false;
    return was_updated;
}

void ubxm10050_request_gpstime(void)
{
    /* Poll NAV-TIMEGPS by sending an empty poll request */
    ubxm10050_send_raw(UBX_CLASS_NAV, UBX_NAV_TIMEGPS, NULL, 0);
}

bool ubxm10050_enable_power_save_mode(void)
{
    /* Enable PSMCT (Power Save Mode - Cyclic Tracking).
     * The receiver cycles between tracking and idle states to maintain
     * a fix while reducing power consumption by ~50%.
     * Write to RAM+BBR since RAM is erased during PSM state transitions
     * and restored from BBR on wakeup. */
    const ValsetU1 psm_items[] = {
        { CFG_PM_OPERATEMODE, 2 },  /* 2 = PSMCT (cyclic tracking) */
    };

    log_info("GPS M10: Enabling PSMCT (cyclic tracking) power save\n");
    bool success = ubxm10050_valset_u1_multi(VALSET_LAYER_RAM | VALSET_LAYER_BBR,
                                              psm_items,
                                              sizeof(psm_items) / sizeof(psm_items[0]));
    if (!success) {
        log_error("GPS M10: Failed to enable PSMCT\n");
        return false;
    }

    return true;
}

void ubxm10050_sleep(void)
{
    /* Send RXM-PMREQ to enter backup (deep sleep) mode.
     * The receiver shuts down almost entirely; wakes on UART1 activity.
     * No ACK is sent - the device goes to sleep immediately. */
    UbxRxmPmreqPayload pmreq = {
        .version       = 0x00,
        .reserved1     = {0, 0, 0},
        .duration      = 0,          /* 0 = indefinite */
        .flags         = 0x02,       /* bit1 = backup */
        .wakeupSources = 0x08,       /* bit3 = UART1 */
    };

    log_info("GPS M10: Entering backup (sleep) mode\n");
    ubxm10050_send_raw(UBX_CLASS_RXM, UBX_RXM_PMREQ,
                       (const uint8_t *)&pmreq, sizeof(pmreq));

    /* No more PVT messages after backup mode - set explicitly */
    m10_current_gps_data.power_safe_mode_state = POWER_SAFE_MODE_STATE_INACTIVE;
}

void ubxm10050_init_and_sleep(void)
{
    memset(&m10_current_gps_data, 0, sizeof(gps_data));

    // ublox M10 series defaults to 38400 baud after power-on
    log_info("GPS M10: Initializing USART at 38400 baud for sleep\n");
    usart_gps_init(38400, true);
    delay_ms(100);

    ubxm10050_sleep();
}

bool ubxm10050_init(void)
{
    bool success;

    memset(&m10_current_gps_data, 0, sizeof(gps_data));
    m10_initialized  = false;
    ack_received     = false;
    nack_received    = false;
    raw_capture_pos  = 0;
    pvt_packet_count = 0;

    log_info("GPS M10: Init UART at %d baud\n", GPS_SERIAL_PORT_BAUD_RATE);
    usart_gps_init(GPS_SERIAL_PORT_BAUD_RATE, true);
    delay_ms(100);

    /* Cold reset to put chip in known state */
    log_info("GPS M10: Cold reset\n");
    ubxm10050_cold_reset();
    delay_ms(2000);    /* M10 takes ~1.5s to restart after reset */

    /* Diagnostic: report how many bytes arrived during the reset wait.
     * Expected: ~hundreds of NMEA bytes if GPS RX is alive.
     * Zero means no bytes received - UART RX or GPS power is broken. */
    log_info("GPS M10: Post-reset gps_ints=%lu, captured %u bytes: ",
             gps_ints, (unsigned)raw_capture_pos);
    uint16_t dump_len = raw_capture_pos < RAW_CAPTURE_SIZE ? raw_capture_pos : RAW_CAPTURE_SIZE;
    for (uint16_t i = 0; i < dump_len; i++) {
        log_info("%02X ", raw_capture_buf[i]);
    }
    log_info("\n");

    /* Step 1: Set baud rate via CFG-VALSET.
     * The chip resets to 38400 by default. If GPS_SERIAL_PORT_BAUD_RATE
     * differs, set it in RAM first, then switch our UART to match. */
    if (GPS_SERIAL_PORT_BAUD_RATE != 38400) {
        log_info("GPS M10: Setting baud rate to %d\n", GPS_SERIAL_PORT_BAUD_RATE);
        success = ubxm10050_valset_u4(VALSET_LAYER_RAM,
                                       CFG_UART1_BAUDRATE,
                                       (uint32_t)GPS_SERIAL_PORT_BAUD_RATE);
        if (!success) {
            log_error("GPS M10: Failed to set baud rate\n");
            return false;
        }
        delay_ms(100);
        usart_gps_set_baud_rate(GPS_SERIAL_PORT_BAUD_RATE);
        delay_ms(100);
    }

    /* Step 2: Configure UART1 protocols */
    log_info("GPS M10: Configuring UART1 protocols\n");
    {
        const ValsetU1 proto_items[] = {
            { CFG_UART1INPROT_UBX,   1 },  /* Accept UBX input */
            { CFG_UART1INPROT_NMEA,  0 },  /* Reject NMEA input */
            { CFG_UART1OUTPROT_UBX,  1 },  /* Output UBX */
#if GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE
            { CFG_UART1OUTPROT_NMEA, 1 },  /* Enable NMEA output */
#else
            { CFG_UART1OUTPROT_NMEA, 0 },  /* No NMEA output */
#endif
        };
        success = ubxm10050_valset_u1_multi(VALSET_LAYER_RAM,
                                             proto_items,
                                             sizeof(proto_items) / sizeof(proto_items[0]));
        if (!success) {
            log_error("GPS M10: Protocol config failed\n");
            return false;
        }
    }

    /* Step 3: Set dynamic model to Airborne < 1g (required for balloon) */
    log_info("GPS M10: Setting dynamic model to AIR1G\n");
    {
        const ValsetU1 dynmodel_items[] = {
            { CFG_NAVSPG_DYNMODEL, DYNMODEL_AIR1G },
        };
        success = ubxm10050_valset_u1_multi(VALSET_LAYER_RAM,
                                             dynmodel_items, 1);
        if (!success) {
            log_error("GPS M10: Dynamic model config failed\n");
            return false;
        }
    }

    /* Step 4: Enable NAV-PVT at 1Hz on UART1, disable other nav messages */
    log_info("GPS M10: Configuring message output rates\n");
    {
        const ValsetU1 msg_items[] = {
            { CFG_MSGOUT_NAV_PVT_UART1,     1 },  /* 1 = every epoch */
            { CFG_MSGOUT_NAV_TIMEGPS_UART1, 1 },  /* GPS time for leap seconds */
            { CFG_MSGOUT_NAV_TIMEUTC_UART1, 0 },  /* Not needed - PVT has UTC */
            { CFG_MSGOUT_NAV_STATUS_UART1,  0 },  /* Not needed - PVT has fixOK */
        };
        success = ubxm10050_valset_u1_multi(VALSET_LAYER_RAM,
                                             msg_items,
                                             sizeof(msg_items) / sizeof(msg_items[0]));
        if (!success) {
            log_error("GPS M10: Message rate config failed\n");
            return false;
        }
    }

#if GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE
    /* Step 5: Enable standard NMEA sentence output at 1Hz */
    log_info("GPS M10: Configuring NMEA message output rates\n");
    {
        const ValsetU1 nmea_items[] = {
            { CFG_MSGOUT_NMEA_GGA_UART1, 1 },  /* GGA every epoch */
            { CFG_MSGOUT_NMEA_GLL_UART1, 1 },  /* GLL every epoch */
            { CFG_MSGOUT_NMEA_GSA_UART1, 1 },  /* GSA every epoch */
            { CFG_MSGOUT_NMEA_GSV_UART1, 1 },  /* GSV every epoch */
            { CFG_MSGOUT_NMEA_RMC_UART1, 1 },  /* RMC every epoch */
            { CFG_MSGOUT_NMEA_VTG_UART1, 1 },  /* VTG every epoch */
        };
        success = ubxm10050_valset_u1_multi(VALSET_LAYER_RAM,
                                             nmea_items,
                                             sizeof(nmea_items) / sizeof(nmea_items[0]));
        if (!success) {
            log_error("GPS M10: NMEA message rate config failed\n");
            return false;
        }
    }
#endif

    m10_initialized = true;
    log_info("GPS M10: Init complete\n");
    return true;
}
