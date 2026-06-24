import { describe, it, expect } from "vitest";
import { mount } from "@vue/test-utils";
import { createPinia } from "pinia";
import WizardShell from "@/components/layout/WizardShell.vue";
import { useUiStore } from "@/stores/ui";

function mountWizard() {
  const pinia = createPinia();
  const wrapper = mount(WizardShell, {
    global: { plugins: [pinia] },
    slots: { default: "<p>slot content</p>" },
  });
  return { wrapper, ui: useUiStore(pinia) };
}

describe("WizardShell", () => {
  it("renders all four step buttons", () => {
    const { wrapper } = mountWizard();
    const buttons = wrapper.findAll("nav button");
    expect(buttons).toHaveLength(4);
    expect(buttons[0].text()).toContain("Hardware");
    expect(buttons[1].text()).toContain("Configure");
    expect(buttons[2].text()).toContain("Build");
    expect(buttons[3].text()).toContain("Flash");
  });

  it("renders the slot content", () => {
    const { wrapper } = mountWizard();
    expect(wrapper.html()).toContain("slot content");
  });

  it("shows step 1 (Hardware) as active initially", () => {
    const { wrapper } = mountWizard();
    const firstButton = wrapper.findAll("nav button")[0];
    expect(firstButton.classes()).toContain("text-white");
  });

  it("locks later steps when no hardware is selected", () => {
    const { wrapper } = mountWizard();
    const buttons = wrapper.findAll("nav button");
    // Steps 2-4 should be locked (cursor-not-allowed)
    expect(buttons[1].classes()).toContain("cursor-not-allowed");
    expect(buttons[2].classes()).toContain("cursor-not-allowed");
    expect(buttons[3].classes()).toContain("cursor-not-allowed");
  });

  it("navigates to configure step after hardware is selected", async () => {
    const { wrapper, ui } = mountWizard();
    ui.hardwareVariant = "RS41";
    await wrapper.vm.$nextTick();
    const buttons = wrapper.findAll("nav button");
    await buttons[1].trigger("click");
    expect(buttons[1].classes()).toContain("text-white");
    expect(buttons[0].classes()).not.toContain("text-white");
  });

  it("renders the RS41ng header", () => {
    const { wrapper } = mountWizard();
    expect(wrapper.html()).toContain("RS41ng");
  });
});
