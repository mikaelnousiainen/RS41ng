import type { Schema, SchemaField, SchemaSection, SectionMetadata } from "@/types/schema";

/** Top-level schema keys that are NOT sections */
const NON_SECTION_KEYS = new Set([
  "_schema_version",
  "_template_variables",
  "conflicts",
  "presets",
]);

/** Returns true if a key inside a section is a field (not section metadata) */
function isFieldKey(key: string): boolean {
  return !key.startsWith("_");
}

/** Returns true if a top-level schema key is a config section */
function isSectionKey(key: string): boolean {
  return !NON_SECTION_KEYS.has(key) && !key.startsWith("_");
}

/** Get all section keys from the schema, sorted by _order */
export function getSortedSectionKeys(schema: Schema): string[] {
  return Object.keys(schema)
    .filter(isSectionKey)
    .sort((a, b) => {
      const sa = schema[a] as SchemaSection;
      const sb = schema[b] as SchemaSection;
      return (sa._order ?? 0) - (sb._order ?? 0);
    });
}

/** Get the metadata for a section */
export function getSectionMeta(
  schema: Schema,
  sectionKey: string
): SectionMetadata | null {
  const section = schema[sectionKey];
  if (!section || typeof section !== "object" || Array.isArray(section))
    return null;
  const s = section as SchemaSection;
  if (typeof s._section !== "string") return null;
  return {
    _section: s._section,
    _description: s._description,
    _tier: s._tier,
    _order: s._order,
    _visible_when: s._visible_when,
  };
}

/** Get all field keys in a section (excludes underscore-prefixed metadata) */
export function getSectionFieldKeys(
  schema: Schema,
  sectionKey: string
): string[] {
  const section = schema[sectionKey];
  if (!section || typeof section !== "object" || Array.isArray(section))
    return [];
  return Object.keys(section).filter(isFieldKey);
}

/** Get a specific field from a section, or null if not found */
export function getField(
  schema: Schema,
  sectionKey: string,
  fieldKey: string
): SchemaField | null {
  const section = schema[sectionKey];
  if (!section || typeof section !== "object" || Array.isArray(section))
    return null;
  const field = (section as SchemaSection)[fieldKey];
  if (!field || typeof field !== "object" || Array.isArray(field)) return null;
  return field as SchemaField;
}

/** Find a field by its section.field path (e.g. "global.callsign") */
export function findFieldByPath(
  schema: Schema,
  path: string
): SchemaField | null {
  const dot = path.indexOf(".");
  if (dot === -1) return null;
  const sectionKey = path.slice(0, dot);
  const fieldKey = path.slice(dot + 1);
  return getField(schema, sectionKey, fieldKey);
}

/** Get the C define name that an alias_of path resolves to */
export function resolveAliasDefine(
  schema: Schema,
  aliasOfPath: string
): string | null {
  const field = findFieldByPath(schema, aliasOfPath);
  return field?.define ?? null;
}
