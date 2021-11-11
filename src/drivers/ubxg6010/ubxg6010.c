#include <string.h>

#include "hal/system.h"
#include "hal/usart_gps.h"
#include "hal/usart_ext.h"
#include "hal/delay.h"

#include "ubxg6010.h"
#include "log.h"

#define GPS_INITIAL_BAUD_RATE 9600

typedef struct __attribute__((packed)) {
    uint8_t sc1; // 0xB5
    uint8_t sc2; // 0x62
    uint8_t messageClass;
    uint8_t messageId;
    uint16_t payloadSize;
} uBloxHeader;

typedef struct {
    uint8_t ck_a;
    uint8_t ck_b;
} uBloxChecksum;

typedef struct {
    uint32_t iTOW;        //GPS time of week of the navigation epoch. [- ms]
    uint16_t year;        //Year (UTC) [- y]
    uint8_t month;        //Month, range 1..12 (UTC) [- month]
    uint8_t day;          //Day of month, range 1..31 (UTC) [- d]
    uint8_t hour;          //Hour of day, range 0..23 (UTC) [- h]
    uint8_t min;          //Minute of hour, range 0..59 (UTC) [- min]
    uint8_t sec;          //Seconds of minute, range 0..60 (UTC) [- s]
    uint8_t valid;        //Validity flags (see graphic below) [- -]
    uint32_t tAcc;        //Time accuracy estimate (UTC) [- ns]
    int32_t nano;          //Fraction of second, range -1e9 .. 1e9 (UTC) [- ns]
    uint8_t fixType;    //GNSSfix Type: [- -]
    uint8_t flags;        //Fix status flags (see graphic below) [- -]
    uint8_t flags2;        //Additional flags (see graphic below) [- -]
    uint8_t numSV;        //Number of satellites used in Nav Solution [- -]
    int32_t lon;          //Longitude [1e-7 deg]
    int32_t lat;          //Latitude [1e-7 deg]
    int32_t height;        //Height above ellipsoid [- mm]
    int32_t hMSL;          //Height above mean sea level [- mm]
    uint32_t hAcc;        //Horizontal accuracy estimate [- mm]
    uint32_t vAcc;        //Vertical accuracy estimate [- mm]
    int32_t velN;          //NED north velocity [- mm/s]
    int32_t velE;          //NED east velocity [- mm/s]
    int32_t velD;          //NED down velocity [- mm/s]
    int32_t gSpeed;        //Ground Speed (2-D) [- mm/s]
    int32_t headMot;    //Heading of motion (2-D) [1e-5 deg]
    uint32_t sAcc;        //Speed accuracy estimate [- mm/s]
    uint32_t headAcc;    //Heading accuracy estimate (both motion and vehicle) [1e-5 deg]
    uint16_t pDOP;        //Position DOP [0.01 -]
    uint8_t reserved1[6];        //Reserved [- -]
    int32_t headVeh;    //Heading of vehicle (2-D) [1e-5 deg]
    uint8_t reserved2[4];        //Reserved [- -]
} uBloxNAVPVTPayload;

typedef struct {
    uint32_t iTOW;        //GPS Millisecond Time of Week [- ms]
    int32_t lon;        //Longitude [1e-7 deg]
    int32_t lat;        //Latitude [1e-7 deg]
    int32_t height;        //Height above Ellipsoid [- mm]
    int32_t hMSL;        //Height above mean sea level [- mm]
    uint32_t hAcc;        //Horizontal Accuracy Estimate [- mm]
    uint32_t vAcc;        //Vertical Accuracy Estimate [- mm]
} uBloxNAVPOSLLHPayload;

typedef struct {
    uint32_t iTOW;        //GPS Millisecond Time of Week [- ms]
    int32_t fTOW;        //Fractional Nanoseconds remainder of rounded ms above, range -500000 .. 500000 [- ns]
    int16_t week;        //GPS week (GPS time) [- -]
    uint8_t gpsFix;        //GPSfix Type, range 0..5 0x00 = No Fix 0x01 = Dead Reckoning only 0x02 = 2D-Fix 0x03 = 3D-Fix 0x04 = GPS + dead reckoning combined 0x05 = Time only fix 0x06..0xff: reserved [- -]
    uint8_t flags;        //Fix Status Flags (see graphic below) [- -]
    int32_t ecefX;        //ECEF X coordinate [- cm]
    int32_t ecefY;        //ECEF Y coordinate [- cm]
    int32_t ecefZ;        //ECEF Z coordinate [- cm]
    uint32_t pAcc;        //3D Position Accuracy Estimate [- cm]
    int32_t ecefVX;        //ECEF X velocity [- cm/s]
    int32_t ecefVY;        //ECEF Y velocity [- cm/s]
    int32_t ecefVZ;        //ECEF Z velocity [- cm/s]
    uint32_t sAcc;        //Speed Accuracy Estimate [- cm/s]
    uint16_t pDOP;        //Position DOP [0.01 -]
    uint8_t reserved1;        //Reserved [- -]
    uint8_t numSV;        //Number of SVs used in Nav Solution [- -]
    uint32_t reserved2;        //Reserved [- -]
} uBloxNAVSOLPayload;

