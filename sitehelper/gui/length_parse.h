#ifndef LENGTH_PARSE_H
#define LENGTH_PARSE_H

typedef enum {
    LENGTH_PARSE_OK, LENGTH_PARSE_EMPTY, LENGTH_PARSE_INVALID,
    LENGTH_PARSE_OVERFLOW, LENGTH_PARSE_FRACTIONAL_MM
} LengthParseStatus;

/* Signed decimal, optional lowercase mm/m, ASCII surrounding whitespace.
 * Bare numbers are mm. No exponent syntax or rounding. Output unchanged on
 * failure. Zero/negative values are representable; consumers decide semantics. */
LengthParseStatus length_parse_mm(const char *text, int *millimetres);

#endif
