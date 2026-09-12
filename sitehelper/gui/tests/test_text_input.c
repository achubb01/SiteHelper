#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "text_edit.h"
#include "length_parse.h"

static void test_edit(void)
{
    TextEdit edit = {0};
    assert(text_edit_insert(&edit, "x") == TEXT_EDIT_INVALID);
    text_edit_begin(&edit);
    assert(text_edit_insert(&edit, "4200mm") == TEXT_EDIT_OK);
    text_edit_apply(&edit, TEXT_EDIT_HOME);
    text_edit_apply(&edit, TEXT_EDIT_RIGHT);
    text_edit_apply(&edit, TEXT_EDIT_DELETE);
    assert(strcmp(edit.text, "400mm") == 0 && edit.cursor == 1);
    assert(text_edit_insert(&edit, "2") == TEXT_EDIT_OK);
    text_edit_apply(&edit, TEXT_EDIT_BACKSPACE);
    text_edit_apply(&edit, TEXT_EDIT_LEFT);
    text_edit_apply(&edit, TEXT_EDIT_BACKSPACE);
    assert(strcmp(edit.text, "400mm") == 0 && edit.cursor == 0);
    text_edit_apply(&edit, TEXT_EDIT_END);
    text_edit_apply(&edit, TEXT_EDIT_RIGHT);
    text_edit_apply(&edit, TEXT_EDIT_DELETE);
    assert(edit.cursor == edit.length);
    text_edit_clear(&edit);
    assert(edit.active && !edit.length && !edit.cursor && !edit.text[0]);
    assert(text_edit_insert(&edit, "a\xc3\xa9\xf0\x9f\x98\x80z") == TEXT_EDIT_OK);
    text_edit_apply(&edit, TEXT_EDIT_LEFT);
    text_edit_apply(&edit, TEXT_EDIT_BACKSPACE);
    assert(strcmp(edit.text, "a\xc3\xa9z") == 0 && edit.cursor == 3);
    text_edit_apply(&edit, TEXT_EDIT_LEFT);
    text_edit_apply(&edit, TEXT_EDIT_DELETE);
    assert(strcmp(edit.text, "az") == 0 && edit.cursor == 1);
    const char *bad[] = {"\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe2", "\n", "\x80"};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        assert(text_edit_insert(&edit, bad[i]) == TEXT_EDIT_INVALID);
        assert(strcmp(edit.text, "az") == 0 && edit.cursor == 1);
    }
    text_edit_clear(&edit);
    char full[TEXT_EDIT_CAPACITY];
    memset(full, '1', sizeof full - 1); full[sizeof full - 1] = 0;
    assert(text_edit_insert(&edit, full) == TEXT_EDIT_OK);
    assert(text_edit_insert(&edit, "2") == TEXT_EDIT_FULL);
    assert(edit.length == TEXT_EDIT_CAPACITY - 1 && strcmp(edit.text, full) == 0);
    text_edit_apply(&edit, TEXT_EDIT_BACKSPACE);
    assert(text_edit_insert(&edit, "2") == TEXT_EDIT_OK);
    text_edit_end(&edit);
    assert(!edit.active && !edit.length && !edit.cursor && !edit.text[0]);
}

static void test_parse(void)
{
    const struct {const char *text; int mm;} valid[] = {
        {"4200",4200},{"4200mm",4200},{"4.2m",4200},{" \t4.2000 m\r\n",4200},
        {"4200.000mm",4200},{".001m",1},{"0",0},{"-42mm",-42},
        {"+2m",2000},{"-0.001m",-1},{"00000000000000004.2m",4200}
    };
    int mm;
    for (size_t i = 0; i < sizeof valid / sizeof *valid; i++) {
        assert(length_parse_mm(valid[i].text, &mm) == LENGTH_PARSE_OK && mm == valid[i].mm);
    }
    const char *bad[] = {"42junk", "4.2mx", "1e3", "nan", "inf", "1 2", "1mm m", ".", "+", "4.", "1MM", "0x10", "1,000"};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        mm = 123;
        assert(length_parse_mm(bad[i], &mm) == LENGTH_PARSE_INVALID && mm == 123);
    }
    assert(length_parse_mm("", &mm) == LENGTH_PARSE_EMPTY);
    assert(length_parse_mm(" \t", &mm) == LENGTH_PARSE_EMPTY);
    assert(length_parse_mm("0.1", &mm) == LENGTH_PARSE_FRACTIONAL_MM);
    assert(length_parse_mm("0.0001m", &mm) == LENGTH_PARSE_FRACTIONAL_MM);
    assert(length_parse_mm("4.200000000000000000001m", &mm) == LENGTH_PARSE_FRACTIONAL_MM);
    char text[80];
    snprintf(text, sizeof text, "%d", INT_MAX);
    assert(length_parse_mm(text, &mm) == LENGTH_PARSE_OK && mm == INT_MAX);
    snprintf(text, sizeof text, "%d", INT_MIN);
    assert(length_parse_mm(text, &mm) == LENGTH_PARSE_OK && mm == INT_MIN);
    snprintf(text, sizeof text, "%lld", (long long)INT_MAX + 1);
    assert(length_parse_mm(text, &mm) == LENGTH_PARSE_OVERFLOW);
    snprintf(text, sizeof text, "%lld", (long long)INT_MIN - 1);
    assert(length_parse_mm(text, &mm) == LENGTH_PARSE_OVERFLOW);
    assert(length_parse_mm("999999999999999999999999999999m", &mm) == LENGTH_PARSE_OVERFLOW);
    snprintf(text, sizeof text, "%dm", INT_MAX);
    assert(length_parse_mm(text, &mm) == LENGTH_PARSE_OVERFLOW);
    assert(length_parse_mm(NULL, &mm) == LENGTH_PARSE_INVALID);
    assert(length_parse_mm("1", NULL) == LENGTH_PARSE_INVALID);
}
int main(void) { test_edit(); test_parse(); puts("text input tests passed"); }