typedef struct {
    uint32_t iTOW;        //GPS Millisecond Time of Week [- ms]
    uint32_t fTOW;        //Fractional Nanoseconds remainder of rounded ms above, range -500000 .. 500000 [- ns]
    int16_t week;         //GPS week (GPS time) [- -]
    int8_t leapS;         //Leap Seconds (GPS-UTC) [- s]
    uint8_t valid;        //Validity Flags (see graphic below) [- -]
    uint32_t tAcc;        //Time Accuracy Estimate [- ns]
} uBloxNAVTIMEGPSPayload;

typedef struct {
    uint32_t iTOW;        //GPS Millisecond Time of Week [- ms]
    uint32_t tAcc;        //Time Accuracy Estimate [- ns]
    int32_t nano;        //Nanoseconds of second, range -1e9 .. 1e9 (UTC) [- ns]
    uint16_t year;        //Year, range 1999..2099 (UTC) [- y]
    uint8_t month;        //Month, range 1..12 (UTC) [- month]
    uint8_t day;        //Day of Month, range 1..31 (UTC) [- d]
    uint8_t hour;        //Hour of Day, range 0..23 (UTC) [- h]
    uint8_t min;        //Minute of Hour, range 0..59 (UTC) [- min]
    uint8_t sec;        //Seconds of Minute, range 0..59 (UTC) [- s]
    uint8_t valid;        //Validity Flags (see graphic below) [- -]
} uBloxNAVTIMEUTCPayload;

typedef struct {
    uint32_t iTOW;        //GPS Millisecond Time of Week [- ms]
    int32_t velN;          //NED north velocity [- cm/s]
    int32_t velE;          //NED east velocity [- cm/s]
    int32_t velD;          //NED down velocity [- cm/s]
    uint32_t speed;        //Speed (3-D) [- cm/s]
    uint32_t gSpeed;        //Ground Speed (2-D) [- cm/s]
    int32_t headMot;    //Heading of motion (2-D) [1e-5 deg]
    uint32_t sAcc;        //Speed accuracy estimate [- cm/s]
    uint32_t headAcc;    //Heading accuracy estimate (both motion and vehicle) [1e-5 deg]
} uBloxNAVVELNEDPayload;

typedef struct {
    uint32_t iTOW;        //GPS Millisecond Time of Week [- ms]
    uint8_t gpsFix;        //GPS Fix State
    uint8_t flags;  // LSB = gpsFixOK - the only valid way of determining if a fix is actually OK.
    uint8_t fixStat;
    uint8_t flags2; // Power Save Mode State
    uint32_t ttff;
    uint32_t msss;
} uBloxNAVSTATUSPayload;

typedef struct {
    uint8_t portID;        //Port Identifier Number (see Serial [- -]
    uint8_t reserved1;        //Reserved [- -]
    uint16_t txReady;        //TX ready PIN configuration [- -]
    uint32_t mode;        //A bit mask describing the UART mode [- -]
    uint32_t baudRate;        //Baud rate in bits/second [- Bits/s]
    uint16_t inProtoMask;        //A mask describing which input protocols are active. [- -]
    uint16_t outProtoMask;        //A mask describing which output protocols are active. [- -]
    uint16_t flags;        //Flags bit mask (see graphic below) [- -]
    uint8_t reserved2[2];        //Reserved [- -]
} uBloxCFGPRTPayload;

typedef struct {
    uint8_t clsID;        //Message Class [- -]
    uint8_t msgID;        //Message Identifier [- -]
    uint8_t ck_a;
    uint8_t ck_b;
} uBloxACKACKayload;

