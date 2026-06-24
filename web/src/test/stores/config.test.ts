import { describe, it, expect } from "vitest";
import { useConfigStore } from "@/stores/config";
import type { ConfigState } from "@/types/config";

describe("useConfigStore", () => {
  describe("initial state", () => {
    it("starts with empty values", () => {
      const config = useConfigStore();
      expect(config.values).toEqual({});
    });
  });

  describe("set and get", () => {
    it("stores a string value", () => {
      const config = useConfigStore();
      config.set("global", "callsign", "OH2A");
      expect(config.get("global", "callsign")).toBe("OH2A");
    });

    it("stores a boolean value", () => {
      const config = useConfigStore();
      config.set("global", "leds_enable", true);
      expect(config.get("global", "leds_enable")).toBe(true);
    });

    it("stores a numeric value", () => {
      const config = useConfigStore();
      config.set("radio_si4032", "tx_power", 7);
      expect(config.get("radio_si4032", "tx_power")).toBe(7);
    });

    it("stores a string array", () => {
      const config = useConfigStore();
      config.set("cw", "messages", ["$cs $loc6 $altm"]);
      expect(config.get("cw", "messages")).toEqual(["$cs $loc6 $altm"]);
    });

    it("creates the section if it does not exist", () => {
      const config = useConfigStore();
      config.set("aprs", "ssid", "9");
      expect(config.values["aprs"]).toBeDefined();
    });

    it("returns undefined for unknown field", () => {
      const config = useConfigStore();
      expect(config.get("global", "nonexistent")).toBeUndefined();
    });

    it("returns undefined for unknown section", () => {
      const config = useConfigStore();
      expect(config.get("nonexistent", "field")).toBeUndefined();
    });

    it("overwrites existing value", () => {
      const config = useConfigStore();
      config.set("global", "callsign", "N0CALL");
      config.set("global", "callsign", "OH2A");
      expect(config.get("global", "callsign")).toBe("OH2A");
    });
  });

  describe("reset", () => {
    it("replaces all values with a deep copy of the provided state", () => {
      const config = useConfigStore();
      config.set("global", "callsign", "CHANGED");

      const initial: ConfigState = {
        global: { callsign: "N0CALL", leds_enable: true },
      };
      config.reset(initial);

      expect(config.get("global", "callsign")).toBe("N0CALL");
      expect(config.get("global", "leds_enable")).toBe(true);
    });

    it("does not share references with the initial object", () => {
      const config = useConfigStore();
      const initial: ConfigState = { global: { callsign: "N0CALL" } };
      config.reset(initial);

      initial["global"]["callsign"] = "MUTATED";
      expect(config.get("global", "callsign")).toBe("N0CALL");
    });
  });

  describe("applyPreset", () => {
    it("merges preset values into existing config", () => {
      const config = useConfigStore();
      config.set("global", "callsign", "OH2A");
      config.set("radio_si4032", "tx_power", 3);

      config.applyPreset({
        radio_si4032: { tx_aprs: true, tx_power: 7 },
      });

      expect(config.get("global", "callsign")).toBe("OH2A"); // unchanged
      expect(config.get("radio_si4032", "tx_aprs")).toBe(true); // added
      expect(config.get("radio_si4032", "tx_power")).toBe(7); // overwritten
    });

    it("creates a new section if it does not exist", () => {
      const config = useConfigStore();
      config.applyPreset({ cats: { ssid: 11 } });
      expect(config.get("cats", "ssid")).toBe(11);
    });
  });
});
