import { describe, it, expect } from "vitest";
import { PRESETS, getPreset } from "@/config/presets";

describe("PRESETS", () => {
  it("has the expected number of presets", () => {
    expect(PRESETS.length).toBeGreaterThanOrEqual(10);
  });

  it("every preset has a unique id", () => {
    const ids = PRESETS.map((p) => p.id);
    expect(new Set(ids).size).toBe(ids.length);
  });

  it("every preset has a label and description", () => {
    for (const preset of PRESETS) {
      expect(preset.label.length).toBeGreaterThan(0);
      expect(preset.description.length).toBeGreaterThan(0);
    }
  });

  it("every preset has valid hardware types", () => {
    const validTypes = ["RS41", "RS41_RSM4x4", "DFM17"];
    for (const preset of PRESETS) {
      expect(preset.hardware.length).toBeGreaterThan(0);
      for (const hw of preset.hardware) {
        expect(validTypes).toContain(hw);
      }
    }
  });

  it("presets no longer set hardware.type in overlay (hardware is selected separately)", () => {
    for (const preset of PRESETS) {
      expect(preset.overlay.hardware).toBeUndefined();
    }
  });

  it("includes a WSPR preset", () => {
    const wspr = PRESETS.find((p) => p.id === "wspr-rs41");
    expect(wspr).toBeDefined();
    expect(wspr?.overlay.sensors?.si5351_enable).toBe(true);
  });

  it("includes a multi-mode preset with APRS + Horus V3 + CATS", () => {
    const multi = PRESETS.find((p) => p.id === "multi-mode-rs41");
    expect(multi?.overlay.tx_modes?.radio_tx_aprs).toBe(true);
    expect(multi?.overlay.tx_modes?.radio_tx_horus_v3).toBe(true);
    expect(multi?.overlay.tx_modes?.radio_tx_cats).toBe(true);
  });

  it("includes a Horus V3 preset", () => {
    const v3 = PRESETS.find((p) => p.id === "horus-v3-rs41");
    expect(v3).toBeDefined();
    expect(v3?.overlay.tx_modes?.radio_tx_horus_v3).toBe(true);
  });

  it("includes a fox mode preset", () => {
    const fox = PRESETS.find((p) => p.id === "fox-mode-rs41");
    expect(fox).toBeDefined();
    expect(fox?.overlay.fox_mode?.enable_fox_mode).toBe(true);
  });
});

describe("getPreset", () => {
  it("finds a preset by id", () => {
    const preset = getPreset("aprs-rs41");
    expect(preset?.id).toBe("aprs-rs41");
    expect(preset?.hardware).toContain("RS41");
  });

  it("returns undefined for unknown id", () => {
    expect(getPreset("nonexistent")).toBeUndefined();
  });
});
