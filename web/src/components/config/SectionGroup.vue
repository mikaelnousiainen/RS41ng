<template>
  <section v-if="sectionVisible && visibleFields.length > 0" class="space-y-4">
    <div>
      <h2 class="text-sm font-semibold text-gray-300 uppercase tracking-wider">
        {{ meta?._section ?? sectionKey }}
      </h2>
      <p v-if="meta?._description" class="text-xs text-gray-500 mt-0.5">
        {{ meta._description }}
      </p>
    </div>

    <div class="space-y-4 pl-0">
      <template v-for="([fieldKey, field], idx) in visibleFields" :key="fieldKey">
        <!-- Group sub-heading: rendered when a field starts a new group -->
        <h3
          v-if="field.group && field.group !== visibleFields[idx - 1]?.[1]?.group"
          class="text-xs font-medium text-gray-400 uppercase tracking-wider pt-3 pb-1 border-t border-gray-800"
          :class="{ 'mt-2': idx > 0 }"
        >
          {{ field.group }}
        </h3>
        <FieldRenderer
          :sectionKey="sectionKey"
          :fieldKey="fieldKey"
          :field="field"
        />
      </template>
    </div>

    <slot name="after" />
  </section>
</template>

<script setup lang="ts">
import { computed } from "vue";
import type { Schema, SchemaField } from "@/types/schema";
import type { Tier } from "@/stores/ui";
import { getSectionMeta, getSectionFieldKeys, getField } from "@/config/schema-registry";
import { isFieldVisible, isSectionVisible } from "@/config/visibility";
import { useConfigStore } from "@/stores/config";
import FieldRenderer from "./FieldRenderer.vue";

/** Tier order: fields at or below the selected tier are shown */
const TIER_ORDER: Record<Tier, number> = {
  simple: 0,
  intermediate: 1,
  advanced: 2,
};

const props = defineProps<{
  schema: Schema;
  sectionKey: string;
  tier: Tier;
}>();

const configStore = useConfigStore();

const meta = computed(() => getSectionMeta(props.schema, props.sectionKey));

const sectionVisible = computed(() =>
  isSectionVisible(props.schema, props.sectionKey, configStore.values)
);

const visibleFields = computed((): [string, SchemaField][] =>
  getSectionFieldKeys(props.schema, props.sectionKey).flatMap((fieldKey) => {
    const field = getField(props.schema, props.sectionKey, fieldKey);
    if (!field) return [];
    // Skip alias fields — they don't have their own UI
    if (field.alias_of !== undefined) return [];
    // Tier filter: only show fields at or below the selected tier
    if (TIER_ORDER[field.tier] > TIER_ORDER[props.tier]) return [];
    // Visibility filter: condition-based (hardware type, mode enables, etc.)
    if (!isFieldVisible(field, configStore.values)) return [];
    return [[fieldKey, field] as [string, SchemaField]];
  })
);
</script>
