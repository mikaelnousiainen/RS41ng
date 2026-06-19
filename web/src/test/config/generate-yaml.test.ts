import { describe, it, expect } from "vitest";
import { load as yamlLoad } from "js-yaml";
import type { Schema } from "@/types/schema";
import type { ConfigState } from "@/types/config";
import { generateConfigYaml } from "@/config/generate-yaml";

function makeSchema(): Schema {
  return {
    hardware: {
      _section: "Hardware",
      _tier: "simple",
      _order: 0,
      type: {
        define: "HARDWARE_TYPE",
        type: "enum",
        label: "Hardware",
        tier: "simple",
        default: "RS41",
        values: [
          { value: "RS41", label: "RS41" },
          { value: "DFM17", label: "DFM-17" },
        ],
        define_style: "toggle",
      },
    },
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
    },
    radio_si4032: {
      _section: "Radio",
      _tier: "simple",
      _order: 2,
      tx_power: {
        define: "TX_POWER",
        type: "integer",
        label: "TX Power",
        tier: "simple",
        default: 7,
      },
    },
  } as unknown as Schema;
}

describe("generateConfigYaml", () => {
  it("produces valid YAML that round-trips back to the config", () => {
    const schema = makeSchema();
    const config: ConfigState = {
      hardware: { type: "RS41" },
      global: { callsign: "OH2A" },
      radio_si4032: { tx_power: 5 },
    };
    const yaml = generateConfigYaml(schema, config);
    const parsed = yamlLoad(yaml) as ConfigState;
    expect(parsed.hardware?.type).toBe("RS41");
    expect(parsed.global?.callsign).toBe("OH2A");
    expect(parsed.radio_si4032?.tx_power).toBe(5);
  });

  it("respects section _order (hardware before global before radio)", () => {
    const schema = makeSchema();
    const config: ConfigState = {
      hardware: { type: "RS41" },
      global: { callsign: "OH2A" },
      radio_si4032: { tx_power: 5 },
    };
    const yaml = generateConfigYaml(schema, config);
    const hwPos = yaml.indexOf("hardware:");
    const globalPos = yaml.indexOf("global:");
    const radioPos = yaml.indexOf("radio_si4032:");
    expect(hwPos).toBeLessThan(globalPos);
    expect(globalPos).toBeLessThan(radioPos);
  });

  it("omits sections that have no values in config", () => {
    const schema = makeSchema();
    const config: ConfigState = {
      global: { callsign: "OH2A" },
    };
    const yaml = generateConfigYaml(schema, config);
    expect(yaml).not.toContain("radio_si4032:");
  });

  it("omits fields not present in config (undefined not written as null)", () => {
    const schema = makeSchema();
    const config: ConfigState = {
      radio_si4032: { tx_power: 7 },
    };
    const yaml = generateConfigYaml(schema, config);
    // global not in config → should not appear
    expect(yaml).not.toContain("callsign:");
  });

  it("preserves boolean values correctly", () => {
    const schema: Schema = {
      radio_si4032: {
        _section: "Radio",
        _tier: "simple",
        _order: 0,
        tx_aprs: {
          define: "TX_APRS",
          type: "bool",
          label: "APRS",
          tier: "simple",
          default: false,
        },
      },
    } as unknown as Schema;
    const config: ConfigState = { radio_si4032: { tx_aprs: true } };
    const yaml = generateConfigYaml(schema, config);
    const parsed = yamlLoad(yaml) as ConfigState;
    expect(parsed.radio_si4032?.tx_aprs).toBe(true);
  });

  it("preserves string array values", () => {
    const schema: Schema = {
      messages: {
        _section: "Messages",
        _tier: "intermediate",
        _order: 0,
        cw_messages: {
          define: "cw_messages",
          type: "string_array",
          label: "CW Messages",
          tier: "intermediate",
          file: "config.c",
          default: ["$cs $loc6 $altm"],
        },
      },
    } as unknown as Schema;
    const config: ConfigState = {
      messages: { cw_messages: ["$cs $altm", "$cs $loc6"] },
    };
    const yaml = generateConfigYaml(schema, config);
    const parsed = yamlLoad(yaml) as ConfigState;
    expect(parsed.messages?.cw_messages).toEqual(["$cs $altm", "$cs $loc6"]);
  });
});
