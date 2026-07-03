#!/usr/bin/env python3
"""Fetch a Vaisala RS41 sonde's factory PTU calibration from SondeHub and
generate src/vaisala_boom_cal.h for the Vaisala boom reader (calibration mode 2).

Usage:
    python3 tools/fetch_vaisala_boom_cal.py [SERIAL]

If SERIAL is omitted, the script asks for it interactively. The serial is the
one printed on the sonde's sticker (e.g. V1221335). Calibration data is only
available if the sonde's original flight was received by radiosonde_auto_rx
stations that upload subframe data to SondeHub.

How it works: SondeHub archives the telemetry of received sondes, and some
records carry the sonde's full 816-byte calibration subframe (rs41_subframe,
uploaded by radiosonde_auto_rx). This script downloads the archive, forms a
byte-wise majority consensus over all subframe copies (to remove reception bit
errors), decodes the coefficients at the offsets of the publicly documented
RS41 calibration layout (rs1729/RS), sanity-checks them against the fixed
board constants, and writes src/vaisala_boom_cal.h.

The generated header is a LOCAL build input for your specific sonde -- do not
commit it. Enable it with SENSOR_VAISALA_BOOM_CAL_MODE 2 in the configuration.

Clean-room: uses only the publicly documented calibration layout; not derived
from RS41-NFW. License: GPL-2.0 (same as RS41ng).
"""

import base64
import collections
import gzip
import io
import json
import struct
import sys
import urllib.error
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

HISTORY_URL = 'https://sondehub-history.s3.amazonaws.com/serial/{serial}.json.gz'
SUBFRAME_LEN = 816  # 51 blocks x 16 bytes

# Byte offsets of the calibration values in the subframe (documented layout).
OFF_SERIAL = 13
OFF_RF1, OFF_RF2 = 61, 65      # reference resistors (750 / 1100 ohm)
OFF_CF1, OFF_CF2 = 69, 73      # reference capacitors (0 / 47 pF)
OFF_CO1, OFF_CALT1 = 77, 89    # air temperature: Taylor terms + cal/poly
OFF_CALH = 117                 # humidity capacitance normalisation (U0, U1)
OFF_MTXH = 125                 # humidity 7x6 calibration matrix
OFF_CO2, OFF_CALT2 = 293, 305  # module temperature: Taylor terms + cal/poly
OFF_CORHP, OFF_CORHT = 678, 698  # humidity pressure-correction terms


def f32(data, offset):
    return struct.unpack('<f', data[offset:offset + 4])[0]


def fmt(value):
    return repr(round(value, 9)) + 'f'


def fetch_history(serial):
    url = HISTORY_URL.format(serial=serial)
    print('Downloading %s ...' % url)
    try:
        with urllib.request.urlopen(url, timeout=60) as response:
            raw = response.read()
    except urllib.error.HTTPError as err:
        if err.code in (403, 404):
            sys.exit('No SondeHub history found for serial %r -- the sonde was '
                     'never received, or the serial is misspelled.' % serial)
        raise
    return json.load(io.TextIOWrapper(gzip.GzipFile(fileobj=io.BytesIO(raw)),
                                      encoding='utf-8'))


def consensus_subframe(records):
    frames = []
    for record in records:
        encoded = record.get('rs41_subframe')
        if not encoded:
            continue
        decoded = base64.b64decode(encoded)
        if len(decoded) == SUBFRAME_LEN:
            frames.append(decoded)
    if not frames:
        sys.exit('The SondeHub history has no rs41_subframe records: no '
                 'receiving station uploaded calibration data for this sonde. '
                 'Only calibration mode 1 (approximate) is possible.')
    print('Found %d subframe copies; forming byte-wise majority consensus.'
          % len(frames))
    return bytes(collections.Counter(frame[i] for frame in frames).most_common(1)[0][0]
                 for i in range(SUBFRAME_LEN))


def check(sub, serial):
    embedded = sub[OFF_SERIAL:OFF_SERIAL + 8].decode('ascii', 'replace').strip()
    if embedded != serial:
        sys.exit('Serial mismatch: subframe carries %r, expected %r -- refusing '
                 'to generate a calibration for the wrong sonde.' % (embedded, serial))
    for name, offset, expected in (('Rf1', OFF_RF1, 750.0), ('Rf2', OFF_RF2, 1100.0),
                                   ('Cf2', OFF_CF2, 47.0)):
        value = f32(sub, offset)
        if abs(value - expected) > 0.5:
            sys.exit('Sanity check failed: %s = %r (expected %r). The consensus '
                     'subframe looks corrupted.' % (name, value, expected))
    print('Sanity checks passed (serial %s, Rf1/Rf2/Cf2 as expected).' % embedded)


