/**
 * Whether the current browser exposes the WebUSB API (navigator.usb), which the
 * in-browser ST-LINK flasher requires. False in Firefox and Safari, true in
 * Chromium-based browsers (Chrome, Edge). Guards against non-browser/SSR contexts.
 */
export function isWebUsbSupported(): boolean {
  return typeof navigator !== "undefined" && "usb" in navigator;
}
