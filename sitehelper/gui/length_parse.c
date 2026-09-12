#include "length_parse.h"
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

static int space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}
static int digit(char c) { return c >= '0' && c <= '9'; }

LengthParseStatus length_parse_mm(const char *text, int *millimetres)
{
    if (text == NULL || millimetres == NULL) { return LENGTH_PARSE_INVALID; }
    while (space(*text)) { text++; }
    if (!*text) { return LENGTH_PARSE_EMPTY; }
    int negative = *text == '-';
    if (*text == '-' || *text == '+') { text++; }
    const char *number = text;
    size_t digits = 0, fractional = 0;
    while (digit(*text)) { digits++; text++; }
    if (*text == '.') {
        text++;
        while (digit(*text)) { digits++; fractional++; text++; }
        if (!fractional) { return LENGTH_PARSE_INVALID; }
    }
    if (!digits) { return LENGTH_PARSE_INVALID; }
    const char *number_end = text;
    while (space(*text)) { text++; }
    size_t scale = 0;
    if (*text == 'm') {
        text++;
        if (*text == 'm') { text++; } else { scale = 3; }
    }
    while (space(*text)) { text++; }
    if (*text) { return LENGTH_PARSE_INVALID; }

    /* Work in decimal digits, dropping only exact trailing zeroes. No floating
     * point, locale dependence, huge powers of ten or partial-string parsing. */
    size_t discard = fractional > scale ? fractional - scale : 0;
    const char *end = number_end;
    for (size_t i = 0; i < discard; i++) {
        if (*--end != '0') { return LENGTH_PARSE_FRACTIONAL_MM; }
    }
    uint64_t limit = negative ? (uint64_t)(-(int64_t)INT_MIN) : (uint64_t)INT_MAX;
    uint64_t value = 0;
    for (const char *p = number; p < end; p++) {
        if (*p == '.') { continue; }
        unsigned d = (unsigned)(*p - '0');
        if (value > (limit - d) / 10) { return LENGTH_PARSE_OVERFLOW; }
        value = value * 10 + d;
    }
    for (size_t i = fractional; i < scale; i++) {
        if (value > limit / 10) { return LENGTH_PARSE_OVERFLOW; }
        value *= 10;
    }
    *millimetres = negative ? (int)(-(int64_t)value) : (int)value;
    return LENGTH_PARSE_OK;
}
