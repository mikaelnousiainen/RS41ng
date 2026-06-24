import { dump as yamlDump } from "js-yaml";
import type { Schema, SchemaField } from "../types/schema";
interface TemplateVar { var: string; description: string }
import type { ConfigState } from "../types/config";
import { buildDefaults } from "./defaults";
import { getSortedSectionKeys, getSectionFieldKeys, getField } from "./schema-registry";

/**
 * Generate a fully-documented `config.yaml.example` from the schema: EVERY section
 * and EVERY field, each at its default value with a documentation comment derived
 * from the schema (label, description, tier, allowed values, constraints, target
 * #define).
 *
 * The YAML data is serialized by js-yaml (so all quoting/escaping/structure is
 * correct); this module only injects `#` comment lines — the one thing js-yaml
 * cannot emit — into the dumped output. Comment text is collapsed to a single line
 * so it can never break the surrounding YAML.
 *
 * Single source of truth for the example file — regenerate with
 * `bun run scripts/generate_example_config.ts` whenever the schema changes.
 */

/** Collapse any whitespace (including newlines embedded in schema text) to single spaces. */
function oneLine(s: string): string {
  return s.replace(/\s+/g, " ").trim();
}

/** Documentation comment lines for a field (indentation applied by the caller). */
function fieldDocLines(field: SchemaField): string[] {
  const lines: string[] = [];
  const head = field.description ? `${field.label} — ${field.description}` : field.label;
  lines.push(`# ${oneLine(head)} [${field.tier}]`);

  if (field.values && field.values.length > 0) {
    lines.push(`#   options: ${field.values.map((v) => oneLine(`${v.value} = ${v.label}`)).join("; ")}`);
  }

  const v = field.validation;
  if (v) {
    const parts: string[] = [];
    if (v.required) parts.push("required");
    if (v.min !== undefined || v.max !== undefined) parts.push(`range ${v.min ?? ""}..${v.max ?? ""}`);
    if (v.max_length !== undefined) parts.push(`max ${v.max_length} chars`);
    if (v.max_item_length !== undefined) parts.push(`max ${v.max_item_length} chars/item`);
    if (v.pattern) parts.push(`pattern ${v.pattern}`);
    if (parts.length) lines.push(`#   constraints: ${parts.join(", ")}`);
  }

  if (field.visible_when) lines.push(`#   applies when: ${field.visible_when}`);
  lines.push(`#   defines ${field.define}`);
  return lines;
}

const HEADER = [
  "# RS41ng Firmware Configuration — full reference",
  "#",
  "# Lists EVERY configuration section and setting with its default value and",
  "# documentation. AUTO-GENERATED from web/config-schema.yaml — do not edit by hand.",
  "# Regenerate with: bun run scripts/generate_example_config.ts",
  "#",
  "# Usage:",
  "#   1. Copy this file to config.yaml in the RS41ng root directory.",
  "#   2. Edit the values you need (e.g. set your callsign, enable transmit modes",
  "#      under tx_modes, set frequencies). Values shown here are the defaults.",
  "#   3. Build with: ./build-firmware.sh   (build-firmware.bat on Windows)",
  "#",
  "# When config.yaml is present the build generates src/config_generated.h and",
  "# src/config_generated.c from it; otherwise the built-in src/config.h is used.",
  "",
];

export function generateExampleYaml(
  schema: Schema,
  config: ConfigState = buildDefaults(schema)
): string {
  // 1. Build the data object (every section + field, in schema order) and let
  //    js-yaml render it — this guarantees valid YAML for all values.
  const data: Record<string, Record<string, unknown>> = {};
  for (const sectionKey of getSortedSectionKeys(schema)) {
    const fieldKeys = getSectionFieldKeys(schema, sectionKey);
    if (fieldKeys.length === 0) continue;
    const section: Record<string, unknown> = {};
    for (const fieldKey of fieldKeys) {
      const field = getField(schema, sectionKey, fieldKey);
      section[fieldKey] = config[sectionKey]?.[fieldKey] ?? field?.default ?? null;
    }
    data[sectionKey] = section;
  }

  const dumped = yamlDump(data, { lineWidth: -1, sortKeys: false });

  // Template-variable reference (used in message string fields), from the schema.
  const templateVars = (schema._template_variables as TemplateVar[] | undefined) ?? [];
  const templateVarLines =
    templateVars.length > 0
      ? [
          "# Template variables usable in message strings (e.g. APRS/CW/FT8 messages):",
          ...templateVars.map((t) => `#   ${t.var}  ${oneLine(t.description)}`),
          "",
        ]
      : [];

  // 2. Walk the rendered YAML and inject section banners + per-field doc comments.
  //    js-yaml emits section keys at column 0 (`section:`) and fields at column 2
  //    (`  field:`); array items (column 4) and value continuations pass through.
  const rule = "# " + "=".repeat(72);
  const out: string[] = [...HEADER, ...templateVarLines];
  let currentSection: string | null = null;

  for (const line of dumped.split("\n")) {
    const sectionMatch = /^([A-Za-z0-9_]+):/.exec(line);
    const fieldMatch = /^ {2}([A-Za-z0-9_]+):/.exec(line);

    if (sectionMatch) {
      currentSection = sectionMatch[1];
      const label = (schema[currentSection] as { _section?: string })?._section ?? currentSection;
      out.push("", rule, `# ${oneLine(label)}`, rule, line);
    } else if (fieldMatch && currentSection) {
      const field = getField(schema, currentSection, fieldMatch[1]);
      if (field) for (const docLine of fieldDocLines(field)) out.push("  " + docLine);
      out.push(line);
    } else {
      out.push(line);
    }
  }

  return out.join("\n").replace(/\n+$/, "\n");
}
