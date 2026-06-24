import type { FlashStrategy } from "./flash-strategy";
import { WebStlinkStrategy } from "./strategies/webstlink-strategy";

const strategies: FlashStrategy[] = [new WebStlinkStrategy()];

/** Get all strategies available in the current browser */
export function getAvailableStrategies(): FlashStrategy[] {
  return strategies.filter((s) => s.available);
}

/** Get a strategy by id, or undefined */
export function getStrategy(id: string): FlashStrategy | undefined {
  return strategies.find((s) => s.id === id);
}

/** Get the default (first available) strategy, or undefined */
export function getDefaultStrategy(): FlashStrategy | undefined {
  return strategies.find((s) => s.available);
}
