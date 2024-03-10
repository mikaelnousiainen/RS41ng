#include <string.h>
#include <assert.h>

#include "whisker.h"

#ifdef RS41
#define HARDWARE_ID 0x7d00
#endif
#ifdef DFM17
#define HARDWARE_ID 0x7d01
#endif

#define SOFTWARE_ID 0x00

static inline void cats_push_f16(cats_packet *p, float val)
{
    union {
        float f;
        uint32_t u;
    } fu = { .f = val };

    assert(sizeof fu.f == sizeof fu.u);

    uint32_t f32 = fu.u;
    uint32_t sign = (f32 >> 16) & 0x8000;
    uint32_t exponent = ((f32 >> 23) - 127 + 15) & 0x1F;
    uint32_t mantissa = f32 & 0x007FFFFF;
    uint32_t f16 = sign | (exponent << 10) | (mantissa >> 13);

    p->data[p->len++] = f16;
    p->data[p->len++] = f16 >> 8;
}

static inline void cats_push_f32(cats_packet *p, float val)
{
    union {
        float f;
        uint32_t u;
    } fu = { .f = val };

    assert(sizeof fu.f == sizeof fu.u);

    uint32_t f32 = fu.u;
    p->data[p->len++] = f32;
    p->data[p->len++] = f32 >> 8;
    p->data[p->len++] = f32 >> 16;
    p->data[p->len++] = f32 >> 24;
}

static inline void cats_push_u16(cats_packet *p, uint16_t val)
{
    for(int i = 0; i < 2; i++) {
        p->data[p->len++] = val >> (i * 8);
    }
}

static inline void cats_push_u32(cats_packet *p, uint32_t val)
{
    
    for(int i = 0; i < 4; i++) {
        p->data[p->len++] = val >> (i * 8);
    }
}

void cats_append_identification_whisker(cats_packet *packet, char *callsign, uint8_t ssid, uint16_t icon)
{
    packet->data[packet->len++] = 0x00; // type = identidication
    size_t callsign_len = strlen(callsign);
    packet->data[packet->len++] = callsign_len + 3; // len
    packet->data[packet->len++] = icon & 0xFF; // icon
    packet->data[packet->len++] = (icon >> 8) & 0xFF; // icon
    memcpy(packet->data + packet->len, callsign, callsign_len); //callsign
    packet->len += callsign_len;
    packet->data[packet->len++] = ssid; // ssid
}

void cats_append_gps_whisker(cats_packet *packet, gps_data gps)
{
    packet->data[packet->len++] = 0x02; // type = gps
    packet->data[packet->len++] = 14; // len
    int32_t lat_converted = gps.latitude_degrees_1000000 * (1LL<<31) / 90LL / 10000000LL;
    int32_t lon_converted = gps.longitude_degrees_1000000 * (1LL<<31) / 180LL / 10000000LL;
    cats_push_u32(packet, lat_converted); // lat
    cats_push_u32(packet, lon_converted); // lon
    cats_push_f16(packet, gps.altitude_mm / 1000.0); // alt
    packet->data[packet->len++] = 0; // max error = 0m
    packet->data[packet->len++] = gps.heading_degrees_100000 / 100000.0 * 128.0 / 180.0; // heading
    cats_push_f16(packet, gps.ground_speed_cm_per_second / 100.0); // horizontal speed
}

void cats_append_comment_whisker(cats_packet *packet, char *comment)
{
    packet->data[packet->len++] = 0x03; // type = comment
    size_t comment_len = strlen(comment);
    assert(comment_len <= 255);
    packet->data[packet->len++] = comment_len;
    memcpy(packet->data + packet->len, comment, comment_len);
    packet->len += comment_len;
}

void cats_append_node_info_whisker(cats_packet *packet, telemetry_data *data)
{
    uint8_t *d = packet->data;
    size_t *l = &packet->len;

    bool has_altitude = GPS_HAS_FIX(data->gps);
    float altitude = data->gps.altitude_mm / 1000.0;

    d[(*l)++] = 0x09; // type = node info
    d[(*l)++] = has_altitude ? 14 : 10; // len
    // bitmask
    d[(*l)++] = 0;
    d[(*l)++] = (CATS_IS_BALLOON << 2) | (has_altitude << 1);
    d[(*l)++] = 0b11110011;

    cats_push_u16(packet, HARDWARE_ID);
    d[(*l)++] = SOFTWARE_ID;
    d[(*l)++] = 21; // vertical antenna -> ~5.25 dBi gain
    d[(*l)++] = CATS_REPORTED_TX_POWER_DBM * 4.0; // TX power
    d[(*l)++] = data->battery_voltage_millivolts / 100; // voltage
    d[(*l)++] = data->internal_temperature_celsius_100 / 100; // transmitter temperature
    if(has_altitude) {
        cats_push_f32(packet, altitude);
    }
}
