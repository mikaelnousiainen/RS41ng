/**
 * Trigger a browser download of in-memory text content by creating a temporary
 * object URL and clicking a synthetic anchor.
 */
export function downloadFile(
  filename: string,
  mimeType: string,
  content: string
): void {
  const blob = new Blob([content], { type: mimeType });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  a.click();
  URL.revokeObjectURL(url);
}
