import { describe, it, expect } from "vitest";
import { useUiStore, STEPS } from "@/stores/ui";

describe("useUiStore", () => {
  /** Helper: select hardware so navigation past step 1 is unlocked */
  function selectHardware(ui: ReturnType<typeof useUiStore>) {
    ui.hardwareVariant = "RS41";
  }

  describe("initial state", () => {
    it("starts on hardware step", () => {
      const ui = useUiStore();
      expect(ui.currentStep).toBe("hardware");
    });

    it("starts with simple tier", () => {
      const ui = useUiStore();
      expect(ui.tier).toBe("simple");
    });

    it("stepIndex is 0 initially", () => {
      const ui = useUiStore();
      expect(ui.stepIndex).toBe(0);
    });

    it("starts with no hardware variant selected", () => {
      const ui = useUiStore();
      expect(ui.hardwareVariant).toBeNull();
      expect(ui.hardwareSelected).toBe(false);
    });
  });

  describe("hardware selection gate", () => {
    it("blocks goTo non-hardware steps when no variant selected", () => {
      const ui = useUiStore();
      ui.goTo("configure");
      expect(ui.currentStep).toBe("hardware");
    });

    it("blocks goNext from hardware when no variant selected", () => {
      const ui = useUiStore();
      ui.goNext();
      expect(ui.currentStep).toBe("hardware");
    });

    it("allows navigation after selecting a variant", () => {
      const ui = useUiStore();
      selectHardware(ui);
      ui.goTo("configure");
      expect(ui.currentStep).toBe("configure");
    });
  });

  describe("goTo", () => {
    it("navigates to the given step", () => {
      const ui = useUiStore();
      selectHardware(ui);
      ui.goTo("build");
      expect(ui.currentStep).toBe("build");
      expect(ui.stepIndex).toBe(2);
    });

    it("navigates to flash", () => {
      const ui = useUiStore();
      selectHardware(ui);
      ui.goTo("flash");
      expect(ui.currentStep).toBe("flash");
      expect(ui.stepIndex).toBe(3);
    });

    it("navigates back to configure", () => {
      const ui = useUiStore();
      selectHardware(ui);
      ui.goTo("flash");
      ui.goTo("configure");
      expect(ui.currentStep).toBe("configure");
    });
  });

  describe("goNext", () => {
    it("advances from hardware to configure", () => {
      const ui = useUiStore();
      selectHardware(ui);
      ui.goNext();
      expect(ui.currentStep).toBe("configure");
    });

    it("advances from configure to build", () => {
      const ui = useUiStore();
      selectHardware(ui);
      ui.goTo("configure");
      ui.goNext();
      expect(ui.currentStep).toBe("build");
    });

    it("advances from build to flash", () => {
      const ui = useUiStore();
      selectHardware(ui);
      ui.goTo("build");
      ui.goNext();
      expect(ui.currentStep).toBe("flash");
    });

    it("stays on flash when already at last step", () => {
      const ui = useUiStore();
      selectHardware(ui);
      ui.goTo("flash");
      ui.goNext();
      expect(ui.currentStep).toBe("flash");
    });
  });

  describe("goBack", () => {
    it("goes from configure to hardware", () => {
      const ui = useUiStore();
      selectHardware(ui);
      ui.goTo("configure");
      ui.goBack();
      expect(ui.currentStep).toBe("hardware");
    });

    it("goes from build to configure", () => {
      const ui = useUiStore();
      selectHardware(ui);
      ui.goTo("build");
      ui.goBack();
      expect(ui.currentStep).toBe("configure");
    });

    it("goes from flash to build", () => {
      const ui = useUiStore();
      selectHardware(ui);
      ui.goTo("flash");
      ui.goBack();
      expect(ui.currentStep).toBe("build");
    });

    it("stays on hardware when already at first step", () => {
      const ui = useUiStore();
      ui.goBack();
      expect(ui.currentStep).toBe("hardware");
    });
  });

  describe("STEPS constant", () => {
    it("has exactly 4 steps in correct order", () => {
      expect(STEPS).toEqual(["hardware", "configure", "build", "flash"]);
    });
  });

  describe("tier", () => {
    it("can be changed to intermediate", () => {
      const ui = useUiStore();
      ui.tier = "intermediate";
      expect(ui.tier).toBe("intermediate");
    });

    it("can be changed to advanced", () => {
      const ui = useUiStore();
      ui.tier = "advanced";
      expect(ui.tier).toBe("advanced");
    });
  });
});