typedef struct {
    uint8_t msgClass;        //Message Class [- -]
    uint8_t msgID;        //Message Identifier [- -]
} uBloxCFGMSGPOLLPayload;

typedef struct {
    uint8_t msgClass;        //Message Class [- -]
    uint8_t msgID;        //Message Identifier [- -]
    uint8_t rate;        //Send rate on current Port [- -]
} uBloxCFGMSGPayload;

typedef struct {
    uint16_t navBbrMask;        //BBR Sections to clear. The following Special Sets apply:
    // 0x0000 Hotstart
    // 0x0001 Warmstart
    // 0xFFFF Coldstart [- -]
    uint8_t resetMode;        //Reset Type
    // - 0x00 - Hardware reset (Watchdog) immediately
    // - 0x01 - Controlled Software reset
    // - 0x02 - Controlled Software reset (GPS only)
    // - 0x04 - Hardware reset (Watchdog) after shutdown (>=FW6.0)
    // - 0x08 - Controlled GPS stop
    // - 0x09 - Controlled GPS start [- -]
    // - 0x09 - Controlled GPS start [- -]
    uint8_t reserved1;        //Reserved [- -]
} uBloxCFGRSTPayload;

typedef struct {
    uint16_t mask;        //Parameters Bitmask. Only the masked parameters will be applied. (see graphic below) [- -]
    uint8_t dynModel;        //Dynamic Platform model: - 0 􀀀 Portable - 2 􀀀 Stationary - 3 􀀀 Pedestrian - 4 􀀀 Automotive - 5 􀀀 Sea - 6 􀀀 Airborne with <1g Acceleration - 7 􀀀 Airborne with <2g Acceleration - 8 􀀀 Airborne with <4g Acceleration [- -]
    uint8_t fixMode;        //Position Fixing Mode. - 1: 2D only - 2: 3D only - 3: Auto 2D/3D [- -]
    int32_t fixedAlt;        //Fixed altitude (mean sea level) for 2D fix mode. [0.01 m]
    uint32_t fixedAltVar;        //Fixed altitude variance for 2D mode. [0.0001 m^2]
    int8_t minElev;        //Minimum Elevation for a GNSS satellite to be used in NAV [- deg]
    uint8_t drLimit;        //Maximum time to perform dead reckoning (linear extrapolation) in case of GPS signal loss [- s]
    uint16_t pDop;        //Position DOP Mask to use [0.1 -]
    uint16_t tDop;        //Time DOP Mask to use [0.1 -]
    uint16_t pAcc;        //Position Accuracy Mask [- m]
    uint16_t tAcc;        //Time Accuracy Mask [- m]
    uint8_t staticHoldThresh;        //Static hold threshold [- cm/s]
    uint8_t dgpsTimeOut;        //DGPS timeout, firmware 7 and newer only [- s]
    uint32_t reserved2;        //Always set to zero [- -]
    uint32_t reserved3;        //Always set to zero [- -]
    uint32_t reserved4;        //Always set to zero [- -]
} uBloxCFGNAV5Payload;

typedef struct {
    uint8_t reserved1;        //Always set to 8 [- -]
    uint8_t lpMode;        //Low Power Mode 0: Max. performance mode 1: Power Save Mode (>= FW 6.00 only) 2-3: reserved 4: Eco mode 5-255: reserved [- -]
} uBloxCFGRXMPayload;

typedef struct {
    uint16_t measRate;       //Measurement Rate, GPS measurements are taken every measRate milliseconds [- ms]
    uint16_t navRate;        //Navigation Rate, in number of measurement cycles. On u-blox 5 and u-blox 6, this parameter cannot be changed, and is always equals 1. [- cycles]
    uint16_t timeRef;        //Alignment to reference time: 0 = UTC time, 1 = GPS time [- -]
} uBloxCFGRATEPayload;

typedef struct {
    uint8_t filter;    // Filter flags
    uint8_t version;   // 0x23 = NMEA version 2.3, 0x21 = NMEA version 2.1
    uint8_t numSV;     // Maximum Number of SVs to report in NMEA protocol.
    uint8_t flags;     // Flags: 0x01 = enable compatibility mode, 0x02 = enable considering mode
} uBloxCFGNMEA;

