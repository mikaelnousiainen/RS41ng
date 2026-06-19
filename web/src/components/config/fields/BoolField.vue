<template>
  <div class="flex items-start justify-between gap-4">
    <div class="flex-1 min-w-0">
      <label
        :for="inputId"
        class="text-xs font-medium text-gray-400 cursor-pointer select-none"
      >
        {{ field.label }}
      </label>
      <p v-if="field.description" class="text-xs text-gray-600 mt-0.5">
        {{ field.description }}
      </p>
      <p v-for="e in errors" :key="e" class="text-xs text-red-400">{{ e }}</p>
    </div>
    <button
      :id="inputId"
      type="button"
      role="switch"
      :aria-checked="isChecked"
      :aria-label="field.label"
      class="relative inline-flex h-5 w-9 flex-shrink-0 rounded-full border-2 border-transparent cursor-pointer transition-colors focus:outline-none focus:ring-2 focus:ring-green-500 focus:ring-offset-2 focus:ring-offset-gray-900"
      :class="isChecked ? 'bg-green-600' : 'bg-gray-700'"
      @click="$emit('update:modelValue', !isChecked)"
    >
      <span
        class="pointer-events-none inline-block h-4 w-4 rounded-full bg-white shadow transform ring-0 transition-transform"
        :class="isChecked ? 'translate-x-4' : 'translate-x-0'"
      />
    </button>
  </div>
</template>

<script setup lang="ts">
import { computed } from "vue";
import { useField, type FieldProps } from "@/composables/useField";

const props = defineProps<FieldProps>();
defineEmits<{ "update:modelValue": [value: boolean] }>();

const { inputId } = useField(props);
const isChecked = computed(() => props.modelValue === true || props.modelValue === "true");
</script>
