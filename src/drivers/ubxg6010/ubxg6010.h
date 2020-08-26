#ifndef __UBXG6010_H
#define __UBXG6010_H

#include <stdint.h>

#include "src/gps.h"

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

typedef union {
    uBloxNAVPVTPayload navpvt;
    uBloxCFGPRTPayload cfgprt;
    uBloxCFGMSGPayload cfgmsg;
    uBloxCFGNAV5Payload cfgnav5;
    uBloxNAVPOSLLHPayload navposllh;
    uBloxNAVSOLPayload navsol;
    uBloxNAVTIMEUTCPayload navtimeutc;
    uBloxACKACKayload ackack;
    uBloxCFGRSTPayload cfgrst;
    uBloxCFGRXMPayload cfgrxm;
} ubloxPacketData;

typedef struct __attribute__((packed)) {
    uBloxHeader header;
    ubloxPacketData data;
} uBloxPacket;

void ubxg6010_init();

void ubxg6010_get_current_gps_data(gps_data *data);

void ubxg6010_handle_incoming_byte(uint8_t data);

#endif
