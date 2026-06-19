## Key Finding: RSM425 Introduces a New Telemetry Format

The most important discovery is from **radiosonde_auto_rx v1.8.1** (released February 2025), which is explicitly described as an urgent update:

**RS41: Support for the new 'multi-GNSS' telemetry format that has been observed on very new RS41 models (X-prefix). We highly recommend everyone update to 1.8.1 to ensure these newer RS41s are received correctly.** Thanks to rs1729 for quickly adding support for this new format!

The specifics:

- This new telemetry format has only been seen on 'X' prefix RS41s (RSM425 PCB, 02.07.01 software version), but note that it is only used if a Vaisala MW51 ground station is used. We may see X-prefix RS41s not using this new telemetry format.

So RSM425 **is** handled differently at the decoder level. Here's a breakdown of what's new:

### New Serial Number Prefix: "X"

RSM425 units use serial numbers starting with **"X"** — a new prefix not seen on earlier revisions. This is in addition to the known prefixes:

- Older boards (RSM4x1/4x2): serials starting with letters like **S**, **P**, **R**, **T**, etc.
- RSM4x4 boards: serials starting with **"W"** (per OM3BC's documentation)
- RSM4x5 boards: serials starting with **"X"**

### New "Multi-GNSS" Telemetry Format

The RSM425 running Vaisala firmware version **02.07.01** introduces a fundamentally new GPS telemetry block format. The older RS41 telemetry uses three GPS sub-blocks (GPS1, GPS2, GPS3) that carry UBX-style raw GPS data (pseudoranges, Doppler, satellite info, and navigation solution), all based on GPS-only positioning. The new format apparently uses a **multi-GNSS** approach — meaning the sonde's u-blox M10 receiver is now reporting data from multiple constellations (GPS, GLONASS, Galileo, BeiDou), and the telemetry frame structure has been altered to accommodate this.

This required decoder changes by rs1729 in the rs41mod decoder used by auto_rx.

### Conditional Activation

Importantly, this new telemetry format is **not always active** on RSM425 boards. It is only used when the sonde is configured by a **Vaisala MW51 ground station** (the newer sounding system). This means:

- RSM425 sondes configured with an older MW41 ground station may still use the traditional GPS-only telemetry format
- The format change is a **firmware/configuration** difference, not purely a hardware one
- Without the auto_rx v1.8.1 update, these X-prefix sondes with the new format would **fail to decode GPS data correctly** (the decoder would read gibberish from the new frame layout)

### Vaisala's Multi-GNSS Marketing

This aligns with Vaisala's own product messaging. Vaisala's RS41 product page now highlights "Multi-GNSS technology in Vaisala's standard RS41 radiosonde models uses data from multiple navigation satellites, increasing resilience" against GPS interference and cyberattacks — this is clearly a reference to the RSM425 capability.

## Updated Summary

So RSM4x5 **is** meaningfully different from RSM4x4, not just a minor PCB tweak:

| Aspect                        | RSM4x4                                 | RSM4x5                                                     |
| ----------------------------- | -------------------------------------- | ---------------------------------------------------------- |
| **Serial prefix**             | W…                                     | **X…**                                                     |
| **Vaisala FW version**        | Earlier                                | **02.07.01** (at least)                                    |
| **Telemetry format**          | Traditional GPS-only blocks (GPS1/2/3) | **New multi-GNSS format** (when paired with MW51)          |
| **GNSS constellations**       | GPS-only in telemetry                  | **Multi-constellation** (GPS + GLONASS + Galileo + BeiDou) |
| **Decoder impact**            | Standard rs41mod decoding              | **Required decoder update** (auto_rx v1.8.1+)              |
| **Ground station dependency** | MW41 or MW51                           | New format **only activated with MW51**                    |

The hardware (STM32L412, u-blox M10, Si4032) is likely the same between rev 4 and rev 5 — the u-blox M10 already supports multi-GNSS natively. The difference is that rev 5 ships with updated Vaisala firmware that actually leverages the multi-GNSS capability and transmits it in a new telemetry frame format, which in turn required all the community decoders to be updated.
