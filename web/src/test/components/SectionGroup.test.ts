import { describe, it, expect, beforeEach } from "vitest";
import { mount } from "@vue/test-utils";
import { createPinia, setActivePinia } from "pinia";
import SectionGroup from "@/components/config/SectionGroup.vue";
import type { Schema } from "@/types/schema";

function makeSchema(): Schema {
  return {
    global: {
      _section: "Global Settings",
      _tier: "simple",
      _order: 0,
      callsign: {
        define: "CALLSIGN",
        type: "string",
        label: "Callsign",
        tier: "simple",
        default: "N0CALL",
      },
      tx_power: {
        define: "TX_POWER",
        type: "integer",
        label: "TX Power",
        tier: "intermediate",
        default: 7,
      },
      advanced_opt: {
        define: "ADV_OPT",
        type: "integer",
        label: "Advanced Option",
        tier: "advanced",
        default: 0,
      },
      // alias field — should never render
      aprs_callsign: {
        define: "APRS_CALLSIGN",
        type: "string",
        label: "APRS Callsign",
        tier: "simple",
        alias_of: "global.callsign",
      },
      // conditional field — hidden when tx_aprs is false
      tx_aprs_count: {
        define: "TX_APRS_COUNT",
        type: "integer",
        label: "APRS Count",
        tier: "simple",
        default: 2,
        visible_when: "global.tx_aprs",
      },
      tx_aprs: {
        define: "TX_APRS",
        type: "bool",
        label: "Enable APRS",
        tier: "simple",
        default: false,
      },
    },
  } as unknown as Schema;
}

describe("SectionGroup", () => {
  beforeEach(() => {
    setActivePinia(createPinia());
  });

  it("shows the section title", () => {
    const wrapper = mount(SectionGroup, {
      props: { schema: makeSchema(), sectionKey: "global", tier: "simple" },
    });
    expect(wrapper.text()).toContain("Global Settings");
  });

  it("renders simple fields when tier is simple", () => {
    const wrapper = mount(SectionGroup, {
      props: { schema: makeSchema(), sectionKey: "global", tier: "simple" },
    });
    expect(wrapper.text()).toContain("Callsign");
    expect(wrapper.text()).not.toContain("TX Power");
    expect(wrapper.text()).not.toContain("Advanced Option");
  });

  it("renders simple + intermediate fields when tier is intermediate", () => {
    const wrapper = mount(SectionGroup, {
      props: { schema: makeSchema(), sectionKey: "global", tier: "intermediate" },
    });
    expect(wrapper.text()).toContain("Callsign");
    expect(wrapper.text()).toContain("TX Power");
    expect(wrapper.text()).not.toContain("Advanced Option");
  });

  it("renders all fields when tier is advanced", () => {
    const wrapper = mount(SectionGroup, {
      props: { schema: makeSchema(), sectionKey: "global", tier: "advanced" },
    });
    expect(wrapper.text()).toContain("Callsign");
    expect(wrapper.text()).toContain("TX Power");
    expect(wrapper.text()).toContain("Advanced Option");
  });

  it("does not render alias_of fields", () => {
    const wrapper = mount(SectionGroup, {
      props: { schema: makeSchema(), sectionKey: "global", tier: "advanced" },
    });
    expect(wrapper.text()).not.toContain("APRS Callsign");
  });

  it("hides fields whose visible_when condition is false", () => {
    // tx_aprs is false by default → tx_aprs_count should be hidden
    const wrapper = mount(SectionGroup, {
      props: { schema: makeSchema(), sectionKey: "global", tier: "simple" },
    });
    expect(wrapper.text()).not.toContain("APRS Count");
  });

  it("renders nothing when section has no visible fields", () => {
    // Empty schema section
    const schema: Schema = {
      empty: {
        _section: "Empty Section",
        _tier: "advanced",
        _order: 0,
      },
    } as unknown as Schema;
    const wrapper = mount(SectionGroup, {
      props: { schema, sectionKey: "empty", tier: "simple" },
    });
    expect(wrapper.find("section").exists()).toBe(false);
  });
});
