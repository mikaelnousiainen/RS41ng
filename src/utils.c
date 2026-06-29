#include "utils.h"

// Replaces every occurrence of "rep" in "orig" with "with", writing the result
// into "dest" (a buffer of dest_len bytes, including the terminating NUL).
//
// The output is always NUL-terminated and never exceeds dest_len. If the fully
// expanded result would not fit, it is truncated to fill the buffer rather than
// dropped: replacements are still performed up to the point the buffer fills,
// so the destination never contains unexpanded "rep" tokens that could have
// been replaced within the available space. This matters for templated radio
// payloads (e.g. APRS comments), where a buffer-sized message must contain the
// substituted values, not the literal template variables.
//
// Returns the length (excluding NUL) of the string written to dest.
size_t str_replace(char *dest, size_t dest_len, char *orig, char *rep, char *with)
{
    if (!dest || dest_len == 0) {
        return 0;
    }

    if (!orig || !rep) {
        dest[0] = '\0';
        return 0;
    }

    size_t len_rep = strlen(rep);
    char *out = dest;
    size_t remaining = dest_len - 1; // reserve space for the terminating NUL

    // An empty "rep" would match endlessly; just copy orig verbatim (truncated).
    if (len_rep == 0) {
        size_t n = strlen(orig);
        if (n > remaining) {
            n = remaining;
        }
        memcpy(out, orig, n);
        out += n;
        *out = '\0';
        return (size_t) (out - dest);
    }

    if (!with) {
        with = "";
    }
    size_t len_with = strlen(with);

    char *cursor = orig;
    char *match;
    while (remaining > 0 && (match = strstr(cursor, rep)) != NULL) {
        // Copy the literal segment preceding the match.
        size_t front = (size_t) (match - cursor);
        if (front > remaining) {
            front = remaining;
        }
        memcpy(out, cursor, front);
        out += front;
        remaining -= front;
        if (remaining == 0) {
            break;
        }

        // Copy the replacement, truncating it if the buffer is nearly full.
        size_t with_copy = len_with > remaining ? remaining : len_with;
        memcpy(out, with, with_copy);
        out += with_copy;
        remaining -= with_copy;

        cursor = match + len_rep;
    }

    // Copy whatever remains after the last match (or all of orig if no match).
    if (remaining > 0) {
        size_t tail = strlen(cursor);
        if (tail > remaining) {
            tail = remaining;
        }
        memcpy(out, cursor, tail);
        out += tail;
    }

    *out = '\0';
    return (size_t) (out - dest);
}
