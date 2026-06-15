<template>
  <component
    :is="fieldComponent"
    v-if="fieldComponent"
    :field="field"
    :modelValue="currentValue"
    :errors="fieldErrors"
    @update:modelValue="updateValue"
  />
</template>

<script setup lang="ts">
import { computed } from "vue";
import type { SchemaField } from "@/types/schema";
import type { ConfigValue } from "@/types/config";
import { useConfigStore } from "@/stores/config";
import { validateField } from "@/config/validation";

import StringField from "./fields/StringField.vue";
import BoolField from "./fields/BoolField.vue";
import IntegerField from "./fields/IntegerField.vue";
import HexField from "./fields/HexField.vue";
import FrequencyField from "./fields/FrequencyField.vue";
import CharField from "./fields/CharField.vue";
import EnumField from "./fields/EnumField.vue";
import StringArrayField from "./fields/StringArrayField.vue";

// enum and enum_constant share EnumField; it coerces numeric values for enum_constant.
const TYPE_MAP = {
  string: StringField,
  bool: BoolField,
  integer: IntegerField,
  hex: HexField,
  frequency: FrequencyField,
  char: CharField,
  enum: EnumField,
  enum_constant: EnumField,
  string_array: StringArrayField,
} as const;

const props = defineProps<{
  sectionKey: string;
  fieldKey: string;
  field: SchemaField;
}>();

const configStore = useConfigStore();

const fieldComponent = computed(
  () => TYPE_MAP[props.field.type as keyof typeof TYPE_MAP] ?? null
);

const currentValue = computed(() => configStore.get(props.sectionKey, props.fieldKey));

const fieldErrors = computed(() => validateField(props.field, currentValue.value));

function updateValue(val: ConfigValue) {
  configStore.set(props.sectionKey, props.fieldKey, val);
}
</script>
