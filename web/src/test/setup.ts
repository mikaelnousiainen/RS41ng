import { createPinia, setActivePinia } from "pinia";
import { beforeEach } from "vitest";

beforeEach(() => {
  setActivePinia(createPinia());
});
