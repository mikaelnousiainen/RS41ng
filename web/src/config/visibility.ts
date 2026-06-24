import type { Schema, SchemaField } from "@/types/schema";
import type { ConfigState } from "@/types/config";
import { getSectionMeta } from "./schema-registry";

/**
 * Evaluate a visible_when / _visible_when expression against config state.
 *
 * Expressions use section.field dot notation:
 *   "tx_modes.radio_tx_aprs"
 *   "hardware.type == 'RS41'"
 *   "tx_modes.radio_tx_aprs || tx_modes.radio_tx_horus_v2"
 *   "hardware.type == 'RS41' && sensors.si5351_enable"
 *
 * Strategy: replace all `section.field` tokens with their resolved JS-literal
 * values, then evaluate the resulting expression with Function().
 */
export function evaluateCondition(
  expression: string,
  config: ConfigState
): boolean {
  // Replace section.field with resolved JS literal values.
  // Pattern: word chars (including underscores) dot word chars (including underscores)
  const substituted = expression.replace(
    /([A-Za-z_]\w*)\.([A-Za-z_]\w*)/g,
    (_, section, field) => {
      const val = config[section]?.[field];
      if (val === undefined) return "undefined";
      if (typeof val === "boolean") return String(val);
      if (typeof val === "number") return String(val);
      if (typeof val === "string") return JSON.stringify(val);
      // Fallback: serialize arrays/other values as JSON literals
      return JSON.stringify(val);
    }
  );

  try {
    // eslint-disable-next-line no-new-func
    return Boolean(new Function(`return (${substituted})`)());
  } catch {
    // On parse error default to visible
    return true;
  }
}

/**
 * Returns true if a field should be visible given the current config state.
 * Fields with no visible_when are always visible.
 */
export function isFieldVisible(
  field: SchemaField,
  config: ConfigState
): boolean {
  if (!field.visible_when) return true;
  return evaluateCondition(field.visible_when, config);
}

/**
 * Returns true if a section should be visible given the current config state.
 * Sections with no _visible_when are always visible.
 */
export function isSectionVisible(
  schema: Schema,
  sectionKey: string,
  config: ConfigState
): boolean {
  const meta = getSectionMeta(schema, sectionKey);
  if (!meta?._visible_when) return true;
  return evaluateCondition(meta._visible_when, config);
}
