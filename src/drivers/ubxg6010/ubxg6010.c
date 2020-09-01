#include <string.h>

#include "hal/system.h"
#include "hal/usart_gps.h"
#include "hal/delay.h"

#include "ubxg6010.h"
#include "log.h"

gps_data current_gps_data;

volatile bool ack_received = false;
volatile bool nack_received = false;

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

static bool ubxg6010_wait_for_ack()
{
    ack_received = false;
    nack_received = false;
    uint8_t timeout = 200;

    while (!ack_received && !nack_received) {
        delay_ms(1);
        if (!timeout--) {
            break;
        }
    }

    return ack_received;
}

void ubxg6010_send_command(uint8_t msgClass, uint8_t msgId, uint8_t *payload, uint16_t payloadSize)
{
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

void ubxg6010_get_current_gps_data(gps_data *data)
{
    system_disable_irq();
    memcpy(data, &current_gps_data, sizeof(gps_data));
    system_enable_irq();
}

bool ubxg6010_init()
{
    bool success;

    usart_gps_init(38400, true);
    delay_ms(10);

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

    ubxg6010_send_packet(&msgcfgrst);
    delay_ms(800);

    usart_gps_init(9600, true);
    delay_ms(10);

    ubxg6010_send_packet(&msgcfgrst);
    delay_ms(800);

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
    ubxg6010_send_packet(&msgcfgprt);

    usart_gps_init(38400, true);
    delay_ms(10);

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

    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgrxm);
    if (!success) {
        return false;
    }

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

    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgmsg);
    if (!success) {
        return false;
    }

    // Rate of 1 for message: 0x01 0x06 Navigation Solution Information
    msgcfgmsg.data.cfgmsg.msgID = 0x6;
    msgcfgmsg.data.cfgmsg.rate = 1;
    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgmsg);
    if (!success) {
        return false;
    }

    // Configure rate of 1 for message: 0x01 0x21 UTC Time Solution
    msgcfgmsg.data.cfgmsg.msgID = 0x21;
    msgcfgmsg.data.cfgmsg.rate = 1;
    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgmsg);
    if (!success) {
        return false;
    }

    // Configure rate of 2 for message: 0x01 0x12 Velocity Solution in NED
    msgcfgmsg.data.cfgmsg.msgID = 0x12;
    msgcfgmsg.data.cfgmsg.rate = 2;
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
            .data.cfgnav5={
                    .mask=0b0000001111111111, // Configure all settings
                    .dynModel=7, // Dynamic model: Airborne with <2g Acceleration
                    .fixMode=2, // Fix mode: 3D only
                    .fixedAlt=0, // Fixed altitude (mean sea level) for 2D fix mode.
                    .fixedAltVar=10000, // Fixed altitude variance for 2D mode.
                    .minElev=5, // Minimum Elevation for a GNSS satellite to be used in NAV (degrees)
                    .drLimit=0, // Maximum time to perform dead reckoning (linear extrapolation) in case of GPS signal loss (seconds)
                    .pDop=25, // Position DOP Mask to use
                    .tDop=25, // Time DOP Mask to use
                    .pAcc=100, // Position Accuracy Mask (m)
                    .tAcc=300, // Time Accuracy Mask (m)
                    .staticHoldThresh=0, // Static hold threshold (cm/s)
                    .dgpsTimeOut=2, // DGPS timeout, firmware 7 and newer only
                    .reserved2=0,
                    .reserved3=0,
                    .reserved4=0
            },
    };

    success = ubxg6010_send_packet_and_wait_for_ack(&msgcfgnav5);
    if (!success) {
        return false;
    }

    return true;
}

