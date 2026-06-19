import { describe, it, expect } from "vitest";
import type { Schema, SchemaField, SchemaSection } from "@/types/schema";
import {
  validateField,
  validateConfig,
  detectConflicts,
} from "@/config/validation";
import type { ConfigState } from "@/types/config";

// ─── validateField ─────────────────────────────────────────────────────────────

describe("validateField — string", () => {
  const field: SchemaField = {
    define: "CALLSIGN",
    type: "string",
    label: "Callsign",
    tier: "simple",
    default: "N0CALL",
    validation: { required: true, max_length: 6, pattern: "^[A-Z0-9]+$" },
  };

  it("passes a valid value", () => {
    expect(validateField(field, "OH2A")).toEqual([]);
  });

  it("fails when required and value is empty string", () => {
    const errors = validateField(field, "");
    expect(errors.length).toBeGreaterThan(0);
  });

  it("fails when required and value is undefined", () => {
    const errors = validateField(field, undefined);
    expect(errors.length).toBeGreaterThan(0);
  });

  it("fails when value exceeds max_length", () => {
    const errors = validateField(field, "TOOLONGCALL");
    expect(errors.some((e) => e.includes("at most 6"))).toBe(true);
  });

  it("fails when value does not match pattern", () => {
    const errors = validateField(field, "oh2a");
    expect(errors.some((e) => e.includes("format"))).toBe(true);
  });
});

describe("validateField — integer", () => {
  const field: SchemaField = {
    define: "TX_POWER",
    type: "integer",
    label: "TX Power",
    tier: "simple",
    default: 7,
    validation: { min: 0, max: 7 },
  };

  it("passes a valid value", () => {
    expect(validateField(field, 5)).toEqual([]);
  });

  it("fails when below min", () => {
    const errors = validateField(field, -1);
    expect(errors.some((e) => e.includes("at least 0"))).toBe(true);
  });

  it("fails when above max", () => {
    const errors = validateField(field, 8);
    expect(errors.some((e) => e.includes("at most 7"))).toBe(true);
  });

  it("passes for value at exact boundary", () => {
    expect(validateField(field, 0)).toEqual([]);
    expect(validateField(field, 7)).toEqual([]);
  });

  it("is optional when no validation.required", () => {
    expect(validateField(field, undefined)).toEqual([]);
  });
});

describe("validateField — bool", () => {
  const field: SchemaField = {
    define: "LEDS_ENABLE",
    type: "bool",
    label: "LEDs",
    tier: "intermediate",
    default: true,
  };

  it("passes true", () => expect(validateField(field, true)).toEqual([]));
  it("passes false", () => expect(validateField(field, false)).toEqual([]));
  it("passes undefined (optional)", () =>
    expect(validateField(field, undefined)).toEqual([]));
});

describe("validateField — enum", () => {
  const field: SchemaField = {
    define: "HARDWARE_TYPE",
    type: "enum",
    label: "Hardware",
    tier: "simple",
    default: "RS41",
    values: [
      { value: "RS41", label: "RS41" },
      { value: "DFM17", label: "DFM17" },
    ],
  };

  it("passes a valid enum value", () => {
    expect(validateField(field, "RS41")).toEqual([]);
  });

  it("fails for an invalid enum value", () => {
    const errors = validateField(field, "INVALID");
    expect(errors.length).toBeGreaterThan(0);
  });
});

describe("validateField — enum_constant with integers", () => {
  const field: SchemaField = {
    define: "GPS_DYNAMIC_MODEL",
    type: "enum_constant",
    label: "Dynamic Model",
    tier: "advanced",
    default: 6,
    values: [
      { value: 0, label: "Portable" },
      { value: 6, label: "Airborne <1g" },
    ],
  };

  it("passes integer value in list", () => {
    expect(validateField(field, 6)).toEqual([]);
  });

  it("passes value compared as string (numeric YAML)", () => {
    expect(validateField(field, 0)).toEqual([]);
  });

  it("fails for value not in list", () => {
    expect(validateField(field, 99)).length.greaterThan(0);
  });
});

describe("validateField — char", () => {
  const field: SchemaField = {
    define: "APRS_SSID",
    type: "char",
    label: "SSID",
    tier: "intermediate",
    default: "9",
    validation: { pattern: "^[0-9A-F]$" },
  };

  it("passes a valid single char", () => {
    expect(validateField(field, "9")).toEqual([]);
  });

  it("fails for more than one character", () => {
    expect(validateField(field, "AB")).length.greaterThan(0);
  });

  it("fails for char not matching pattern", () => {
    expect(validateField(field, "z")).length.greaterThan(0);
  });
});

describe("validateField — string_array", () => {
  const field: SchemaField = {
    define: "cw_message_templates",
    type: "string_array",
    file: "config.c",
    label: "CW Messages",
    tier: "intermediate",
    default: ["$cs $loc6 $altm"],
    validation: { max_item_length: 64 },
  };

  it("passes a valid array", () => {
    expect(validateField(field, ["$cs $altm"])).toEqual([]);
  });

  it("fails when an item exceeds max_item_length", () => {
    const longMsg = "A".repeat(65);
    const errors = validateField(field, [longMsg]);
    expect(errors.some((e) => e.includes("max length"))).toBe(true);
  });

  it("fails when value is not an array", () => {
    expect(validateField(field, "not an array")).length.greaterThan(0);
  });
});

