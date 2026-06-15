import { computed } from "vue";
import type { SchemaField } from "@/types/schema";
import type { ConfigValue } from "@/types/config";

/** Props shared by every field-input component rendered by FieldRenderer. */
export interface FieldProps {
  field: SchemaField;
  modelValue: ConfigValue | undefined;
  errors: string[];
}

/**
 * Base Tailwind classes for a field control (text input / select). Each field
 * adds its own width/font modifiers (w-full, flex-1, font-mono, …) on top.
 */
export const FIELD_CONTROL_CLASS =
  "block rounded-md bg-gray-800 border-gray-700 text-gray-100 text-sm focus:ring-green-500 focus:border-green-500";

/** Derives a stable DOM id for a field's control from its C define name. */
export function useField(props: { field: SchemaField }) {
  const inputId = computed(
    () => `field-${props.field.define.toLowerCase().replace(/_/g, "-")}`
  );
  return { inputId };
}
