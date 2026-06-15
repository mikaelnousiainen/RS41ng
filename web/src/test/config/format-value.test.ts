import { describe, it, expect } from "vitest";
import type { SchemaField } from "@/types/schema";
import { escapeString, formatChar, formatValue } from "@/config/format-value";

// ─── escapeString ──────────────────────────────────────────────────────────────

describe("escapeString", () => {
  it("returns plain strings unchanged", () => {
    expect(escapeString("hello")).toBe("hello");
  });

  it("escapes backslashes", () => {
    expect(escapeString("a\\b")).toBe("a\\\\b");
  });

  it("escapes double-quotes", () => {
    expect(escapeString('say "hi"')).toBe('say \\"hi\\"');
  });

  it("escapes both in one string", () => {
    expect(escapeString('a\\"b')).toBe('a\\\\\\"b');
  });
});

// ─── formatChar ────────────────────────────────────────────────────────────────

describe("formatChar", () => {
  it("formats a normal character", () => {
    expect(formatChar("9")).toBe("'9'");
  });

  it("formats backslash as escaped literal", () => {
    expect(formatChar("\\")).toBe("'\\\\'");
  });

  it("formats single-quote", () => {
    expect(formatChar("'")).toBe("\"'\\''\"");
  });
});

// ─── formatValue ───────────────────────────────────────────────────────────────

function field(overrides: Partial<SchemaField> & { type: string }): SchemaField {
  return {
    define: "TEST",
    label: "Test",
    tier: "simple",
    ...overrides,
  } as unknown as SchemaField;
}

describe("formatValue — string", () => {
  it("wraps in double-quotes", () => {
    expect(formatValue(field({ type: "string" }), "OH2A")).toBe('"OH2A"');
  });

  it("escapes quotes inside string value", () => {
    expect(formatValue(field({ type: "string" }), 'say "hi"')).toBe('"say \\"hi\\""');
  });
});

describe("formatValue — bool", () => {
  it("returns true for boolean true", () => {
    expect(formatValue(field({ type: "bool" }), true)).toBe("true");
  });

  it("returns false for boolean false", () => {
    expect(formatValue(field({ type: "bool" }), false)).toBe("false");
  });

  it("accepts string 'true'", () => {
    expect(formatValue(field({ type: "bool" }), "true")).toBe("true");
  });

  it("treats other strings as false", () => {
    expect(formatValue(field({ type: "bool" }), "false")).toBe("false");
  });
});

describe("formatValue — integer", () => {
  it("converts number to string", () => {
    expect(formatValue(field({ type: "integer" }), 7)).toBe("7");
  });

  it("converts string number", () => {
    expect(formatValue(field({ type: "integer" }), "42")).toBe("42");
  });
});

describe("formatValue — hex", () => {
  it("returns value as-is", () => {
    expect(formatValue(field({ type: "hex" }), "0x1234ABCD")).toBe("0x1234ABCD");
  });
});

describe("formatValue — frequency", () => {
  it("appends suffix when present", () => {
    expect(formatValue(field({ type: "frequency", suffix: "UL" }), 434500000)).toBe(
      "434500000UL"
    );
  });

  it("emits value without suffix when absent", () => {
    expect(formatValue(field({ type: "frequency" }), 434500000)).toBe("434500000");
  });
});

describe("formatValue — char", () => {
  it("wraps a normal char in single-quotes", () => {
    expect(formatValue(field({ type: "char" }), "A")).toBe("'A'");
  });
});

describe("formatValue — enum", () => {
  it("returns value as string", () => {
    expect(formatValue(field({ type: "enum" }), "RS41")).toBe("RS41");
  });
});

describe("formatValue — enum_constant", () => {
  it("returns numeric value as string", () => {
    expect(formatValue(field({ type: "enum_constant" }), 6)).toBe("6");
  });
});

describe("formatValue — unknown type", () => {
  it("throws an error", () => {
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    expect(() => formatValue(field({ type: "unknown_type" as any }), "x")).toThrow(
      "Unknown field type"
    );
  });
});