typedef union {
    uBloxNAVPVTPayload navpvt;
    uBloxCFGPRTPayload cfgprt;
    uBloxCFGMSGPOLLPayload cfgmsgpoll;
    uBloxCFGMSGPayload cfgmsg;
    uBloxCFGNAV5Payload cfgnav5;
    uBloxNAVPOSLLHPayload navposllh;
    uBloxNAVSOLPayload navsol;
    uBloxNAVTIMEGPSPayload navtimegps;
    uBloxNAVTIMEUTCPayload navtimeutc;
    uBloxNAVVELNEDPayload navvelned;
    uBloxNAVSTATUSPayload navstatus;
    uBloxACKACKayload ackack;
    uBloxCFGRSTPayload cfgrst;
    uBloxCFGRXMPayload cfgrxm;
    uBloxCFGRATEPayload cfgrate;
    uBloxCFGNMEA cfgnmea;
} ubloxPacketData;

typedef struct __attribute__((packed)) {
    uBloxHeader header;
    ubloxPacketData data;
} uBloxPacket;

gps_data ubxg6010_current_gps_data;

volatile bool gps_initialized = false;
volatile bool ack_received = false;
volatile bool nack_received = false;

volatile static uint8_t sync_ubx = 0;
volatile static uint8_t sync_nmea = 0;

static uBloxChecksum
ubxg6010_calculate_checksum(const uint8_t msgClass, const uint8_t msgId, const uint8_t *message, uint16_t size)
{
    uBloxChecksum ck = {0, 0};

    ck.ck_a += msgClass;
    ck.ck_b += ck.ck_a;
    ck.ck_a += msgId;
    ck.ck_b += ck.ck_a;

    ck.ck_a += size & 0xffU;
    ck.ck_b += ck.ck_a;
    ck.ck_a += size >> 8U;
    ck.ck_b += ck.ck_a;

    for (uint16_t i = 0; i < size; i++) {
        ck.ck_a += message[i];
        ck.ck_b += ck.ck_a;
    }

    return ck;
}

static void ubxg6010_clear_ack()
{
    ack_received = false;
    nack_received = false;
}

static bool ubxg6010_wait_for_ack()
{
    uint16_t timeout = 1500;

    while (!ack_received && !nack_received && timeout-- > 0) {
        delay_ms(1);
    }

    return ack_received;
}

void ubxg6010_send_command(uint8_t msgClass, uint8_t msgId, uint8_t *payload, uint16_t payloadSize)
{
    ubxg6010_clear_ack();

    uBloxChecksum chksum = ubxg6010_calculate_checksum(msgClass, msgId, payload, payloadSize);

    usart_gps_send_byte(0xB5);
    usart_gps_send_byte(0x62);
    usart_gps_send_byte(msgClass);
    usart_gps_send_byte(msgId);
    usart_gps_send_byte((uint8_t) (payloadSize & 0xFFU));
    usart_gps_send_byte((uint8_t) (payloadSize >> 8U));

    uint16_t i;
    for (i = 0; i < payloadSize; ++i) {
        usart_gps_send_byte(payload[i]);
    }
    usart_gps_send_byte(chksum.ck_a);
    usart_gps_send_byte(chksum.ck_b);
}

void ubxg6010_send_packet(uBloxPacket *packet)
{
    ubxg6010_send_command(packet->header.messageClass, packet->header.messageId, (uint8_t *) &packet->data,
            packet->header.payloadSize);
}

bool ubxg6010_send_packet_and_wait_for_ack(uBloxPacket *packet)
{
    int retries = 3;
    bool success;

    do {
        ubxg6010_send_packet(packet);
        success = ubxg6010_wait_for_ack();
    } while (!success && retries-- > 0);

    return success;
}

bool ubxg6010_get_current_gps_data(gps_data *data)
{
    system_disable_irq();
    memcpy(data, &ubxg6010_current_gps_data, sizeof(gps_data));
    ubxg6010_current_gps_data.updated = false;
    system_enable_irq();

    return data->updated;
}

uBloxPacket msgcfgrst = {
        .header = {
                0xb5,
                0x62,
                .messageClass=0x06,
                .messageId=0x04,
                .payloadSize=sizeof(uBloxCFGRSTPayload)
        },
        .data.cfgrst = {
                .navBbrMask=0xffff, // Coldstart
                .resetMode=1, // Controlled Software reset
                .reserved1=0
        },
};

