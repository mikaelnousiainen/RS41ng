<template>
  <div class="space-y-2">
    <label class="block text-xs font-medium text-gray-400">Firmware binary (.bin)</label>
    <div
      class="relative flex items-center justify-center rounded-md border-2 border-dashed transition-colors p-6 cursor-pointer"
      :class="boxClass"
      @dragover.prevent="isDragging = true"
      @dragleave="isDragging = false"
      @drop.prevent="onDrop"
      @click="fileInput?.click()"
    >
      <input
        ref="fileInput"
        type="file"
        accept=".bin"
        class="sr-only"
        @change="onFileChange"
      />
      <div class="text-center space-y-1">
        <template v-if="selectedFile">
          <p class="text-sm font-semibold text-green-400 flex items-center justify-center gap-2">
            <span class="text-base leading-none" aria-hidden="true">&#x2713;</span>
            File selected
          </p>
          <p class="text-sm font-mono text-green-300 break-all">{{ selectedFile.name }}</p>
          <p class="text-xs text-green-600/80">Click or drop another file to replace it.</p>
        </template>
        <template v-else>
          <p class="text-sm text-gray-400">Drop <code class="font-mono">RS41ng.bin</code> here or click to browse</p>
          <p class="text-xs text-gray-600">build/RS41ng.bin</p>
        </template>
      </div>
    </div>
    <p v-if="error" class="text-xs text-red-400">{{ error }}</p>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from "vue";
import { useFlasherStore } from "@/stores/flasher";

const flasher = useFlasherStore();
const fileInput = ref<HTMLInputElement | null>(null);
const isDragging = ref(false);
const error = ref<string | null>(null);

// Single source of truth: the store's binaryFile. This keeps the box in sync with
// the file that will actually be flashed, and it persists across "Clear / Flash again"
// (which preserves the file) while clearing on "Reset".
const selectedFile = computed(() => flasher.binaryFile);

const boxClass = computed(() => {
  if (isDragging.value) return "border-green-500 bg-green-900/20";
  if (selectedFile.value) return "border-green-600 bg-green-900/10 hover:border-green-500";
  return "border-gray-700 hover:border-gray-600 bg-gray-900";
});

function setFile(file: File) {
  if (!file.name.endsWith(".bin")) {
    error.value = "Please select a .bin firmware file.";
    return;
  }
  error.value = null;
  flasher.binaryFile = file;
}

function onFileChange(e: Event) {
  const file = (e.target as HTMLInputElement).files?.[0];
  if (file) setFile(file);
}

function onDrop(e: DragEvent) {
  isDragging.value = false;
  const file = e.dataTransfer?.files?.[0];
  if (file) setFile(file);
}
</script>
