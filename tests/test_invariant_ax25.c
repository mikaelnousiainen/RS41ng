#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "src/codecs/ax25/ax25.h"

/* We need access to the decode function and the packet structure */
extern int ax25_decode(ax25_packet_t *packet, const uint8_t *data, size_t len);

START_TEST(test_information_field_overflow)
{
    /* Invariant: The information field in a decoded AX.25 packet must never
     * exceed the fixed buffer size, regardless of input packet length.
     * A malicious over-the-air packet with an oversized info field must not
     * cause a buffer overflow. */

    ax25_packet_t packet;
    memset(&packet, 0, sizeof(packet));

    /* Craft raw AX.25 frames with varying information field sizes.
     * AX.25 frame: dest(7) + src(7) + control(1) + PID(1) + info(variable)
     * Minimum header = 16 bytes */
    const size_t header_size = 16;
    size_t info_sizes[] = {
        10,                              /* valid short payload */
        sizeof(packet.information_field) - 1, /* boundary: exactly max */
        sizeof(packet.information_field),     /* boundary: one over */
        sizeof(packet.information_field) * 4, /* exploit: large overflow */
    };
    int num_cases = sizeof(info_sizes) / sizeof(info_sizes[0]);

    for (int i = 0; i < num_cases; i++) {
        size_t frame_len = header_size + info_sizes[i];
        uint8_t *frame = calloc(frame_len, 1);
        ck_assert_ptr_nonnull(frame);

        /* Fill header with minimal valid AX.25 addressing */
        /* Destination and source: 7 bytes each, SSID byte has end-of-address bit */
        memset(frame, 0x40, 14); /* space characters shifted left */
        frame[13] |= 0x01;      /* end of address marker */
        frame[14] = 0x03;       /* UI frame control */
        frame[15] = 0xF0;       /* no layer 3 PID */

        /* Fill info field with 'A' pattern */
        memset(frame + header_size, 'A', info_sizes[i]);

        memset(&packet, 0, sizeof(packet));
        int ret = ax25_decode(&packet, frame, frame_len);

        /* Security property: if decode succeeds, info field must be bounded */
        if (ret == 0) {
            size_t actual_len = strlen(packet.information_field);
            ck_assert_msg(actual_len < sizeof(packet.information_field),
                "Information field overflow: wrote %zu bytes into %zu-byte buffer",
                actual_len, sizeof(packet.information_field));
        }
        free(frame);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_information_field_overflow);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}