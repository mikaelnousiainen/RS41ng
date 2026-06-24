<template>
  <div>
    <button
      type="button"
      class="text-xs text-green-400 hover:text-green-300 transition-colors"
      @click="open = true"
    >
      Show template variables reference
    </button>

    <!-- Backdrop + dialog -->
    <Teleport to="body">
      <div
        v-if="open"
        class="fixed inset-0 z-50 flex items-center justify-center p-4"
      >
        <!-- Backdrop -->
        <div
          class="absolute inset-0 bg-black/60"
          @click="open = false"
        />

        <!-- Dialog -->
        <div class="relative bg-gray-900 border border-gray-700 rounded-lg shadow-xl max-w-lg w-full max-h-[80vh] flex flex-col">
          <div class="flex items-center justify-between px-4 py-3 border-b border-gray-800">
            <h3 class="text-sm font-semibold text-gray-200">Template Variables</h3>
            <button
              type="button"
              class="text-gray-500 hover:text-gray-300 transition-colors"
              @click="open = false"
            >
              <svg class="w-4 h-4" viewBox="0 0 16 16" fill="currentColor">
                <path d="M4.646 4.646a.5.5 0 01.708 0L8 7.293l2.646-2.647a.5.5 0 01.708.708L8.707 8l2.647 2.646a.5.5 0 01-.708.708L8 8.707l-2.646 2.647a.5.5 0 01-.708-.708L7.293 8 4.646 5.354a.5.5 0 010-.708z" />
              </svg>
            </button>
          </div>

          <div class="overflow-y-auto px-4 py-3">
            <p class="text-xs text-gray-500 mb-3">
              Use these variables in message templates. They are replaced with live values at transmission time.
            </p>
            <table class="w-full text-sm">
              <thead>
                <tr class="text-left text-xs text-gray-500 uppercase tracking-wider">
                  <th class="pb-2 pr-4">Variable</th>
                  <th class="pb-2">Description</th>
                </tr>
              </thead>
              <tbody class="text-gray-300">
                <tr
                  v-for="tv in templateVars"
                  :key="tv.var"
                  class="border-t border-gray-800"
                >
                  <td class="py-1.5 pr-4 font-mono text-xs text-green-400 whitespace-nowrap">{{ tv.var }}</td>
                  <td class="py-1.5 text-xs text-gray-400">{{ tv.description }}</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </Teleport>
  </div>
</template>

<script setup lang="ts">
import { ref } from "vue";
import { schema } from "@/config/schema-loader";
import type { TemplateVariable } from "@/types/schema";

const open = ref(false);

const templateVars: TemplateVariable[] = schema._template_variables ?? [];
</script>
