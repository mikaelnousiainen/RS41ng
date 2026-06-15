<template>
  <button
    type="button"
    :title="copied ? 'Copied!' : 'Copy to clipboard'"
    class="inline-flex items-center gap-1 px-2 py-1 text-xs font-medium rounded transition-colors"
    :class="
      copied
        ? 'text-green-400 bg-green-900/30'
        : 'text-gray-400 hover:text-gray-200 bg-gray-800 hover:bg-gray-700'
    "
    @click="copy"
  >
    <span>{{ copied ? "✓ Copied" : "Copy" }}</span>
  </button>
</template>

<script setup lang="ts">
import { ref } from "vue";

const props = defineProps<{ text: string }>();

const copied = ref(false);

async function copy() {
  try {
    await navigator.clipboard.writeText(props.text);
    copied.value = true;
    setTimeout(() => {
      copied.value = false;
    }, 2000);
  } catch {
    // Clipboard API not available (non-HTTPS, Firefox without permission)
  }
}
</script>
