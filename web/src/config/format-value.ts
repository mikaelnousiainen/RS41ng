import type { SchemaField } from "../types/schema";

/** Escape backslashes and double-quotes for C string literals */
export function escapeString(s: string): string {
  return s.replace(/\\/g, "\\\\").replace(/"/g, '\\"');
}

/** Format a single character value as a C character literal */
export function formatChar(ch: string): string {
  if (ch === "\\") return "'\\\\'";
  // A bare apostrophe can't be written as the char literal '''; emit it via the
  // shell-style quoted form "'\''" so the generated source stays valid C.
  if (ch === "'") return "\"'\\''\"";
  return `'${ch}'`;
}

/**
 * Convert a config field value to its C representation.
 * Used for #define values in config.h.
 */
export function formatValue(field: SchemaField, value: unknown): string {
  switch (field.type) {
    case "string":
      return `"${escapeString(String(value))}"`;
    case "bool":
      return value === true || value === "true" ? "true" : "false";
    case "integer":
      return String(Number(value));
    case "hex":
      return String(value);
    case "frequency":
      return `${value}${field.suffix ?? ""}`;
    case "char":
      return formatChar(String(value));
    case "enum_constant":
      return String(value);
    case "enum":
      return String(value);
    default:
      throw new Error(`Unknown field type: ${field.type}`);
  }
}
