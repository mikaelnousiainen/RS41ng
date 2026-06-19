<template>
  <div class="space-y-8">
    <div>
      <h1 class="text-xl font-semibold text-gray-100">Build Firmware</h1>
      <p class="mt-1 text-sm text-gray-400">
        Build your custom firmware using Docker on your machine.
      </p>
    </div>

    <!-- Build with Docker -->
    <section class="space-y-3">
      <OsTabGroup v-slot="{ os }">
        <div class="space-y-4 text-sm text-gray-400">
          <!-- Pre-requisites (first time only) -->
          <details class="group rounded-md border border-gray-700 bg-gray-900/50">
            <summary class="cursor-pointer select-none px-4 py-3 flex items-center gap-2 text-sm font-medium text-gray-300 hover:text-gray-200">
              <svg
                class="w-3.5 h-3.5 transition-transform group-open:rotate-90 text-gray-500"
                fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2"
              >
                <path d="M9 5l7 7-7 7" />
              </svg>
              Pre-requisites
              <span class="text-xs font-normal text-gray-500">(first time only)</span>
            </summary>
            <div class="px-4 pb-4 space-y-4">
              <div class="space-y-2">
                <p class="font-medium text-gray-300">Install Docker</p>
                <CommandBlock
                  v-if="os === 'linux'"
                  command="# Follow https://docs.docker.com/engine/install/"
                  label="Linux"
                />
                <CommandBlock
                  v-else
                  command="# Download Docker Desktop from https://www.docker.com/products/docker-desktop/"
                  :label="os === 'macos' ? 'macOS' : 'Windows'"
                />
              </div>

              <div class="space-y-2">
                <p class="font-medium text-gray-300">Clone the RS41ng repository</p>
                <CommandBlock
                  command="git clone https://github.com/mikaelnousiainen/RS41ng.git
cd RS41ng"
                  label="terminal"
                />
              </div>

              <div class="space-y-2">
                <p class="font-medium text-gray-300">Build the Docker image</p>
                <CommandBlock command="docker build -t rs41ng_compiler ." label="terminal" />
              </div>
            </div>
          </details>

          <!-- Download and place config -->
          <div class="space-y-2">
            <p class="font-medium text-gray-300">
              Download and place your configuration file
            </p>
            <p class="text-xs text-gray-500">
              Click the button below to download <code class="font-mono text-green-400">config.yaml</code>,
              then save it into the <code class="font-mono">RS41ng</code> folder on your computer
              (the same folder that contains the <code class="font-mono">Dockerfile</code>).
            </p>
            <div class="flex flex-wrap items-center gap-3 pt-1">
              <button
                type="button"
                class="inline-flex items-center gap-1.5 px-3 py-2 text-sm font-medium rounded-md border transition-colors font-mono bg-green-700 hover:bg-green-600 text-white border-green-600"
                @click="downloadYaml"
              >
                &darr; config.yaml
              </button>
            </div>

            <!-- Advanced: config.h / config.c for developers -->
            <details class="group pt-1">
              <summary class="cursor-pointer select-none text-xs text-gray-600 hover:text-gray-500 flex items-center gap-1.5">
                <svg
                  class="w-3 h-3 transition-transform group-open:rotate-90"
                  fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2"
                >
                  <path d="M9 5l7 7-7 7" />
                </svg>
                Advanced: download generated config.h / config.c directly
              </summary>
              <div class="flex flex-wrap gap-3 pt-2 pl-4">
                <button
                  type="button"
                  class="inline-flex items-center gap-1.5 px-3 py-2 text-sm font-medium rounded-md border transition-colors font-mono bg-gray-800 hover:bg-gray-700 text-gray-200 border-gray-700"
                  @click="downloadH"
                >
                  &darr; config.h
                </button>
                <button
                  type="button"
                  class="inline-flex items-center gap-1.5 px-3 py-2 text-sm font-medium rounded-md border transition-colors font-mono bg-gray-800 hover:bg-gray-700 text-gray-200 border-gray-700"
                  @click="downloadC"
                >
                  &darr; config.c
                </button>
              </div>
            </details>
          </div>

          <!-- Build the firmware -->
          <div class="space-y-2">
            <p class="font-medium text-gray-300">Build the firmware</p>
            <CommandBlock
              v-if="os === 'windows'"
              command="build-firmware.bat"
              label="cmd.exe"
            />
            <CommandBlock
              v-else
              command="./build-firmware.sh"
              label="terminal"
            />
          </div>

          <p class="text-xs text-gray-600">
            The build script reads your <code class="font-mono">config.yaml</code> to detect the
            hardware type automatically. To use a different config file, pass it as the first
            argument, e.g. <code class="font-mono">./build-firmware.sh my-tracker.yaml</code>. If
            building without config.yaml, pass the hardware flag directly, e.g.
            <code class="font-mono">./build-firmware.sh -DRS41_RSM4X4=1</code>.
          </p>

          <p class="text-xs text-gray-600">
            Output: <code class="font-mono">build/RS41ng.bin</code>
          </p>
        </div>
      </OsTabGroup>
    </section>

    <!-- Navigation -->
    <div class="flex justify-between pt-4 border-t border-gray-800">
      <button
        type="button"
        class="px-4 py-2 text-sm font-medium bg-gray-800 hover:bg-gray-700 text-gray-300 rounded-md transition-colors"
        @click="ui.goBack()"
      >
        &larr; Back
      </button>
      <button
        type="button"
        class="px-4 py-2 text-sm font-medium bg-green-600 hover:bg-green-500 text-white rounded-md transition-colors"
        @click="ui.goNext()"
      >
        Next: Flash &rarr;
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useUiStore } from "@/stores/ui";
import { schema } from "@/config/schema-loader";
import { useConfigStore } from "@/stores/config";
import { generateConfigYaml } from "@/config/generate-yaml";
import { generateConfigH } from "@/config/generate-header";
import { generateConfigC } from "@/config/generate-source";
import { downloadFile } from "@/utils/download";
import OsTabGroup from "@/components/build/OsTabGroup.vue";
import CommandBlock from "@/components/build/CommandBlock.vue";

const ui = useUiStore();
const configStore = useConfigStore();

function downloadYaml() {
  downloadFile("config.yaml", "application/yaml", generateConfigYaml(schema, configStore.values));
}

function downloadH() {
  downloadFile("config.h", "text/plain", generateConfigH(schema, configStore.values));
}

function downloadC() {
  downloadFile("config.c", "text/plain", generateConfigC(schema, configStore.values));
}
</script>
