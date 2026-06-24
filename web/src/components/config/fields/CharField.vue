<template>
  <FieldShell :field="field" :input-id="inputId" :errors="errors">
    <input
      :id="inputId"
      type="text"
      maxlength="1"
      :value="(modelValue as string) ?? ''"
      :class="[FIELD_CONTROL_CLASS, 'w-20 font-mono text-center']"
      @input="$emit('update:modelValue', ($event.target as HTMLInputElement).value)"
    />
  </FieldShell>
</template>

<script setup lang="ts">
import FieldShell from "./FieldShell.vue";
import { useField, FIELD_CONTROL_CLASS, type FieldProps } from "@/composables/useField";

const props = defineProps<FieldProps>();
defineEmits<{ "update:modelValue": [value: string] }>();
const { inputId } = useField(props);
</script>
