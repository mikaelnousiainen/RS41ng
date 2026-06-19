<template>
  <div class="space-y-2">
    <label class="block text-xs font-medium text-gray-400">{{ field.label }}</label>
    <div
      v-for="(item, i) in items"
      :key="i"
      class="flex items-center gap-2"
    >
      <input
        type="text"
        :value="item"
        :maxlength="field.validation?.max_item_length"
        :class="[FIELD_CONTROL_CLASS, 'flex-1 font-mono']"
        @input="updateItem(i, ($event.target as HTMLInputElement).value)"
      />
      <button
        type="button"
        title="Remove"
        class="text-gray-600 hover:text-red-400 transition-colors text-sm"
        @click="removeItem(i)"
      >
        ✕
      </button>
    </div>
    <button
      type="button"
      class="text-xs text-green-400 hover:text-green-300 transition-colors"
      @click="addItem"
    >
      + Add message
    </button>
    <p v-for="e in errors" :key="e" class="text-xs text-red-400">{{ e }}</p>
    <p v-if="field.description && !errors.length" class="text-xs text-gray-600">
      {{ field.description }}
    </p>
    <TemplateVarsDialog v-if="field.template_vars" />
  </div>
</template>

<script setup lang="ts">
import { computed } from "vue";
import { FIELD_CONTROL_CLASS, type FieldProps } from "@/composables/useField";
import TemplateVarsDialog from "@/components/config/TemplateVarsDialog.vue";

const props = defineProps<FieldProps>();
const emit = defineEmits<{ "update:modelValue": [value: string[]] }>();

const items = computed((): string[] => {
  if (Array.isArray(props.modelValue)) return props.modelValue as string[];
  return [];
});

function updateItem(i: number, val: string) {
  const arr = [...items.value];
  arr[i] = val;
  emit("update:modelValue", arr);
}

function removeItem(i: number) {
  emit("update:modelValue", items.value.filter((_, j: number) => j !== i));
}

function addItem() {
  emit("update:modelValue", [...items.value, ""]);
}
</script>
