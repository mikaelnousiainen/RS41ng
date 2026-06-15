<template>
  <FieldShell :field="field" :input-id="inputId" :errors="errors">
    <select
      :id="inputId"
      :value="String(modelValue ?? '')"
      :class="[FIELD_CONTROL_CLASS, 'w-full']"
      @change="onSelect"
    >
      <option v-for="opt in field.values" :key="String(opt.value)" :value="String(opt.value)">
        {{ opt.label }}
      </option>
    </select>
  </FieldShell>
</template>

<script setup lang="ts">
import FieldShell from "./FieldShell.vue";
import { useField, FIELD_CONTROL_CLASS, type FieldProps } from "@/composables/useField";

const props = defineProps<FieldProps>();
const emit = defineEmits<{ "update:modelValue": [value: string | number] }>();
const { inputId } = useField(props);

function onSelect(e: Event) {
  const raw = (e.target as HTMLSelectElement).value;
  // enum_constant values are numeric C constants — preserve the number type;
  // plain enum values stay strings.
  if (props.field.type === "enum_constant") {
    const n = Number(raw);
    emit("update:modelValue", Number.isFinite(n) && raw !== "" ? n : raw);
  } else {
    emit("update:modelValue", raw);
  }
}
</script>
