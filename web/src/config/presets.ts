import type { ConfigState } from "@/types/config";

export interface Preset {
  id: string;
  label: string;
  description: string;
  /** Hardware types this preset is compatible with */
  hardware: ("RS41" | "RS41_RSM4x4" | "DFM17")[];
  /** Partial config overlay applied on top of defaults */
  overlay: ConfigState;
}

/**
 * Built-in preset configurations (partial overlays applied on top of defaults).
 * Only fields that differ from schema defaults need to be included.
 */
export const PRESETS: Preset[] = [
  // --- RS41 presets (also compatible with RS41_RSM4x4) ---
  {
    id: "aprs-rs41",
    label: "APRS only (RS41)",
    description: "APRS 1200 baud on 70cm. Good starting point for most use cases.",
    hardware: ["RS41", "RS41_RSM4x4"],
    overlay: {
      tx_modes: {
        radio_tx_aprs: true,
        radio_tx_frequency_aprs_1200: 432500000,
        radio_tx_aprs_count: 1,
        radio_tx_horus_v2: false,
        radio_tx_horus_v3: false,
        radio_tx_cats: false,
        radio_tx_cw: false,
        radio_tx_pip: false,
      },
      radio_si4032: { tx_power: 7 },
    },
  },
  {
    id: "horus-v3-rs41",
    label: "Horus V3 (RS41)",
    description:
      "Horus 4FSK V3 digital mode. Recommended primary mode for balloon recovery.",
    hardware: ["RS41", "RS41_RSM4x4"],
    overlay: {
      tx_modes: {
        radio_tx_horus_v3: true,
        radio_tx_frequency_horus_v3: 432501000,
        radio_tx_horus_v3_count: 5,
        radio_tx_aprs: false,
        radio_tx_horus_v2: false,
        radio_tx_cats: false,
        radio_tx_cw: false,
        radio_tx_pip: false,
      },
      radio_si4032: { tx_power: 5 },
    },
  },
  {
    id: "horus-v2-rs41",
    label: "Horus V2 (RS41)",
    description:
      "Horus 4FSK V2 digital mode. Deprecated — use V3 for new setups.",
    hardware: ["RS41", "RS41_RSM4x4"],
    overlay: {
      tx_modes: {
        radio_tx_horus_v2: true,
        radio_tx_frequency_horus_v2: 432501000,
        radio_tx_horus_v2_count: 1,
        radio_tx_aprs: false,
        radio_tx_horus_v3: false,
        radio_tx_cats: false,
        radio_tx_cw: false,
        radio_tx_pip: false,
      },
      radio_si4032: { tx_power: 5 },
      horus_v2: { payload_id: 256 },
    },
  },
  {
    id: "cats-rs41",
    label: "CATS (RS41)",
    description: "CATS digital mode — high channel capacity, rapid beacon rate.",
    hardware: ["RS41", "RS41_RSM4x4"],
    overlay: {
      tx_modes: {
        radio_tx_cats: true,
        radio_tx_frequency_cats: 434100000,
        radio_tx_cats_count: 1,
        radio_tx_aprs: false,
        radio_tx_horus_v2: false,
        radio_tx_horus_v3: false,
        radio_tx_cw: false,
        radio_tx_pip: false,
      },
      radio_si4032: { tx_power: 7 },
      cats: { ssid: 11, icon: 13, is_balloon: true },
    },
  },
  {
    id: "multi-mode-rs41",
    label: "Multi-mode (RS41)",
    description: "APRS + Horus V3 + CATS combined. Maximum interoperability.",
    hardware: ["RS41", "RS41_RSM4x4"],
    overlay: {
      tx_modes: {
        radio_tx_aprs: true,
        radio_tx_frequency_aprs_1200: 432500000,
        radio_tx_aprs_count: 2,
        radio_tx_horus_v3: true,
        radio_tx_frequency_horus_v3: 432501000,
        radio_tx_horus_v3_count: 2,
        radio_tx_cats: true,
        radio_tx_frequency_cats: 434100000,
        radio_tx_cats_count: 1,
        radio_tx_horus_v2: false,
        radio_tx_cw: false,
        radio_tx_pip: false,
      },
      radio_si4032: { tx_power: 7 },
      cats: { ssid: 11, icon: 13, is_balloon: true },
    },
  },
  {
    id: "fox-mode-rs41",
    label: "Fox Mode (RS41)",
    description:
      "Fox hunting / transmitter hunting mode. Disables GPS and uses CW/pip/tone.",
    hardware: ["RS41", "RS41_RSM4x4"],
    overlay: {
      fox_mode: { enable_fox_mode: true },
      tx_modes: {
        radio_tx_cw: true,
        radio_tx_frequency_cw: 432501000,
        radio_tx_cw_count: 1,
        radio_tx_pip: true,
        radio_tx_frequency_pip: 432501000,
        radio_tx_pip_count: 6,
        radio_tx_aprs: false,
        radio_tx_horus_v2: false,
        radio_tx_horus_v3: false,
        radio_tx_cats: false,
      },
      radio_si4032: { tx_power: 7 },
    },
  },
  {
    id: "wspr-rs41",
    label: "HF WSPR (RS41 + Si5351)",
    description:
      "WSPR on HF via external Si5351. Requires Si5351 board connected.",
    hardware: ["RS41", "RS41_RSM4x4"],
    overlay: {
      tx_modes: {
        radio_tx_aprs: false,
        radio_tx_horus_v2: false,
        radio_tx_horus_v3: false,
        radio_tx_cats: false,
        radio_tx_cw: false,
        radio_tx_pip: false,
      },
      radio_si4032: { tx_power: 7 },
      sensors: { si5351_enable: true },
      radio_si5351: {
        tx_power: 3,
        tx_cw: false,
        tx_pip: false,
        tx_horus_v2: false,
        tx_jt9: false,
        tx_jt65: false,
        tx_jt4: false,
        tx_wspr: true,
        tx_fsq: false,
        tx_ft8: false,
      },
      wspr: {
        frequency: 14085000,
        dbm: 10,
        time_sync_seconds: 120,
        time_sync_offset_seconds: 1,
      },
    },
  },

  // --- DFM-17 presets ---
  {
    id: "aprs-dfm17",
    label: "APRS only (DFM-17)",
    description: "APRS 1200 baud on 70cm for the Graw DFM-17 sonde.",
    hardware: ["DFM17"],
    overlay: {
      tx_modes: {
        radio_tx_aprs: true,
        radio_tx_frequency_aprs_1200: 432500000,
        radio_tx_aprs_count: 1,
        radio_tx_horus_v2: false,
        radio_tx_horus_v3: false,
        radio_tx_cats: false,
        radio_tx_cw: false,
        radio_tx_pip: false,
      },
      radio_si4063: { tx_power: 127 },
    },
  },
  {
    id: "horus-v3-dfm17",
    label: "Horus V3 (DFM-17)",
    description:
      "Horus 4FSK V3 digital mode on the Graw DFM-17 sonde.",
    hardware: ["DFM17"],
    overlay: {
      tx_modes: {
        radio_tx_horus_v3: true,
        radio_tx_frequency_horus_v3: 432501000,
        radio_tx_horus_v3_count: 5,
        radio_tx_aprs: false,
        radio_tx_horus_v2: false,
        radio_tx_cats: false,
        radio_tx_cw: false,
        radio_tx_pip: false,
      },
      radio_si4063: { tx_power: 32 },
    },
  },
  {
    id: "cats-dfm17",
    label: "CATS (DFM-17)",
    description: "CATS digital mode on the Graw DFM-17 sonde.",
    hardware: ["DFM17"],
    overlay: {
      tx_modes: {
        radio_tx_cats: true,
        radio_tx_frequency_cats: 434100000,
        radio_tx_cats_count: 1,
        radio_tx_aprs: false,
        radio_tx_horus_v2: false,
        radio_tx_horus_v3: false,
        radio_tx_cw: false,
        radio_tx_pip: false,
      },
      radio_si4063: { tx_power: 127 },
      cats: { ssid: 11, icon: 13, is_balloon: true },
    },
  },
];

/** Get a preset by id */
export function getPreset(id: string): Preset | undefined {
  return PRESETS.find((p) => p.id === id);
}
