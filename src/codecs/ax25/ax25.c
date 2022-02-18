#include <string.h>
#include <stdbool.h>
#include "ax25.h"

static inline uint16_t ax25_calculate_crc_for_bit(uint16_t crc, bool bit)
{
    uint16_t result = crc;
    uint16_t temp;

    // XOR lsb of CRC with the latest bit
    temp = crc ^ (bit ? 1U : 0U);
    // Shift 16-bit CRC one bit to the right
    result >>= 1U;

    // If XOR result from above has lsb set
    if (temp & 0x0001U) {
        result ^= 0x8408U;
    }

    return result;
}

static inline uint16_t ax25_calculate_crc_for_byte(uint16_t crc, uint8_t byte)
{
    uint8_t temp = byte;
    for (uint16_t i = 0; i < 8; i++, temp >>= 1U) {
        bool bit = ((temp & 1U) == 1U) ? true : false;
        crc = ax25_calculate_crc_for_bit(crc, bit);
    }

    return crc;
}

static uint16_t ax25_calculate_crc(uint16_t length, uint8_t *data)
{
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        crc = ax25_calculate_crc_for_byte(crc, byte);
    }

    return crc;
}

static uint16_t ax25_encode_digipeater_path(char *input, char *packet_data)
{
    uint16_t digipeaters_length = strlen(input);
    uint16_t packet_data_index = 0;

    for (uint16_t index = 0; index < digipeaters_length; index++) {
        if (input[index] == ',' || index == digipeaters_length - 1) {
            if (input[index] != ',') {
                packet_data[packet_data_index] = input[index] == '-' ? ' ' : input[index];
                packet_data_index++;
            }

            uint16_t fill_count = 7 - (packet_data_index % 7);
            while (fill_count > 0 && fill_count < 7) {
                packet_data[packet_data_index] = ' ';
                fill_count--;
                packet_data_index++;
            }

            continue;
        }

        packet_data[packet_data_index] = input[index] == '-' ? ' ' : input[index];
        packet_data_index++;
    }

    return packet_data_index;
}

uint16_t ax25_encode_packet_aprs(char *source, uint8_t source_ssid, char *destination, uint8_t destination_ssid,
        char *digipeater_addresses, char *information_field, uint16_t length, uint8_t *packet_data)
{
    // TODO: use length to limit packet size

    ax25_packet_header *header = (ax25_packet_header *) packet_data;

    header->flag = AX25_PACKET_FLAG;

    size_t call_length = strlen(source);
    memset(header->source, ' ', sizeof(header->source));
    memcpy(header->source, source, call_length < 6 ? call_length : 6);
    header->source_ssid = (uint8_t) (source_ssid >= 'A' ? source_ssid - 7 : source_ssid);;

    call_length = strlen(destination);
    memset(header->destination, ' ', sizeof(header->destination));
    memcpy(header->destination, destination, call_length < 6 ? call_length : 6);
    header->destination_ssid = destination_ssid;

    char *digipeater_addresses_start = ((char *) header) + 1 + 14;
    uint16_t digipeater_addresses_length = ax25_encode_digipeater_path(digipeater_addresses, digipeater_addresses_start);

    // Perform bit-shifting for all addresses
    uint8_t *actual_data_start = ((uint8_t *) header) + 1;
    for (uint16_t i = 0; i < 14 + digipeater_addresses_length; i++) {
        actual_data_start[i] = actual_data_start[i] << 1U;
    }
    actual_data_start[13 + digipeater_addresses_length] |= 1U;

    ax25_packet_header_end *header_end = (ax25_packet_header_end *) (((uint8_t *) header) + 1 + 14 +
                                                                     digipeater_addresses_length);

    header_end->control_field = AX25_CONTROL_FIELD_UI_FRAME;
    header_end->protocol_id = AX25_PROTOCOL_ID_NO_LAYER_3;

    uint16_t info_length = strlen(information_field);
    strcpy(header_end->information_field, information_field);

    uint16_t crc_length = 14 + digipeater_addresses_length + 2 + info_length;
    uint16_t crc = ax25_calculate_crc(crc_length, actual_data_start);

    ax25_packet_footer *footer = (ax25_packet_footer *) (((uint8_t *) header_end->information_field) + info_length);

    // CRC is stored MSB first
    footer->frame_check_sequence[0] = (crc & 0xFFU) ^ 0xFFU;
    footer->frame_check_sequence[1] = (crc >> 8U) ^ 0xFFU;
    footer->flag = AX25_PACKET_FLAG;

    return 1 + 14 + digipeater_addresses_length + 2 + info_length + 2 + 1;
}

