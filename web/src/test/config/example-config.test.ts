import { describe, it, expect } from "vitest";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, resolve } from "node:path";
import { load as yamlLoad } from "js-yaml";
import type { Schema } from "@/types/schema";
import type { ConfigSection, ConfigState } from "@/types/config";
import { buildDefaults } from "@/config/defaults";
import { validateConfig } from "@/config/validation";
import { generateConfigH } from "@/config/generate-header";
import { generateConfigC } from "@/config/generate-source";
import { generateExampleYaml } from "@/config/generate-example";
import { getSortedSectionKeys, getSectionFieldKeys } from "@/config/schema-registry";

/**
 * Guards the repo-root `config.yaml.example` as a complete, documented reference:
 *   - it stays byte-identical to the generator (so it can never drift or go stale);
 *   - it contains EVERY schema section and field;
 *   - it survives the exact shared pipeline that `scripts/generate_config.ts` runs
 *     (merge defaults → validate → generate config.h/config.c).
 */

const here = dirname(fileURLToPath(import.meta.url));
const schema = yamlLoad(
  readFileSync(resolve(here, "../../../config-schema.yaml"), "utf8")
) as Schema;
const exampleText = readFileSync(resolve(here, "../../../../config.yaml.example"), "utf8");
const exampleConfig = yamlLoad(exampleText) as ConfigState;

// Mirror of the CLI's mergeWithDefaults(): user values over schema defaults.
function mergeWithDefaults(user: ConfigState): ConfigState {
  const merged = buildDefaults(schema);
  for (const [sectionKey, section] of Object.entries(user)) {
    if (section && typeof section === "object" && !Array.isArray(section)) {
      merged[sectionKey] = {
        ...(merged[sectionKey] ?? {}),
        ...(section as ConfigSection),
      };
    }
  }
  return merged;
}

describe("config.yaml.example", () => {
  it("is in sync with the schema (run `bun run scripts/generate_example_config.ts` if this fails)", () => {
    expect(exampleText).toBe(generateExampleYaml(schema));
  });

  it("contains every section and field defined in the schema", () => {
    const missing: string[] = [];
    for (const sectionKey of getSortedSectionKeys(schema)) {
      const fieldKeys = getSectionFieldKeys(schema, sectionKey);
      if (fieldKeys.length === 0) continue;
      const section = (exampleConfig as Record<string, ConfigSection | undefined>)[sectionKey];
      for (const fieldKey of fieldKeys) {
        if (!section || !(fieldKey in section)) missing.push(`${sectionKey}.${fieldKey}`);
      }
    }
    expect(missing).toEqual([]);
  });

  it("passes the shared validator with no errors or conflicts", () => {
    const result = validateConfig(schema, mergeWithDefaults(exampleConfig));
    expect(result.fieldErrors).toEqual({});
    expect(result.conflicts).toEqual([]);
    expect(result.valid).toBe(true);
  });

  it("generates a config.h and config.c through the shared pipeline", () => {
    const header = generateConfigH(schema, exampleConfig);
    expect(header).toContain("#define CALLSIGN");
    expect(header).toContain('#include "config_internal.h"');

    const source = generateConfigC(schema, exampleConfig);
    expect(source).toContain('#include "config.h"');
    expect(source).toContain("NULL");
  });
});