uBloxPacket msgcfgprt = {
        .header = {
                0xb5,
                0x62,
                .messageClass=0x06,
                .messageId=0x00,
                .payloadSize=sizeof(uBloxCFGPRTPayload)
        },
        .data.cfgprt = {
                .portID=1,
                .reserved1=0,
                .txReady=0,
                .mode=0b0000100011000000, // 8 bits, no parity, 1 stop bit
                .baudRate=38400,
                .inProtoMask=1, // UBX protocol for input
                .outProtoMask=1, // UBX protocol for output
                .flags=0,
                .reserved2={0, 0}
        },
};

/**
 * Low Power Mode
 * 0: Max. performance mode
 * 1: Power Save Mode (>= FW 6.00 only)
 * 2-3: reserved
 * 4: Eco mode
 * 5-255: reserved
 */
uBloxPacket msgcfgrxm = {
        .header = {
                0xb5,
                0x62,
                .messageClass=0x06,
                .messageId=0x11,
                .payloadSize=sizeof(uBloxCFGRXMPayload)
        },
        .data.cfgrxm = {
                .reserved1=8, // Always set to 8
                .lpMode=0 // Low power mode: Eco mode -- TODO: set back to Eco mode
        }
};

// Configure rate of 1 for message: 0x01 0x02 Geodetic Position Solution
uBloxPacket msgcfgmsg = {
        .header = {
                0xb5,
                0x62,
                .messageClass=0x06,
                .messageId=0x01,
                .payloadSize=sizeof(uBloxCFGMSGPayload)
        },
        .data.cfgmsg = {
                .msgClass=0x01,
                .msgID=0x02,
                .rate=1
        }
};

uBloxPacket msgcfgrate = {
        .header = {
                0xb5,
                0x62,
                .messageClass=0x06,
                .messageId=0x08,
                .payloadSize=sizeof(uBloxCFGRATEPayload)
        },
        .data.cfgrate = {
                .measRate=1000,
                .navRate=1,
                .timeRef=0,
        }
};

/**
 * Dynamic Platform model:
 * - 0 Portable
 * - 2 Stationary
 * - 3 Pedestrian
 * - 4 Automotive
 * - 5 Sea
 * - 6 Airborne with <1g Acceleration
 * - 7 Airborne with <2g Acceleration
 * - 8 Airborne with <4g Acceleration
 *
 * Position Fixing Mode.
 * - 1: 2D only
 * - 2: 3D only
 * - 3: Auto 2D/3D
 */
uBloxPacket msgcfgnav5 = {
        .header = {
                0xb5,
                0x62,
                .messageClass=0x06,
                .messageId=0x24,
                .payloadSize=sizeof(uBloxCFGNAV5Payload)
        },
        // NOTE: Dynamic model needs to be set to one of the Airborne modes to support high altitudes!
        // Notes from darksidelemm RS41HUP fork: Tweaked the PDOP limits a bit, to make it a bit more likely to report a position.
        .data.cfgnav5={
                .mask=0b0000001111111111, // Configure all settings
                .dynModel=6, // Dynamic model: Airborne with <1g Acceleration
                .fixMode=2, // Fix mode: 3D only
                .fixedAlt=0, // Fixed altitude (mean sea level) for 2D fix mode.
                .fixedAltVar=10000, // Fixed altitude variance for 2D mode.
                .minElev=5, // Minimum Elevation for a GNSS satellite to be used in NAV (degrees)
                .drLimit=0, // Maximum time to perform dead reckoning (linear extrapolation) in case of GPS signal loss (seconds)
                .pDop=100, // Position DOP Mask to use (was 25)
                .tDop=100, // Time DOP Mask to use (was 25)
                .pAcc=100, // Position Accuracy Mask (m)
                .tAcc=300, // Time Accuracy Mask (m)
                .staticHoldThresh=0, // Static hold threshold (cm/s)
                .dgpsTimeOut=2, // DGPS timeout, firmware 7 and newer only
                .reserved2=0,
                .reserved3=0,
                .reserved4=0
        },
};

// Configure NMEA protocol version 2.3
uBloxPacket msgcfgnmea = {
        .header = {
                0xb5,
                0x62,
                .messageClass=0x06,
                .messageId=0x17,
                .payloadSize=sizeof(uBloxCFGNMEA)
        },
        .data.cfgnmea = {
                .filter = 0x00,
                .version = 0x23,
                .numSV = 24,
                .flags = 0x00,
        }
};

