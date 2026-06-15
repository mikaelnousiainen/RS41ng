import type { Schema, SchemaField } from "@/types/schema";
import type { ConfigState, ConfigValue } from "@/types/config";
import { getSortedSectionKeys, getSectionFieldKeys, getField } from "./schema-registry";
import { evaluateCondition } from "./visibility";

/** Validate a single field value. Returns a list of error messages (empty = valid). */
export function validateField(
  field: SchemaField,
  value: ConfigValue | undefined
): string[] {
  const errors: string[] = [];
  const v = field.validation;

  // Alias fields have no independent value — skip
  if (field.alias_of !== undefined) return errors;

  const isUndefined = value === undefined || value === null || value === "";

  // Required check
  if (v?.required && isUndefined) {
    errors.push(`${field.label} is required`);
    return errors; // no further checks if value is missing
  }

  if (isUndefined) return errors;

  // Type-specific checks
  if (field.type === "string" || field.type === "hex") {
    const str = String(value);
    if (v?.max_length !== undefined && str.length > v.max_length) {
      errors.push(`${field.label} must be at most ${v.max_length} characters (got ${str.length})`);
    }
    if (v?.pattern !== undefined) {
      if (!new RegExp(v.pattern).test(str)) {
        errors.push(`${field.label} does not match required format`);
      }
    }
  }

  if (field.type === "char") {
    const str = String(value);
    if (str.length !== 1) {
      errors.push(`${field.label} must be exactly one character`);
    }
    if (v?.pattern !== undefined && !new RegExp(v.pattern).test(str)) {
      errors.push(`${field.label} does not match required format`);
    }
  }

  if (field.type === "integer" || field.type === "frequency") {
    const num = Number(value);
    if (isNaN(num)) {
      errors.push(`${field.label} must be a number`);
    } else {
      if (v?.min !== undefined && num < v.min) {
        errors.push(`${field.label} must be at least ${v.min}`);
      }
      if (v?.max !== undefined && num > v.max) {
        errors.push(`${field.label} must be at most ${v.max}`);
      }
    }
  }

  if (field.type === "enum" || field.type === "enum_constant") {
    if (field.values) {
      const valid = field.values.map((ev) => String(ev.value));
      if (!valid.includes(String(value))) {
        errors.push(`${field.label}: invalid value "${value}"`);
      }
    }
  }

  if (field.type === "string_array") {
    if (!Array.isArray(value)) {
      errors.push(`${field.label} must be a list`);
    } else if (v?.max_item_length !== undefined) {
      for (const [i, item] of value.entries()) {
        if (String(item).length > v.max_item_length) {
          errors.push(
            `${field.label} item ${i + 1} exceeds max length of ${v.max_item_length}`
          );
        }
      }
    }
  }

  return errors;
}

export interface ValidationResult {
  /** Map of "section.fieldKey" → list of error messages */
  fieldErrors: Record<string, string[]>;
  /** Active conflict messages */
  conflicts: string[];
  /** True if there are no errors and no conflicts */
  valid: boolean;
}

/**
 * Validate the entire config state against the schema.
 * Only validates fields that are visible (respects visible_when).
 */
export function validateConfig(
  schema: Schema,
  config: ConfigState
): ValidationResult {
  const fieldErrors: Record<string, string[]> = {};

  for (const sectionKey of getSortedSectionKeys(schema)) {
    for (const fieldKey of getSectionFieldKeys(schema, sectionKey)) {
      const field = getField(schema, sectionKey, fieldKey);
      if (!field) continue;
      // Skip invisible fields
      if (
        field.visible_when &&
        !evaluateCondition(field.visible_when, config)
      ) {
        continue;
      }

      const value = config[sectionKey]?.[fieldKey];
      const errors = validateField(field, value);
      if (errors.length > 0) {
        fieldErrors[`${sectionKey}.${fieldKey}`] = errors;
      }
    }
  }

  const conflicts = detectConflicts(schema, config);

  return {
    fieldErrors,
    conflicts,
    valid: Object.keys(fieldErrors).length === 0 && conflicts.length === 0,
  };
}

/**
 * Evaluate all conflict rules and return messages for any that are triggered.
 */
export function detectConflicts(
  schema: Schema,
  config: ConfigState
): string[] {
  if (!schema.conflicts) return [];
  const active: string[] = [];
  for (const rule of schema.conflicts) {
    try {
      if (evaluateCondition(rule.condition, config)) {
        active.push(rule.message);
      }
    } catch {
      // Skip rules that fail to evaluate
    }
  }
  return active;
}
