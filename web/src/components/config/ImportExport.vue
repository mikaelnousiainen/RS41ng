<template>
  <div class="space-y-3">
    <h2 class="text-xs font-medium text-gray-400 uppercase tracking-wider">
      Download your config
    </h2>
    <div class="flex flex-wrap gap-3">
      <button
        v-for="btn in buttons"
        :key="btn.filename"
        type="button"
        class="inline-flex items-center gap-1.5 px-3 py-2 text-sm font-medium rounded-md border transition-colors font-mono"
        :class="
          btn.primary
            ? 'bg-green-700 hover:bg-green-600 text-white border-green-600'
            : 'bg-gray-800 hover:bg-gray-700 text-gray-200 border-gray-700'
        "
        @click="download(btn)"
      >
        ↓ {{ btn.filename }}
      </button>
    </div>
    <p class="text-xs text-gray-600">
      Download <code class="font-mono">config.yaml</code> and place it in the RS41ng repo root
      before building.
    </p>
  </div>
</template>

<script setup lang="ts">
import type { Schema } from "@/types/schema";
import { useConfigStore } from "@/stores/config";
import { generateConfigYaml } from "@/config/generate-yaml";
import { generateConfigH } from "@/config/generate-header";
import { generateConfigC } from "@/config/generate-source";
import { downloadFile } from "@/utils/download";

const props = defineProps<{
  schema: Schema;
}>();

const configStore = useConfigStore();

interface DownloadButton {
  filename: string;
  mimeType: string;
  primary: boolean;
  generate: () => string;
}

const buttons: DownloadButton[] = [
  {
    filename: "config.yaml",
    mimeType: "application/yaml",
    primary: true,
    generate: () => generateConfigYaml(props.schema, configStore.values),
  },
  {
    filename: "config.h",
    mimeType: "text/plain",
    primary: false,
    generate: () => generateConfigH(props.schema, configStore.values),
  },
  {
    filename: "config.c",
    mimeType: "text/plain",
    primary: false,
    generate: () => generateConfigC(props.schema, configStore.values),
  },
];

function download(btn: DownloadButton) {
  downloadFile(btn.filename, btn.mimeType, btn.generate());
}
</script>
