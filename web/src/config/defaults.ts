import type { Schema } from "@/types/schema";
import type { ConfigState } from "@/types/config";
import { getSortedSectionKeys, getSectionFieldKeys, getField } from "./schema-registry";

/**
 * Build a ConfigState populated with the default value from every schema field
 * that has a `default` (alias_of fields are skipped — they reference another value).
 */
export function buildDefaults(schema: Schema): ConfigState {
  const defaults: ConfigState = {};

  for (const sectionKey of getSortedSectionKeys(schema)) {
    for (const fieldKey of getSectionFieldKeys(schema, sectionKey)) {
      const field = getField(schema, sectionKey, fieldKey);
      if (!field) continue;
      // Skip alias fields — they don't have their own value
      if (field.alias_of !== undefined) continue;
      if (field.default === undefined) continue;

      if (!defaults[sectionKey]) defaults[sectionKey] = {};
      defaults[sectionKey][fieldKey] = field.default;
    }
  }

  return defaults;
}
