<template>
  <div class="space-y-6">
    <div>
      <h1 class="text-xl font-semibold text-gray-100">Configure Firmware</h1>
      <p class="mt-1 text-sm text-gray-400">
        Set up your radiosonde hardware and transmission modes.
      </p>
    </div>

    <ConflictBanner :schema="schema" />

    <PresetSelector />

    <TierSelector />

    <!-- One collapsible group per schema section, hardware section excluded (set in step 1) -->
    <div class="space-y-8">
      <SectionGroup
        v-for="sectionKey in sectionKeys"
        :key="sectionKey"
        :schema="schema"
        :sectionKey="sectionKey"
        :tier="ui.tier"
      />
    </div>

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
        Next: Build &rarr;
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from "vue";
import { useUiStore } from "@/stores/ui";
import { schema } from "@/config/schema-loader";
import { getSortedSectionKeys } from "@/config/schema-registry";
import ConflictBanner from "@/components/config/ConflictBanner.vue";
import PresetSelector from "@/components/config/PresetSelector.vue";
import TierSelector from "@/components/config/TierSelector.vue";
import SectionGroup from "@/components/config/SectionGroup.vue";

const ui = useUiStore();

const sectionKeys = computed(() =>
  getSortedSectionKeys(schema).filter((key: string) => key !== "hardware")
);
</script>