def generate_header(sub, serial):
    def triple(offset):
        return [f32(sub, offset + 4 * i) for i in range(3)]

    t_taylor, t_cal = triple(OFF_CO1), triple(OFF_CALT1)
    tu_taylor, tu_cal = triple(OFF_CO2), triple(OFF_CALT2)
    h_u = [f32(sub, OFF_CALH), f32(sub, OFF_CALH + 4)]
    mtx = [f32(sub, OFF_MTXH + 4 * i) for i in range(42)]
    corp = [f32(sub, OFF_CORHP + 4 * i) for i in range(3)]
    cort = [f32(sub, OFF_CORHT + 4 * i) for i in range(12)]

    lines = []
    push = lines.append
    push('/*')
    push(' * Per-sonde RS41 PTU factory calibration coefficients (calibration mode 2).')
    push(' *')
    push(' * GENERATED by tools/fetch_vaisala_boom_cal.py for sonde %s' % serial)
    push(' * from its SondeHub subframe archive on %s.'
         % datetime.now(timezone.utc).strftime('%Y-%m-%d'))
    push(' * These values are specific to this individual sonde -- do not commit.')
    push(' *')
    push(' * The calibration ALGORITHM is the publicly documented RS41 PTU method')
    push(' * (rs1729/RS get_T / get_RH2adv); these #defines are the per-sonde data')
    push(' * it uses. Clean-room: not derived from RS41-NFW. GPL-2.0.')
    push(' */')
    push('')
    push('#ifndef __VAISALA_BOOM_CAL_H')
    push('#define __VAISALA_BOOM_CAL_H')
    push('')
    push('// Reference resistors / capacitors (board constants, same for all sondes).')
    push('#define VBCAL_REF_R1   750.0f')
    push('#define VBCAL_REF_R2   1100.0f')
    push('#define VBCAL_REF_C1   0.0f')
    push('#define VBCAL_REF_C2   47.0f')
    push('')
    push('// Air-temperature (PT1000): R = Rc * T_CAL;')
    push('//   T = (T_T0 + T_T1*R + T_T2*R^2 + T_POLY0) * (1 + T_POLY1)')
    for i, name in enumerate(('T_T0', 'T_T1', 'T_T2')):
        push('#define VBCAL_%-8s %s' % (name, fmt(t_taylor[i])))
    for i, name in enumerate(('T_CAL', 'T_POLY0', 'T_POLY1')):
        push('#define VBCAL_%-8s %s' % (name, fmt(t_cal[i])))
    push('')
    push('// Humidity-module (heater) temperature: own Taylor terms and scale/poly.')
    for i, name in enumerate(('TU_T0', 'TU_T1', 'TU_T2')):
        push('#define VBCAL_%-8s %s' % (name, fmt(tu_taylor[i])))
    for i, name in enumerate(('TU_CAL', 'TU_POLY0', 'TU_POLY1')):
        push('#define VBCAL_%-8s %s' % (name, fmt(tu_cal[i])))
    push('')
    push('// Humidity: capacitance normalisation (U0, U1) + 7x6 calibration matrix.')
    push('#define VBCAL_H_U0     %s' % fmt(h_u[0]))
    push('#define VBCAL_H_U1     %s' % fmt(h_u[1]))
    push('#define VBCAL_H_MATRIX { ' + chr(92))
    for row in range(7):
        entries = ', '.join(fmt(v) for v in mtx[row * 6:(row + 1) * 6])
        push('    ' + entries + (', ' + chr(92) if row < 6 else ' }'))
    push('')
    push('// Humidity pressure-correction terms (only applied with a pressure reading).')
    push('#define VBCAL_H_CORP { ' + ', '.join(fmt(v) for v in corp) + ' }')
    push('#define VBCAL_H_CORT { ' + chr(92))
    for row in range(3):
        entries = ', '.join(fmt(v) for v in cort[row * 4:(row + 1) * 4])
        push('    ' + entries + (', ' + chr(92) if row < 2 else ' }'))
    push('')
    push('#endif')
    push('')
    return '\n'.join(lines)


def main():
    serial = sys.argv[1].strip() if len(sys.argv) > 1 else ''
    if not serial:
        serial = input('Sonde serial (from the sticker, e.g. V1221335): ').strip()
    if not serial:
        sys.exit('No serial given.')
    serial = serial.upper()

    records = fetch_history(serial)
    print('History contains %d telemetry records.' % len(records))
    sub = consensus_subframe(records)
    check(sub, serial)

    header_path = Path(__file__).resolve().parent.parent / 'src' / 'vaisala_boom_cal.h'
    header_path.write_text(generate_header(sub, serial), encoding='ascii', newline='\n')
    print('Wrote %s' % header_path)
    print('Set SENSOR_VAISALA_BOOM_CAL_MODE to 2 (and enable the boom sensors) to use it.')


if __name__ == '__main__':
    main()