bool ubxg6010_init()
{
    bool success;

    memset(&ubxg6010_current_gps_data, 0, sizeof(gps_data));

    gps_initialized = false;

    log_info("GPS: Initializing USART with baud rate %d\n", GPS_SERIAL_PORT_BAUD_RATE);
    usart_gps_init(GPS_SERIAL_PORT_BAUD_RATE, true);
    delay_ms(100);

    log_info("GPS: Resetting GPS chip\n");
    ubxg6010_send_packet(&msgcfgrst);
    delay_ms(1000);

    log_info("GPS: Initializing USART with baud rate %d\n", GPS_INITIAL_BAUD_RATE);
    usart_gps_init(GPS_INITIAL_BAUD_RATE, true);
    delay_ms(100);

    log_info("GPS: Resetting GPS chip\n");
    ubxg6010_send_packet(&msgcfgrst);
    delay_ms(1000);

    if (gps_nmea_output_enabled) {
        log_info("GPS: Configuring GPS NMEA output settings\n");
        ubxg6010_send_packet(&msgcfgnmea);
        delay_ms(100);
    }

    log_info("GPS: Configuring GPS chip I/O port settings\n");
    if (gps_nmea_output_enabled) {
        // Enable both UBX and NMEA protocols
        msgcfgprt.data.cfgprt.outProtoMask = 0x03;
    }
    ubxg6010_send_packet(&msgcfgprt);
    delay_ms(100);

    log_info("GPS: Initializing USART with baud rate %d\n", GPS_SERIAL_PORT_BAUD_RATE);
    usart_gps_init(GPS_SERIAL_PORT_BAUD_RATE, true);
    delay_ms(100);

    log_info("GPS: Setting GPS chip power mode\n");
    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgrxm);
    if (!success) {
        return false;
    }

    // Rate of 1 for message: 0x01 0x02 Geodetic Position Solution
    msgcfgmsg.data.cfgmsg.msgID = 0x02;
    msgcfgmsg.data.cfgmsg.rate = 1;
    log_info("GPS: Requesting update messages from GPS chip\n");
    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgmsg);
    if (!success) {
        return false;
    }

    log_info("GPS: Setting Navigation/Measurement rate settings in GPS chip\n");
    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgrate);
    if (!success) {
        return false;
    }

    // Rate of 1 for message: 0x01 0x06 Navigation Solution Information
    msgcfgmsg.data.cfgmsg.msgID = 0x06;
    msgcfgmsg.data.cfgmsg.rate = 1;
    log_info("GPS: Requesting update messages from GPS chip\n");
    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgmsg);
    if (!success) {
        return false;
    }

    // Configure rate of 1 for message: 0x01 0x20 GPS Time Solution
/*    msgcfgmsg.data.cfgmsg.msgID = 0x20;
    msgcfgmsg.data.cfgmsg.rate = 1;
    log_info("GPS: Requesting update messages from GPS chip\n");
    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgmsg);
    if (!success) {
        return false;
    }*/

    // Configure rate of 1 for message: 0x01 0x21 UTC Time Solution
    msgcfgmsg.data.cfgmsg.msgID = 0x21;
    msgcfgmsg.data.cfgmsg.rate = 1;
    log_info("GPS: Requesting update messages from GPS chip\n");
    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgmsg);
    if (!success) {
        return false;
    }

    // Configure rate of 2 for message: 0x01 0x12 Velocity Solution in NED
    msgcfgmsg.data.cfgmsg.msgID = 0x12;
    msgcfgmsg.data.cfgmsg.rate = 2;
    log_info("GPS: Requesting update messages from GPS chip\n");
    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgmsg);
    if (!success) {
        return false;
    }

    // Configure rate of 2 for message: 0x01 0x03 Receiver Navigation Status
    msgcfgmsg.data.cfgmsg.msgID = 0x03;
    msgcfgmsg.data.cfgmsg.rate = 1;
    log_info("GPS: Requesting update messages from GPS chip\n");
    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgmsg);
    if (!success) {
        return false;
    }

    // TODO: Is this message supported: Configure rate of 1 for message: 0x01 0x07 Position Velocity Time Solution
    /*msgcfgmsg.data.cfgmsg.msgID = 0x07;
    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgmsg);
    if (!success) {
        return false;
    }*/

    log_info("GPS: Configuring navigation engine settings\n");
    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgnav5);
    if (!success) {
        return false;
    }

    gps_initialized = true;

    return true;
}

// Poll request for message: 0x01 0x20 GPS Time Solution
uBloxPacket msgnavgpstime = {
        .header = {
                0xb5,
                0x62,
                .messageClass=0x01,
                .messageId=0x20,
                .payloadSize=0
        }
};

