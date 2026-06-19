<template>
  <!-- Shared chrome for label-on-top field controls: label, the control (slot),
       validation errors, and the help description. -->
  <div class="space-y-1">
    <label :for="inputId" class="block text-xs font-medium text-gray-400">
      {{ field.label }}
    </label>
    <slot />
    <p v-for="e in errors" :key="e" class="text-xs text-red-400">{{ e }}</p>
    <p v-if="field.description && !errors.length" class="text-xs text-gray-600">
      {{ field.description }}
    </p>
  </div>
</template>

<script setup lang="ts">
import type { SchemaField } from "@/types/schema";

defineProps<{
  field: SchemaField;
  inputId: string;
  errors: string[];
}>();
</script>
