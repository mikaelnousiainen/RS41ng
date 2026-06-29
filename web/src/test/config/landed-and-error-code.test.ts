import { describe, it, expect } from "vitest";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, resolve } from "node:path";
import { load as yamlLoad } from "js-yaml";
import type { Schema } from "@/types/schema";
import type { ConfigState } from "@/types/config";
import { buildDefaults } from "@/config/defaults";
import { generateConfigH } from "@/config/generate-header";
import { getField, getSectionFieldKeys } from "@/config/schema-registry";

/**
 * Verifies that the HORUS_V3_TX_ERROR_CODE toggle and the full LANDED_MODE_*
 * parameter set exist in the schema with defines/defaults that mirror config.h
 * exactly, and that the header generator emits them.
 */

const here = dirname(fileURLToPath(import.meta.url));
const schema = yamlLoad(
  readFileSync(resolve(here, "../../../config-schema.yaml"), "utf8")
) as Schema;

// (sectionKey, fieldKey) -> [#define name, default value], mirrored from src/config.h
const LANDED_FIELDS: Array<[string, string, string | number | boolean]> = [
  ["enable", "LANDED_MODE_ENABLE", false],
  ["arm_altitude_meters", "LANDED_MODE_ARM_ALTITUDE_METERS", 5000],
  ["climb_threshold_cm_s", "LANDED_MODE_CLIMB_THRESHOLD_CM_S", 50],
  ["speed_threshold_cm_s", "LANDED_MODE_SPEED_THRESHOLD_CM_S", 50],
  ["stationary_seconds", "LANDED_MODE_STATIONARY_SECONDS", 600],
  ["sleep_seconds", "LANDED_MODE_SLEEP_SECONDS", 300],
  ["gps_fix_timeout_seconds", "LANDED_MODE_GPS_FIX_TIMEOUT_SECONDS", 120],
  ["transmit_seconds", "LANDED_MODE_TRANSMIT_SECONDS", 30],
  ["geofence_radius_meters", "LANDED_MODE_GEOFENCE_RADIUS_METERS", 100],
  ["pip_enable", "LANDED_MODE_PIP_ENABLE", true],
  ["pip_interval_seconds", "LANDED_MODE_PIP_INTERVAL_SECONDS", 10],
  ["leds_transmit_only", "LANDED_MODE_LEDS_TRANSMIT_ONLY", true],
];

describe("HORUS_V3_TX_ERROR_CODE schema field", () => {
  it("exists in horus_v3 with the right define, type and default", () => {
    const field = getField(schema, "horus_v3", "error_code");
    expect(field).not.toBeNull();
    expect(field?.define).toBe("HORUS_V3_TX_ERROR_CODE");
    expect(field?.type).toBe("bool");
    expect(field?.default).toBe(false);
    expect(field?.description).toBeTruthy();
  });
});

describe("Landed Mode schema section", () => {
  it("is a section with all parameters mirroring config.h", () => {
    expect((schema.landed as { _section?: string })?._section).toBe("Landed Mode");

    const fieldKeys = getSectionFieldKeys(schema, "landed");
    expect(fieldKeys).toEqual(LANDED_FIELDS.map(([key]) => key));

    for (const [fieldKey, define, def] of LANDED_FIELDS) {
      const field = getField(schema, "landed", fieldKey);
      expect(field, fieldKey).not.toBeNull();
      expect(field?.define, fieldKey).toBe(define);
      expect(field?.default, fieldKey).toBe(def);
      expect(field?.description, fieldKey).toBeTruthy();
    }
  });
});

describe("header generation for the new fields", () => {
  it("emits the defines at default values", () => {
    const config: ConfigState = buildDefaults(schema);
    const header = generateConfigH(schema, config);

    expect(header).toContain("#define HORUS_V3_TX_ERROR_CODE false");
    for (const [, define, def] of LANDED_FIELDS) {
      const formatted = typeof def === "boolean" ? String(def) : String(def);
      expect(header).toContain(`#define ${define} ${formatted}`);
    }
  });

  it("reflects overridden values (landed enabled, error code on)", () => {
    const config: ConfigState = buildDefaults(schema);
    config.landed = { ...config.landed, enable: true, sleep_seconds: 120 };
    config.horus_v3 = { ...config.horus_v3, error_code: true };

    const header = generateConfigH(schema, config);

    expect(header).toContain("#define LANDED_MODE_ENABLE true");
    expect(header).toContain("#define LANDED_MODE_SLEEP_SECONDS 120");
    expect(header).toContain("#define HORUS_V3_TX_ERROR_CODE true");
  });
});
