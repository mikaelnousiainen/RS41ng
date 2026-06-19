export type FieldTier = "simple" | "intermediate" | "advanced";

export type FieldType =
  | "string"
  | "bool"
  | "integer"
  | "hex"
  | "frequency"
  | "char"
  | "enum"
  | "enum_constant"
  | "string_array";

export interface EnumValue {
  value: string | number;
  label: string;
}

export interface EnumDefine {
  /** C preprocessor constant name */
  define: string;
  value: number;
}

export interface ValidationRules {
  required?: boolean;
  min?: number;
  max?: number;
  /** Max length for string fields */
  max_length?: number;
  /** Max length per item for string_array fields */
  max_item_length?: number;
  /** Regex pattern applied to string value */
  pattern?: string;
}

export interface SchemaField {
  define: string;
  type: FieldType;
  label: string;
  description?: string;
  tier: FieldTier;
  /** Not present on alias_of fields */
  default?: string | number | boolean | string[];
  /** "config.c" for string_array message templates; defaults to "config.h" */
  file?: "config.h" | "config.c";
  values?: EnumValue[];
  /** C constant definitions emitted before the field define (enum_constant fields) */
  enum_defines?: EnumDefine[];
  validation?: ValidationRules;
  /** UL suffix for Si5351 frequency defines */
  suffix?: string;
  /** Path (e.g. "global.callsign") — emits #define THIS OTHER_DEFINE */
  alias_of?: string;
  /** Hardware select field uses comment/uncomment style */
  define_style?: "toggle";
  /** Condition string evaluated against config state; field hidden when falsy */
  visible_when?: string;
  /** True if field supports $cs, $loc6, etc. template variables */
  template_vars?: boolean;
  /** Visual group heading rendered before this field's group in the UI */
  group?: string;
}

export interface SectionMetadata {
  _section: string;
  _description?: string;
  _tier: FieldTier;
  _order: number;
  _visible_when?: string;
}

/** A schema section: underscore-prefixed metadata + field entries */
export type SchemaSection = SectionMetadata & {
  [fieldKey: string]: SchemaField | string | number | FieldTier | undefined;
};

export interface ConflictRule {
  id: string;
  message: string;
  /** JS-evaluable boolean expression using section.field dot notation */
  condition: string;
}

export interface TemplateVariable {
  var: string;
  description: string;
}

export interface Schema {
  _schema_version?: string;
  conflicts?: ConflictRule[];
  _template_variables?: TemplateVariable[];
  [sectionKey: string]:
    | SchemaSection
    | ConflictRule[]
    | TemplateVariable[]
    | string
    | undefined;
}
