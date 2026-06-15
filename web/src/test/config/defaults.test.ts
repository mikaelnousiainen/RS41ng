import { describe, it, expect } from "vitest";
import type { Schema, SchemaSection } from "@/types/schema";
import { buildDefaults } from "@/config/defaults";

function makeTestSchema(): Schema {
  return {
    hardware: {
      _section: "Hardware",
      _tier: "simple",
      _order: 0,
      type: {
        define: "RS41",
        define_style: "toggle",
        type: "enum",
        label: "Hardware Type",
        tier: "simple",
        default: "RS41",
        values: [{ value: "RS41", label: "RS41" }],
      },
    } as unknown as SchemaSection,
    global: {
      _section: "Global",
      _tier: "simple",
      _order: 1,
      callsign: {
        define: "CALLSIGN",
        type: "string",
        label: "Callsign",
        tier: "simple",
        default: "N0CALL",
      },
      leds_enable: {
        define: "LEDS_ENABLE",
        type: "bool",
        label: "LEDs",
        tier: "intermediate",
        default: true,
      },
      tx_power: {
        define: "TX_POWER",
        type: "integer",
        label: "TX Power",
        tier: "simple",
        default: 7,
      },
    } as unknown as SchemaSection,
    aprs: {
      _section: "APRS",
      _tier: "intermediate",
      _order: 2,
      // alias_of field — should be skipped
      callsign: {
        define: "APRS_CALLSIGN",
        type: "string",
        label: "APRS Callsign",
        tier: "intermediate",
        alias_of: "global.callsign",
      },
      ssid: {
        define: "APRS_SSID",
        type: "char",
        label: "SSID",
        tier: "intermediate",
        default: "9",
      },
    } as unknown as SchemaSection,
  };
}

describe("buildDefaults", () => {
  it("includes defaults for all non-alias fields", () => {
    const schema = makeTestSchema();
    const defaults = buildDefaults(schema);
    expect(defaults.global?.callsign).toBe("N0CALL");
    expect(defaults.global?.leds_enable).toBe(true);
    expect(defaults.global?.tx_power).toBe(7);
    expect(defaults.hardware?.type).toBe("RS41");
    expect(defaults.aprs?.ssid).toBe("9");
  });

  it("skips alias_of fields", () => {
    const schema = makeTestSchema();
    const defaults = buildDefaults(schema);
    expect(defaults.aprs?.callsign).toBeUndefined();
  });

  it("sections are ordered by _order", () => {
    const schema = makeTestSchema();
    const defaults = buildDefaults(schema);
    const keys = Object.keys(defaults);
    expect(keys.indexOf("hardware")).toBeLessThan(keys.indexOf("global"));
    expect(keys.indexOf("global")).toBeLessThan(keys.indexOf("aprs"));
  });

  it("returns empty object for empty schema", () => {
    const defaults = buildDefaults({ _schema_version: "1.0" });
    expect(defaults).toEqual({});
  });
});