describe("validateField — alias_of is always skipped", () => {
  const field: SchemaField = {
    define: "APRS_CALLSIGN",
    type: "string",
    label: "APRS Callsign",
    tier: "intermediate",
    alias_of: "global.callsign",
  };

  it("returns no errors regardless of value", () => {
    expect(validateField(field, undefined)).toEqual([]);
    expect(validateField(field, "anything")).toEqual([]);
  });
});

// ─── validateConfig ────────────────────────────────────────────────────────────

function makeValidationSchema(): Schema {
  return {
    conflicts: [
      {
        id: "gps_nmea_vs_i2c",
        message: "GPS NMEA conflicts with I2C",
        condition: "gps.nmea_output_enable && sensors.bmp280_enable",
      },
    ],
    global: {
      _section: "Global",
      _tier: "simple",
      _order: 0,
      callsign: {
        define: "CALLSIGN",
        type: "string",
        label: "Callsign",
        tier: "simple",
        default: "N0CALL",
        validation: { required: true, max_length: 6 },
      },
    } as unknown as SchemaSection,
    radio_si4032: {
      _section: "Radio",
      _tier: "simple",
      _order: 1,
      tx_power: {
        define: "TX_POWER",
        type: "integer",
        label: "TX Power",
        tier: "simple",
        default: 7,
        validation: { min: 0, max: 7 },
      },
      tx_aprs_count: {
        define: "TX_APRS_COUNT",
        type: "integer",
        label: "APRS Count",
        tier: "intermediate",
        default: 2,
        validation: { min: 1, max: 50 },
        visible_when: "radio_si4032.tx_aprs",
      },
    } as unknown as SchemaSection,
    gps: {
      _section: "GPS",
      _tier: "advanced",
      _order: 2,
      nmea_output_enable: {
        define: "GPS_NMEA_OUTPUT",
        type: "bool",
        label: "NMEA Output",
        tier: "advanced",
        default: false,
      },
    } as unknown as SchemaSection,
    sensors: {
      _section: "Sensors",
      _tier: "advanced",
      _order: 3,
      bmp280_enable: {
        define: "SENSOR_BMP280_ENABLE",
        type: "bool",
        label: "BMP280",
        tier: "advanced",
        default: false,
      },
    } as unknown as SchemaSection,
  };
}

describe("validateConfig", () => {
  it("returns valid=true for a valid config", () => {
    const schema = makeValidationSchema();
    const config: ConfigState = {
      global: { callsign: "OH2A" },
      radio_si4032: { tx_power: 5, tx_aprs: true, tx_aprs_count: 2 },
      gps: { nmea_output_enable: false },
      sensors: { bmp280_enable: false },
    };
    const result = validateConfig(schema, config);
    expect(result.valid).toBe(true);
    expect(result.fieldErrors).toEqual({});
    expect(result.conflicts).toEqual([]);
  });

  it("reports required field missing", () => {
    const schema = makeValidationSchema();
    const config: ConfigState = {
      global: { callsign: "" },
      radio_si4032: { tx_power: 5 },
      gps: { nmea_output_enable: false },
      sensors: { bmp280_enable: false },
    };
    const result = validateConfig(schema, config);
    expect(result.valid).toBe(false);
    expect(result.fieldErrors["global.callsign"]).toBeDefined();
  });

  it("skips invisible fields (visible_when is false)", () => {
    const schema = makeValidationSchema();
    // tx_aprs is false so tx_aprs_count should be invisible → not validated even if bad
    const config: ConfigState = {
      global: { callsign: "OH2A" },
      radio_si4032: { tx_power: 5, tx_aprs: false, tx_aprs_count: 999 },
      gps: { nmea_output_enable: false },
      sensors: { bmp280_enable: false },
    };
    const result = validateConfig(schema, config);
    expect(result.fieldErrors["radio_si4032.tx_aprs_count"]).toBeUndefined();
  });

  it("validates visible fields (visible_when is true)", () => {
    const schema = makeValidationSchema();
    const config: ConfigState = {
      global: { callsign: "OH2A" },
      radio_si4032: { tx_power: 5, tx_aprs: true, tx_aprs_count: 999 },
      gps: { nmea_output_enable: false },
      sensors: { bmp280_enable: false },
    };
    const result = validateConfig(schema, config);
    expect(result.fieldErrors["radio_si4032.tx_aprs_count"]).toBeDefined();
  });
});

// ─── detectConflicts ───────────────────────────────────────────────────────────

describe("detectConflicts", () => {
  const schema = makeValidationSchema();

  it("returns empty when no conflicts triggered", () => {
    const config: ConfigState = {
      gps: { nmea_output_enable: false },
      sensors: { bmp280_enable: false },
    };
    expect(detectConflicts(schema, config)).toEqual([]);
  });

  it("returns conflict message when condition is true", () => {
    const config: ConfigState = {
      gps: { nmea_output_enable: true },
      sensors: { bmp280_enable: true },
    };
    const conflicts = detectConflicts(schema, config);
    expect(conflicts).toHaveLength(1);
    expect(conflicts[0]).toContain("GPS NMEA");
  });

  it("returns empty when schema has no conflicts", () => {
    const noConflictSchema: Schema = { global: makeValidationSchema().global };
    const config: ConfigState = {};
    expect(detectConflicts(noConflictSchema, config)).toEqual([]);
  });
});
