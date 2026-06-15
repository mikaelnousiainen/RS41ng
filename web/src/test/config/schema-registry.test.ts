import { describe, it, expect } from "vitest";
import type { Schema, SchemaSection } from "@/types/schema";
import {
  getSortedSectionKeys,
  getSectionMeta,
  getSectionFieldKeys,
  getField,
  findFieldByPath,
  resolveAliasDefine,
} from "@/config/schema-registry";

// Minimal test schema — does not depend on the real schema file
function makeTestSchema(): Schema {
  return {
    _schema_version: "1.0",
    conflicts: [],
    global: {
      _section: "Callsign & Basics",
      _tier: "simple",
      _order: 1,
      callsign: {
        define: "CALLSIGN",
        type: "string",
        label: "Call Sign",
        tier: "simple",
        default: "N0CALL",
        validation: { required: true, max_length: 6 },
      },
      leds_enable: {
        define: "LEDS_ENABLE",
        type: "bool",
        label: "Enable LEDs",
        tier: "intermediate",
        default: true,
      },
    } as unknown as SchemaSection,
    hardware: {
      _section: "Hardware Selection",
      _tier: "simple",
      _order: 0,
      type: {
        define: "RS41",
        define_style: "toggle",
        type: "enum",
        label: "Hardware Type",
        tier: "simple",
        default: "RS41",
        values: [
          { value: "RS41", label: "Vaisala RS41" },
          { value: "DFM17", label: "Graw DFM-17" },
        ],
      },
    } as unknown as SchemaSection,
    aprs: {
      _section: "APRS",
      _tier: "intermediate",
      _order: 7,
      _visible_when: "radio_si4032.tx_aprs || radio_si4063.tx_aprs",
      callsign: {
        define: "APRS_CALLSIGN",
        type: "string",
        label: "APRS Call Sign",
        tier: "intermediate",
        alias_of: "global.callsign",
      },
      ssid: {
        define: "APRS_SSID",
        type: "char",
        label: "APRS SSID",
        tier: "intermediate",
        default: "9",
      },
    } as unknown as SchemaSection,
  };
}

describe("getSortedSectionKeys", () => {
  it("returns section keys sorted by _order", () => {
    const schema = makeTestSchema();
    const keys = getSortedSectionKeys(schema);
    expect(keys).toEqual(["hardware", "global", "aprs"]);
  });

  it("excludes non-section keys", () => {
    const schema = makeTestSchema();
    const keys = getSortedSectionKeys(schema);
    expect(keys).not.toContain("_schema_version");
    expect(keys).not.toContain("conflicts");
  });
});

describe("getSectionMeta", () => {
  it("returns metadata for a valid section", () => {
    const schema = makeTestSchema();
    const meta = getSectionMeta(schema, "hardware");
    expect(meta?._section).toBe("Hardware Selection");
    expect(meta?._tier).toBe("simple");
    expect(meta?._order).toBe(0);
  });

  it("returns visible_when when present", () => {
    const schema = makeTestSchema();
    const meta = getSectionMeta(schema, "aprs");
    expect(meta?._visible_when).toBe(
      "radio_si4032.tx_aprs || radio_si4063.tx_aprs"
    );
  });

  it("returns null for non-existent section", () => {
    const schema = makeTestSchema();
    expect(getSectionMeta(schema, "nonexistent")).toBeNull();
  });
});

describe("getSectionFieldKeys", () => {
  it("returns non-underscore keys", () => {
    const schema = makeTestSchema();
    const keys = getSectionFieldKeys(schema, "global");
    expect(keys).toContain("callsign");
    expect(keys).toContain("leds_enable");
    expect(keys).not.toContain("_section");
    expect(keys).not.toContain("_tier");
    expect(keys).not.toContain("_order");
  });

  it("returns empty array for unknown section", () => {
    const schema = makeTestSchema();
    expect(getSectionFieldKeys(schema, "unknown")).toEqual([]);
  });
});

describe("getField", () => {
  it("returns a field object", () => {
    const schema = makeTestSchema();
    const field = getField(schema, "global", "callsign");
    expect(field?.define).toBe("CALLSIGN");
    expect(field?.type).toBe("string");
    expect(field?.default).toBe("N0CALL");
  });

  it("returns null for unknown field", () => {
    const schema = makeTestSchema();
    expect(getField(schema, "global", "nonexistent")).toBeNull();
  });

  it("returns null for unknown section", () => {
    const schema = makeTestSchema();
    expect(getField(schema, "missing", "field")).toBeNull();
  });
});

describe("findFieldByPath", () => {
  it("finds field by section.field path", () => {
    const schema = makeTestSchema();
    const field = findFieldByPath(schema, "global.callsign");
    expect(field?.define).toBe("CALLSIGN");
  });

  it("returns null for invalid path without dot", () => {
    const schema = makeTestSchema();
    expect(findFieldByPath(schema, "global")).toBeNull();
  });
});

describe("resolveAliasDefine", () => {
  it("resolves alias_of path to the define name", () => {
    const schema = makeTestSchema();
    const define = resolveAliasDefine(schema, "global.callsign");
    expect(define).toBe("CALLSIGN");
  });

  it("returns null for unknown path", () => {
    const schema = makeTestSchema();
    expect(resolveAliasDefine(schema, "global.unknown")).toBeNull();
  });
});
