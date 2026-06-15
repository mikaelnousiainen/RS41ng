import { describe, it, expect } from "vitest";
import { useFlasherStore } from "@/stores/flasher";

describe("useFlasherStore", () => {
  describe("initial state", () => {
    it("starts with idle status", () => {
      const flasher = useFlasherStore();
      expect(flasher.status).toBe("idle");
    });

    it("starts with zero progress", () => {
      const flasher = useFlasherStore();
      expect(flasher.progress).toBe(0);
    });

    it("starts with empty log", () => {
      const flasher = useFlasherStore();
      expect(flasher.log).toEqual([]);
    });

    it("starts with null chipId", () => {
      const flasher = useFlasherStore();
      expect(flasher.chipId).toBeNull();
    });
  });

  describe("addLog", () => {
    it("appends an info entry", () => {
      const flasher = useFlasherStore();
      flasher.addLog("info", "connecting...");
      expect(flasher.log).toHaveLength(1);
      expect(flasher.log[0].level).toBe("info");
      expect(flasher.log[0].message).toBe("connecting...");
    });

    it("appends multiple entries in order", () => {
      const flasher = useFlasherStore();
      flasher.addLog("info", "first");
      flasher.addLog("warn", "second");
      flasher.addLog("error", "third");
      expect(flasher.log).toHaveLength(3);
      expect(flasher.log[0].message).toBe("first");
      expect(flasher.log[2].level).toBe("error");
    });

    it("includes a timestamp", () => {
      const before = Date.now();
      const flasher = useFlasherStore();
      flasher.addLog("info", "test");
      const after = Date.now();
      expect(flasher.log[0].timestamp).toBeGreaterThanOrEqual(before);
      expect(flasher.log[0].timestamp).toBeLessThanOrEqual(after);
    });
  });

  describe("reset", () => {
    it("clears all state back to initial", () => {
      const flasher = useFlasherStore();
      flasher.status = "connected";
      flasher.progress = 50;
      flasher.chipId = "0x10016444";
      flasher.addLog("info", "some log");

      flasher.reset();

      expect(flasher.status).toBe("idle");
      expect(flasher.progress).toBe(0);
      expect(flasher.chipId).toBeNull();
      expect(flasher.log).toEqual([]);
    });
  });
});