static void ubxg6010_handle_packet(uBloxPacket *pkt)
{
    uBloxChecksum cksum = ubxg6010_calculate_checksum(pkt->header.messageClass, pkt->header.messageId,
            (const uint8_t *) &pkt->data, pkt->header.payloadSize);
    uBloxChecksum *checksum = (uBloxChecksum *) (((uint8_t *) &pkt->data) + pkt->header.payloadSize);

    if (cksum.ck_a != checksum->ck_a || cksum.ck_b != checksum->ck_b) {
        current_gps_data.bad_packets += 1;
        return;
    }

    if (pkt->header.messageClass == 0x01) {
        log_info("class: %02X, id %02X\n", pkt->header.messageClass, pkt->header.messageId);
    }

    if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x07) {
        // TODO: It seems NAV PVT message is not supported by UBXG6010, confirm this
        log_info("class: %02X, id %02X, fix: %d\n", pkt->header.messageClass, pkt->header.messageId,
                pkt->data.navpvt.fixType);
        current_gps_data.ok_packets += 1;
        current_gps_data.time_of_week_millis = pkt->data.navpvt.iTOW;
        current_gps_data.year = pkt->data.navpvt.year;
        current_gps_data.month = pkt->data.navpvt.month;
        current_gps_data.day = pkt->data.navpvt.day;
        current_gps_data.hours = pkt->data.navpvt.hour;
        current_gps_data.minutes = pkt->data.navpvt.min;
        current_gps_data.seconds = pkt->data.navpvt.sec;

        current_gps_data.fix = pkt->data.navpvt.fixType;
        current_gps_data.lat_raw = pkt->data.navpvt.lat;
        current_gps_data.lon_raw = pkt->data.navpvt.lon;
        current_gps_data.alt_raw = pkt->data.navpvt.hMSL;
        current_gps_data.sats_raw = pkt->data.navpvt.numSV;
        current_gps_data.speed_raw = pkt->data.navpvt.gSpeed;
        current_gps_data.heading_raw = pkt->data.navpvt.headMot;
        current_gps_data.climb_raw = -pkt->data.navpvt.velD;
    } else if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x12) {
        log_info("class: %02X, id %02X\n", pkt->header.messageClass, pkt->header.messageId);
        current_gps_data.ok_packets += 1;
        current_gps_data.time_of_week_millis = pkt->data.navvelned.iTOW;
        current_gps_data.speed_raw = pkt->data.navvelned.gSpeed;
        current_gps_data.heading_raw = pkt->data.navvelned.headMot;
        current_gps_data.climb_raw = -pkt->data.navvelned.velD;
    } else if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x02) {
        log_info("class: %02X, id %02X\n", pkt->header.messageClass, pkt->header.messageId);
        current_gps_data.ok_packets += 1;
        current_gps_data.time_of_week_millis = pkt->data.navposllh.iTOW;
        current_gps_data.lat_raw = pkt->data.navposllh.lat;
        current_gps_data.lon_raw = pkt->data.navposllh.lon;
        current_gps_data.alt_raw = pkt->data.navposllh.hMSL;
    } else if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x06) {
        log_info("class: %02X, id %02X, flags: %02X, SV: %d, fix: %d\n", pkt->header.messageClass,
                pkt->header.messageId,
                pkt->data.navsol.flags, pkt->data.navsol.numSV, pkt->data.navsol.gpsFix);
        current_gps_data.time_of_week_millis = pkt->data.navsol.iTOW;
        current_gps_data.week = pkt->data.navsol.week;
        current_gps_data.fix = pkt->data.navsol.gpsFix;
        current_gps_data.sats_raw = pkt->data.navsol.numSV;
    } else if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x21) {
        log_info("class: %02X, id %02X\n", pkt->header.messageClass, pkt->header.messageId);
        current_gps_data.year = pkt->data.navtimeutc.year;
        current_gps_data.month = pkt->data.navtimeutc.month;
        current_gps_data.day = pkt->data.navtimeutc.day;
        current_gps_data.hours = pkt->data.navtimeutc.hour;
        current_gps_data.minutes = pkt->data.navtimeutc.min;
        current_gps_data.seconds = pkt->data.navtimeutc.sec;
    } else if (pkt->header.messageClass == 0x05 && pkt->header.messageId == 0x01) {
        ack_received = true;
    } else if (pkt->header.messageClass == 0x05 && pkt->header.messageId == 0x00) {
        nack_received = true;
    }
}

void ubxg6010_handle_incoming_byte(uint8_t data)
{
    static uint8_t sync = 0;
    static uint8_t buffer_pos = 0;
    static uint8_t incoming_packet_buffer[sizeof(uBloxPacket) + sizeof(uBloxChecksum)];
    static uBloxPacket *incoming_packet = (uBloxPacket *) incoming_packet_buffer;

    if (!sync) {
        if (!buffer_pos && data == 0xB5) {
            buffer_pos = 1;
            incoming_packet->header.sc1 = data;
        } else if (buffer_pos == 1 && data == 0x62) {
            sync = 1;
            buffer_pos = 2;
            incoming_packet->header.sc2 = data;
        } else {
            buffer_pos = 0;
        }
    } else {
        ((uint8_t *) incoming_packet)[buffer_pos] = data;
        if ((buffer_pos >= sizeof(uBloxHeader) - 1) &&
            (buffer_pos - 1 == (incoming_packet->header.payloadSize + sizeof(uBloxHeader) + sizeof(uBloxChecksum)))) {
            ubxg6010_handle_packet((uBloxPacket *) incoming_packet);
            buffer_pos = 0;
            sync = 0;
        } else {
            buffer_pos++;
            if (buffer_pos >= sizeof(uBloxPacket) + sizeof(uBloxChecksum)) {
                buffer_pos = 0;
                sync = 0;
            }
        }
    }
}
