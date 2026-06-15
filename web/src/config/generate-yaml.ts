import { dump as yamlDump } from "js-yaml";
import type { Schema } from "../types/schema";
import type { ConfigState } from "../types/config";
import { getSortedSectionKeys, getSectionFieldKeys } from "./schema-registry";

/**
 * Serialize a ConfigState to a config.yaml string, with sections and fields
 * ordered according to the schema (_order / field declaration order).
 * Fields not present in config are omitted (not written as null).
 */
export function generateConfigYaml(schema: Schema, config: ConfigState): string {
  const out: Record<string, Record<string, unknown>> = {};

  for (const sectionKey of getSortedSectionKeys(schema)) {
    const section: Record<string, unknown> = {};
    for (const fieldKey of getSectionFieldKeys(schema, sectionKey)) {
      const value = config[sectionKey]?.[fieldKey];
      if (value !== undefined) {
        section[fieldKey] = value;
      }
    }
    if (Object.keys(section).length > 0) {
      out[sectionKey] = section;
    }
  }

  return yamlDump(out, { lineWidth: -1 });
}