uBloxPacket msgcfgmsgpoll = {
        .header = {
                0xb5,
                0x62,
                .messageClass=0x06,
                .messageId=0x01,
                .payloadSize=sizeof(uBloxCFGMSGPOLLPayload)
        },
        .data.cfgmsgpoll = {
                .msgClass=0x01,
                .msgID=0x20,
        }
};

void ubxg6010_request_gpstime()
{
    //ubxg6010_send_packet(&msgcfgmsgpoll);
    ubxg6010_send_packet(&msgnavgpstime);
}

static void ubxg6010_handle_packet(uBloxPacket *pkt)
{
    uBloxChecksum cksum = ubxg6010_calculate_checksum(pkt->header.messageClass, pkt->header.messageId,
            (const uint8_t *) &pkt->data, pkt->header.payloadSize);
    uBloxChecksum *checksum = (uBloxChecksum *) (((uint8_t *) &pkt->data) + pkt->header.payloadSize);

    if (cksum.ck_a != checksum->ck_a || cksum.ck_b != checksum->ck_b) {
        ubxg6010_current_gps_data.bad_packets += 1;
        return;
    }

    log_debug("GPS message: class=0x%02X id=0x%02X\n", pkt->header.messageClass, pkt->header.messageId);

    if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x07) {
        // TODO: It seems NAV PVT message is not supported by UBXG6010, confirm this
        ubxg6010_current_gps_data.ok_packets += 1;
        ubxg6010_current_gps_data.time_of_week_millis = pkt->data.navpvt.iTOW;
        ubxg6010_current_gps_data.year = pkt->data.navpvt.year;
        ubxg6010_current_gps_data.month = pkt->data.navpvt.month;
        ubxg6010_current_gps_data.day = pkt->data.navpvt.day;
        ubxg6010_current_gps_data.hours = pkt->data.navpvt.hour;
        ubxg6010_current_gps_data.minutes = pkt->data.navpvt.min;
        ubxg6010_current_gps_data.seconds = pkt->data.navpvt.sec;

        ubxg6010_current_gps_data.fix = pkt->data.navpvt.fixType;
        ubxg6010_current_gps_data.latitude_degrees_1000000 = pkt->data.navpvt.lat;
        ubxg6010_current_gps_data.longitude_degrees_1000000 = pkt->data.navpvt.lon;
        ubxg6010_current_gps_data.altitude_mm = pkt->data.navpvt.hMSL;
        ubxg6010_current_gps_data.satellites_visible = pkt->data.navpvt.numSV;
        ubxg6010_current_gps_data.ground_speed_cm_per_second = pkt->data.navpvt.gSpeed;
        ubxg6010_current_gps_data.heading_degrees_100000 = pkt->data.navpvt.headMot;
        ubxg6010_current_gps_data.climb_cm_per_second = -pkt->data.navpvt.velD;

        ubxg6010_current_gps_data.updated = true;
    } else if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x12) {
        ubxg6010_current_gps_data.ok_packets += 1;
        ubxg6010_current_gps_data.time_of_week_millis = pkt->data.navvelned.iTOW;
        ubxg6010_current_gps_data.ground_speed_cm_per_second = pkt->data.navvelned.gSpeed;
        ubxg6010_current_gps_data.heading_degrees_100000 = pkt->data.navvelned.headMot;
        ubxg6010_current_gps_data.climb_cm_per_second = -pkt->data.navvelned.velD;

        ubxg6010_current_gps_data.updated = true;
    } else if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x02) {
        ubxg6010_current_gps_data.ok_packets += 1;
        ubxg6010_current_gps_data.time_of_week_millis = pkt->data.navposllh.iTOW;
        ubxg6010_current_gps_data.latitude_degrees_1000000 = pkt->data.navposllh.lat;
        ubxg6010_current_gps_data.longitude_degrees_1000000 = pkt->data.navposllh.lon;
        ubxg6010_current_gps_data.altitude_mm = pkt->data.navposllh.hMSL;

        ubxg6010_current_gps_data.updated = true;
    } else if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x03) {
        ubxg6010_current_gps_data.ok_packets += 1;
        ubxg6010_current_gps_data.fix_ok = pkt->data.navstatus.flags & 0x01;
        ubxg6010_current_gps_data.power_safe_mode_state = pkt->data.navstatus.flags2 & 0x03;

        ubxg6010_current_gps_data.updated = true;
    } else if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x06) {
        ubxg6010_current_gps_data.time_of_week_millis = pkt->data.navsol.iTOW;
        ubxg6010_current_gps_data.week = pkt->data.navsol.week;
        ubxg6010_current_gps_data.fix = pkt->data.navsol.gpsFix;
        ubxg6010_current_gps_data.satellites_visible = pkt->data.navsol.numSV;
        ubxg6010_current_gps_data.position_dilution_of_precision = pkt->data.navsol.pDOP;

        ubxg6010_current_gps_data.updated = true;
    } else if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x20) {
        ubxg6010_current_gps_data.time_of_week_millis = pkt->data.navtimegps.iTOW;
        ubxg6010_current_gps_data.week = pkt->data.navtimegps.week;

        ubxg6010_current_gps_data.updated = true;
    } else if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x21) {
        ubxg6010_current_gps_data.year = pkt->data.navtimeutc.year;
        ubxg6010_current_gps_data.month = pkt->data.navtimeutc.month;
        ubxg6010_current_gps_data.day = pkt->data.navtimeutc.day;
        ubxg6010_current_gps_data.hours = pkt->data.navtimeutc.hour;
        ubxg6010_current_gps_data.minutes = pkt->data.navtimeutc.min;
        ubxg6010_current_gps_data.seconds = pkt->data.navtimeutc.sec;

        ubxg6010_current_gps_data.updated = true;
    } else if (pkt->header.messageClass == 0x05 && pkt->header.messageId == 0x01) {
        ack_received = true;
    } else if (pkt->header.messageClass == 0x05 && pkt->header.messageId == 0x00) {
        nack_received = true;
    }
}

