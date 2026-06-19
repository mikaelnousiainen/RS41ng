import { describe, it, expect } from "vitest";
import { mount } from "@vue/test-utils";
import { createPinia } from "pinia";
import App from "@/App.vue";
import { useUiStore } from "@/stores/ui";
import { useConfigStore } from "@/stores/config";

function mountApp() {
  const pinia = createPinia();
  const wrapper = mount(App, {
    global: { plugins: [pinia] },
  });
  return { wrapper, ui: useUiStore(pinia), config: useConfigStore(pinia) };
}

describe("App", () => {
  it("renders HardwareStep by default", () => {
    const { wrapper } = mountApp();
    expect(wrapper.html()).toContain("Select Hardware");
  });

  it("shows ConfigureStep after selecting hardware and navigating", async () => {
    const { wrapper, ui } = mountApp();
    ui.hardwareVariant = "RS41";
    ui.goTo("configure");
    await wrapper.vm.$nextTick();
    expect(wrapper.html()).toContain("Configure Firmware");
  });

  it("shows BuildStep after navigating to build", async () => {
    const { wrapper, ui } = mountApp();
    ui.hardwareVariant = "RS41";
    ui.goTo("build");
    await wrapper.vm.$nextTick();
    expect(wrapper.html()).toContain("Build Firmware");
  });

  it("shows FlashStep after navigating to flash", async () => {
    const { wrapper, ui } = mountApp();
    ui.hardwareVariant = "RS41";
    ui.goTo("flash");
    await wrapper.vm.$nextTick();
    expect(wrapper.html()).toContain("Flash Firmware");
  });

  it("navigates forward from hardware with Next button", async () => {
    const { wrapper, ui, config } = mountApp();
    // Select hardware via the card click
    ui.hardwareVariant = "RS41";
    config.set("hardware", "type", "RS41");
    await wrapper.vm.$nextTick();
    // Click "Next: Configure"
    const allButtons = wrapper.findAll("button");
    const nextBtn = allButtons.find((b) =>
      b.text().includes("Next: Configure")
    );
    expect(nextBtn).toBeDefined();
    await nextBtn!.trigger("click");
    expect(wrapper.html()).toContain("Configure Firmware");
  });
});
