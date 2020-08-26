#include <string.h>

#include "hal/system.h"
#include "hal/usart_gps.h"
#include "hal/delay.h"

#include "ubxg6010.h"

gps_data current_gps_data;

volatile bool ack_received = false;
volatile bool nack_received = false;

static uBloxChecksum
ubxg6010_calculate_checksum(const uint8_t msgClass, const uint8_t msgId, const uint8_t *message, uint16_t size)
{
    uBloxChecksum ck = {0, 0};
    uint8_t i;
    ck.ck_a += msgClass;
    ck.ck_b += ck.ck_a;
    ck.ck_a += msgId;
    ck.ck_b += ck.ck_a;

    ck.ck_a += size & 0xff;
    ck.ck_b += ck.ck_a;
    ck.ck_a += size >> 8;
    ck.ck_b += ck.ck_a;


    for (i = 0; i < size; i++) {
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

void ubxg6010_get_current_gps_data(gps_data *data)
{
    system_disable_irq();
    memcpy(data, &current_gps_data, sizeof(gps_data));
    system_enable_irq();
}

void ubxg6010_init()
{
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
                    .navBbrMask=0xffff,
                    .resetMode=1,
                    .reserved1=0
            },
    };

    ubxg6010_send_packet(&msgcfgrst);
    delay_ms(800);

    usart_gps_init(9600, true);
    delay_ms(10);

    ubxg6010_send_packet(&msgcfgrst);
    delay_ms(800);

    uBloxPacket msgcgprt = {
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
                    .mode=0b00100011000000,
                    .baudRate=38400,
                    .inProtoMask=1,
                    .outProtoMask=1,
                    .flags=0,
                    .reserved2={0, 0}
            },
    };
    ubxg6010_send_packet(&msgcgprt);

    usart_gps_init(38400, true);
    delay_ms(10);

    uBloxPacket msgcfgrxm = {
            .header = {
                    0xb5,
                    0x62,
                    .messageClass=0x06,
                    .messageId=0x11,
                    .payloadSize=sizeof(uBloxCFGRXMPayload)
            },
            .data.cfgrxm = {
                    .reserved1=8,
                    .lpMode=4
            }
    };

    do {
        ubxg6010_send_packet(&msgcfgrxm);
    } while (!ubxg6010_wait_for_ack());

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

    do {
        ubxg6010_send_packet(&msgcfgmsg);
    } while (!ubxg6010_wait_for_ack());

    msgcfgmsg.data.cfgmsg.msgID = 0x6;
    do {
        ubxg6010_send_packet(&msgcfgmsg);
    } while (!ubxg6010_wait_for_ack());

    msgcfgmsg.data.cfgmsg.msgID = 0x21;
    do {
        ubxg6010_send_packet(&msgcfgmsg);
    } while (!ubxg6010_wait_for_ack());

    uBloxPacket msgcfgnav5 = {
            .header = {
                    0xb5,
                    0x62,
                    .messageClass=0x06,
                    .messageId=0x24,
                    .payloadSize=sizeof(uBloxCFGNAV5Payload)
            },
            .data.cfgnav5={
                    .mask=0b00000001111111111,
                    .dynModel=7,
                    .fixMode=2,
                    .fixedAlt=0,
                    .fixedAltVar=10000,
                    .minElev=5,
                    .drLimit=0,
                    .pDop=25,
                    .tDop=25,
                    .pAcc=100,
                    .tAcc=300,
                    .staticHoldThresh=0,
                    .dgpsTimeOut=2,
                    .reserved2=0,
                    .reserved3=0,
                    .reserved4=0
            },
    };

    do {
        ubxg6010_send_packet(&msgcfgnav5);
    } while (!ubxg6010_wait_for_ack());
}

#include "log.h"

static void ubxg6010_handle_packet(uBloxPacket *pkt)
{
    uBloxChecksum cksum = ubxg6010_calculate_checksum(pkt->header.messageClass, pkt->header.messageId,
            (const uint8_t *) &pkt->data, pkt->header.payloadSize);
    uBloxChecksum *checksum = (uBloxChecksum *) (((uint8_t *) &pkt->data) + pkt->header.payloadSize);

    if (cksum.ck_a != checksum->ck_a || cksum.ck_b != checksum->ck_b) {
        current_gps_data.bad_packets += 1;
        return;
    }

    if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x07) {
        log_info("class: %02X, id %02X\n", pkt->header.messageClass, pkt->header.messageId);
        current_gps_data.ok_packets += 1;
        current_gps_data.fix = pkt->data.navpvt.fixType;
        current_gps_data.lat_raw = pkt->data.navpvt.lat;
        current_gps_data.lon_raw = pkt->data.navpvt.lon;
        current_gps_data.alt_raw = pkt->data.navpvt.hMSL;
        current_gps_data.hours = pkt->data.navpvt.hour;
        current_gps_data.minutes = pkt->data.navpvt.min;
        current_gps_data.seconds = pkt->data.navpvt.sec;
        current_gps_data.sats_raw = pkt->data.navpvt.numSV;
        current_gps_data.speed_raw = pkt->data.navpvt.gSpeed;
    } else if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x02) {
        log_info("class: %02X, id %02X\n", pkt->header.messageClass, pkt->header.messageId);
        current_gps_data.ok_packets += 1;
        current_gps_data.lat_raw = pkt->data.navposllh.lat;
        current_gps_data.lon_raw = pkt->data.navposllh.lon;
        current_gps_data.alt_raw = pkt->data.navposllh.hMSL;
    } else if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x06) {
        log_info("class: %02X, id %02X\n", pkt->header.messageClass, pkt->header.messageId);
        log_info("flags: %02X\n", pkt->data.navsol.flags);
        log_info("SV: %d\n", pkt->data.navsol.numSV);
        current_gps_data.fix = pkt->data.navsol.gpsFix;
        current_gps_data.sats_raw = pkt->data.navsol.numSV;
    } else if (pkt->header.messageClass == 0x01 && pkt->header.messageId == 0x21) {
        log_info("class: %02X, id %02X\n", pkt->header.messageClass, pkt->header.messageId);
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