void ubxg6010_reset_parser()
{
    sync_ubx = 0;
    sync_nmea = 0;
}

static void ubxg6010_handle_nmea_sentence_start(uint8_t data)
{
    if (sync_nmea == 0 && data == '$') {
        sync_nmea = 1;
    } else if (sync_nmea == 1) {
        if (data == 'G') {
            sync_nmea = 2;
        } else {
            sync_nmea = 0;
        }
    } else if (sync_nmea == 2) {
        if (data >= 'A' && data <= 'Z') {
            usart_ext_send_byte('$');
            usart_ext_send_byte('G');
            usart_ext_send_byte(data);
            sync_nmea = 3;
        } else {
            sync_nmea = 0;
        }
    }
}

static void ubxg6010_handle_nmea_output(uint8_t data)
{
    usart_ext_send_byte(data);
    if (data == '\r') {
        sync_nmea = 3;
    } else if (sync_nmea == 3 && data == '\n') {
        sync_nmea = 0;
    }
}

void ubxg6010_handle_incoming_byte(uint8_t data)
{
    static uint8_t buffer_pos = 0;
    static uint8_t incoming_packet_buffer[sizeof(uBloxPacket) + sizeof(uBloxChecksum)];
    static uBloxPacket *incoming_packet = (uBloxPacket *) incoming_packet_buffer;

    if (!sync_ubx && (sync_nmea < 3)) {
        if (!buffer_pos && data == 0xB5) {
            buffer_pos = 1;
            incoming_packet->header.sc1 = data;
        } else if (buffer_pos == 1 && data == 0x62) {
            sync_ubx = 1;
            buffer_pos = 2;
            incoming_packet->header.sc2 = data;
        } else {
            if (gps_nmea_output_enabled) {
                ubxg6010_handle_nmea_sentence_start(data);
            }
            buffer_pos = 0;
        }
    } else if (gps_nmea_output_enabled && sync_nmea >= 3) {
        ubxg6010_handle_nmea_output(data);
    } else {
        ((uint8_t *) incoming_packet)[buffer_pos] = data;
        if ((buffer_pos >= sizeof(uBloxHeader) - 1) &&
            (buffer_pos - 1 == (incoming_packet->header.payloadSize + sizeof(uBloxHeader) + sizeof(uBloxChecksum)))) {
            ubxg6010_handle_packet((uBloxPacket *) incoming_packet);
            buffer_pos = 0;
            sync_ubx = 0;
        } else {
            buffer_pos++;
            if (buffer_pos >= sizeof(uBloxPacket) + sizeof(uBloxChecksum)) {
                buffer_pos = 0;
                sync_ubx = 0;
            }
        }
    }
}
