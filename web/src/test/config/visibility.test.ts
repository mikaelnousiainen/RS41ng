import { describe, it, expect } from "vitest";
import type { Schema, SchemaField, SchemaSection } from "@/types/schema";
import {
  evaluateCondition,
  isFieldVisible,
  isSectionVisible,
} from "@/config/visibility";
import type { ConfigState } from "@/types/config";

const config: ConfigState = {
  hardware: { type: "RS41" },
  radio_si4032: { tx_aprs: true, tx_horus_v2: false, tx_cats: false },
  radio_si4063: { tx_aprs: false },
  sensors: { si5351_enable: false, bmp280_enable: false },
  gps: { nmea_output_enable: false },
};

describe("evaluateCondition", () => {
  it("evaluates a simple boolean reference", () => {
    expect(evaluateCondition("radio_si4032.tx_aprs", config)).toBe(true);
    expect(evaluateCondition("radio_si4032.tx_horus_v2", config)).toBe(false);
  });

  it("evaluates an equality check against a string literal", () => {
    expect(evaluateCondition("hardware.type == 'RS41'", config)).toBe(true);
    expect(evaluateCondition("hardware.type == 'DFM17'", config)).toBe(false);
  });

  it("evaluates logical OR", () => {
    expect(
      evaluateCondition(
        "radio_si4032.tx_aprs || radio_si4063.tx_aprs",
        config
      )
    ).toBe(true);
    expect(
      evaluateCondition(
        "radio_si4032.tx_horus_v2 || radio_si4063.tx_aprs",
        config
      )
    ).toBe(false);
  });

  it("evaluates logical AND", () => {
    expect(
      evaluateCondition(
        "hardware.type == 'RS41' && radio_si4032.tx_aprs",
        config
      )
    ).toBe(true);
    expect(
      evaluateCondition(
        "hardware.type == 'RS41' && radio_si4032.tx_horus_v2",
        config
      )
    ).toBe(false);
  });

  it("evaluates complex AND/OR with parentheses", () => {
    expect(
      evaluateCondition(
        "hardware.type == 'RS41' && (sensors.si5351_enable || sensors.bmp280_enable)",
        config
      )
    ).toBe(false);
  });

  it("returns true (visible) for undefined section.field", () => {
    expect(evaluateCondition("unknown.field", config)).toBe(false);
  });

  it("returns true on parse error", () => {
    expect(evaluateCondition("{{invalid expression}}", config)).toBe(true);
  });
});

describe("isFieldVisible", () => {
  it("returns true when field has no visible_when", () => {
    const field: Partial<SchemaField> = {
      define: "CALLSIGN",
      type: "string",
      label: "Callsign",
      tier: "simple",
    };
    expect(isFieldVisible(field as SchemaField, config)).toBe(true);
  });

  it("returns true when condition is true", () => {
    const field: Partial<SchemaField> = {
      define: "TX_APRS_COUNT",
      type: "integer",
      label: "APRS Count",
      tier: "intermediate",
      visible_when: "radio_si4032.tx_aprs",
    };
    expect(isFieldVisible(field as SchemaField, config)).toBe(true);
  });

  it("returns false when condition is false", () => {
    const field: Partial<SchemaField> = {
      define: "TX_HORUS_V2_COUNT",
      type: "integer",
      label: "Horus V2 Count",
      tier: "intermediate",
      visible_when: "radio_si4032.tx_horus_v2",
    };
    expect(isFieldVisible(field as SchemaField, config)).toBe(false);
  });
});

describe("isSectionVisible", () => {
  const schema: Schema = {
    aprs: {
      _section: "APRS",
      _tier: "intermediate",
      _order: 7,
      _visible_when: "radio_si4032.tx_aprs || radio_si4063.tx_aprs",
    } as unknown as SchemaSection,
    gps: {
      _section: "GPS",
      _tier: "advanced",
      _order: 2,
      // no _visible_when — always visible
    } as unknown as SchemaSection,
    radio_si4063: {
      _section: "Si4063",
      _tier: "simple",
      _order: 5,
      _visible_when: "hardware.type == 'DFM17'",
    } as unknown as SchemaSection,
  };

  it("returns true when section has no _visible_when", () => {
    expect(isSectionVisible(schema, "gps", config)).toBe(true);
  });

  it("returns true when section _visible_when condition is met", () => {
    expect(isSectionVisible(schema, "aprs", config)).toBe(true);
  });

  it("returns false when section _visible_when condition is not met", () => {
    expect(isSectionVisible(schema, "radio_si4063", config)).toBe(false);
  });

  it("returns true for unknown section key (graceful)", () => {
    expect(isSectionVisible(schema, "unknown_section", config)).toBe(true);
  });
});
