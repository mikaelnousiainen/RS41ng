## RS41 PCB Model Naming Convention

The format is **RSM4XY**, where:
- **X = 1** → RS41-SG mainboard (no pressure sensor)
- **X = 2** → RS41-SGP mainboard (with pressure sensor support, e.g. RPM411 daughter board)
- **Y = hardware revision** (1 through 5)

So RSM411/RSM421 are a pair (SG/SGP) at revision 1, RSM412/RSM422 at revision 2, and so on.

## Known Differences by Revision

### Revisions 1 and 2 (RSM411, RSM421, RSM412, RSM422) — "Old" boards

These are the well-documented, long-produced boards that most of the reverse engineering community has studied (bazjo's radiosonde_hardware repo is based on these):

- **MCU**: STM32F100C8 (ARM Cortex-M3, 24 MHz crystal, 64 KB flash, LQFP-48)
- **GPS**: u-blox UBX-G6010-ST (u-blox 6 generation). Uses the older UBX binary protocol with CFG-NAV5 messages for setting dynamic models.
- **Radio TX**: Silicon Labs Si4032
- **Firmware serial prefix**: Serials starting with **"V"** (per OM3BC)
- **Reference heating**: Controlled via the Si4032's GPIO pin (this was a detail that rs41-nfw v26 had to add back for older boards, since the new boards handle it differently)
- **Flash memory**: 64 KB — quite constrained for custom firmware (rs41-nfw changelog mentions code size optimization struggles to fit features on "RSM4x2" boards, with memory usage around 90%)

The distinction between rev 1 (RSM4x1) and rev 2 (RSM4x2) is minor and not well-documented in the community. RSM412 is by far the most commonly encountered in the wild, and is what radiosonde_auto_rx considers the "standard" RS41 mainboard. RSM4x1 may represent very early production runs.

### Revision 4 (RSM414, RSM424) — "New" boards, ~2023-2024

This is a major redesign that started appearing around 2023. First flagged in RS41ng issue #92, these contain a completely different MCU.

- **MCU**: **STM32L412KB** (ARM Cortex-M4 with FPU, 80 MHz, **128 KB flash**, ultra-low-power). This is a significant upgrade — double the flash, much faster core, and lower power consumption.
- **GPS**: **u-blox M10 series** (likely MAX-M10S or equivalent). As noted in the radiosonde_auto_rx v1.7.1 release, these new PCBs have a u-blox 10 GPS receiver. This uses the newer VALSET/VALGET configuration interface rather than the older CFG-NAV5 protocol.
- **Radio TX**: Still Si4032 (same radio IC retained)
- **Firmware serial prefix**: Serials starting with **"W"** (per OM3BC)
- **Pin compatibility**: The STM32L412 and STM32F100 are in the same LQFP-48 package, but as OM3BC notes, although the old and new processors are pin-compatible, the same pins are not used in all cases, meaning old firmware cannot run on the new hardware.
- **Reference heating**: Handled differently from the older boards (new boards don't use the Si4032 GPIO for this)
- **GPS protocol differences**: The u-blox M10 uses a different command set. The rs41-nfw project had to implement separate GPS dynamic model configuration paths — VALSET/VALGET for the M10 vs. the legacy UBX-CFG method for the older u-blox 6. The M10 also supports many more satellites (30+ simultaneously), power saving modes, and features like u-blox Super-S technology for better sensitivity.

### Revision 5 (RSM415, RSM425) — "Newest" boards, ~2025

These started appearing in 2025 and are supported by rs41-nfw from v55 onward. The RSM425 revision has been confirmed working by users (maxkup14). However, **the hardware of RSM425 has not yet been fully documented** — the rs41-nfw project notes this as a "to-do" item, as Nevvman hasn't had one to study in detail.

From a firmware perspective, RSM4x5 boards appear to behave identically to RSM4x4 — same MCU family (STM32L412), same GPS (u-blox M10), same radio (Si4032). The revision 5 may represent minor PCB layout tweaks, component substitutions, or manufacturing optimizations rather than a fundamental architectural change.

## Summary Table

| Feature | Rev 1-2 (RSM4x1/4x2) | Rev 4 (RSM4x4) | Rev 5 (RSM4x5) |
|---|---|---|---|
| **MCU** | STM32F100C8 (Cortex-M3, 64KB) | STM32L412KB (Cortex-M4, 128KB) | STM32L412KB (assumed) |
| **GPS** | u-blox 6 (UBX-G6010) | u-blox M10 | u-blox M10 (assumed) |
| **Radio** | Si4032 | Si4032 | Si4032 |
| **Flash** | 64 KB | 128 KB | 128 KB |
| **GPS config** | Legacy UBX-CFG | VALSET/VALGET | VALSET/VALGET |
| **Serial prefix** | V… | W… | W… |
| **Ref heating** | Via Si4032 GPIO | Direct MCU control | Direct MCU control |
| **Era** | ~2014–2023 | ~2023–2024 | ~2025+ |

Note that **revision 3 (RSM4x3) has not been mentioned anywhere** in the community — it may have been an internal prototype that was never mass-produced, or it simply hasn't been encountered/reported.

The key practical takeaway for the repurposing community is that the rev 4/5 transition broke firmware compatibility with the old boards, requiring entirely separate firmware builds or unified projects like rs41-nfw that abstract over the hardware differences.
