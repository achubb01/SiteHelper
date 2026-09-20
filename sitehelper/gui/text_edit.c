#include "text_edit.h"
#include <stdint.h>
#include <string.h>

void text_edit_begin(TextEdit *edit)
{
    if (edit != NULL) { *edit = (TextEdit){.active = 1}; }
}
void text_edit_clear(TextEdit *edit)
{
    if (edit != NULL) { int active = edit->active; *edit = (TextEdit){.active = active}; }
}
void text_edit_end(TextEdit *edit)
{
    if (edit != NULL) { *edit = (TextEdit){0}; }
}

static int continuation(unsigned char c) { return (c & 0xc0) == 0x80; }

static int valid_text(const unsigned char *s, int allow_newline)
{
    while (*s) {
        uint32_t cp;
        int count;
        unsigned char c = *s++;
        if (c < 0x80) {
            if (c == '\n' && allow_newline) { continue; }
            if (c < 0x20 || c == 0x7f) { return 0; }
            continue;
        }
        if (c >= 0xc2 && c <= 0xdf) { cp = c & 31; count = 1; }
        else if (c >= 0xe0 && c <= 0xef) { cp = c & 15; count = 2; }
        else if (c >= 0xf0 && c <= 0xf4) { cp = c & 7; count = 3; }
        else { return 0; }
        int bytes = count;
        while (count--) {
            if (!continuation(*s)) { return 0; }
            cp = (cp << 6) | (*s++ & 63);
        }
        if ((bytes == 2 && cp < 0x800) || (bytes == 3 && cp < 0x10000) ||
            (cp >= 0xd800 && cp <= 0xdfff) || cp > 0x10ffff ||
            (cp >= 0x80 && cp <= 0x9f)) { return 0; }
    }
    return 1;
}

static TextEditResult insert_text(TextEdit *edit, const char *text, int allow_newline)
{
    if (edit == NULL || !edit->active || text == NULL) { return TEXT_EDIT_INVALID; }
    size_t length = strlen(text);
    if (length > TEXT_EDIT_CAPACITY - 1 - edit->length) { return TEXT_EDIT_FULL; }
    if (!valid_text((const unsigned char *)text, allow_newline)) { return TEXT_EDIT_INVALID; }
    /* Copy first so insertion also supports an aliased source. */
    char copy[TEXT_EDIT_CAPACITY];
    memcpy(copy, text, length);
    memmove(edit->text + edit->cursor + length, edit->text + edit->cursor,
        edit->length - edit->cursor + 1);
    memcpy(edit->text + edit->cursor, copy, length);
    edit->cursor += length;
    edit->length += length;
    return TEXT_EDIT_OK;
}

TextEditResult text_edit_insert(TextEdit *edit, const char *text)
{
    return insert_text(edit,text,0);
}

TextEditResult text_edit_insert_multiline(TextEdit *edit, const char *text)
{
    return insert_text(edit,text,1);
}

TextEditResult text_edit_insert_newline(TextEdit *edit)
{
    if (edit == NULL || !edit->active) { return TEXT_EDIT_INVALID; }
    if (edit->length >= TEXT_EDIT_CAPACITY - 1) { return TEXT_EDIT_FULL; }
    memmove(edit->text + edit->cursor + 1, edit->text + edit->cursor,
        edit->length - edit->cursor + 1);
    edit->text[edit->cursor++] = '\n';
    edit->length++;
    return TEXT_EDIT_OK;
}

void text_edit_apply(TextEdit *edit, TextEditOperation operation)
{
    if (edit == NULL || !edit->active) { return; }
    size_t left = edit->cursor, right = edit->cursor;
    if (left) { do { left--; } while (left && continuation((unsigned char)edit->text[left])); }
    if (right < edit->length) {
        do { right++; } while (right < edit->length && continuation((unsigned char)edit->text[right]));
    }
    size_t begin, end;
    switch (operation) {
        case TEXT_EDIT_LEFT: edit->cursor = left; return;
        case TEXT_EDIT_RIGHT: edit->cursor = right; return;
        case TEXT_EDIT_HOME: edit->cursor = 0; return;
        case TEXT_EDIT_END: edit->cursor = edit->length; return;
        case TEXT_EDIT_BACKSPACE: begin = left; end = edit->cursor; break;
        case TEXT_EDIT_DELETE: begin = edit->cursor; end = right; break;
        default: return;
    }
    memmove(edit->text + begin, edit->text + end, edit->length - end + 1);
    edit->length -= end - begin;
    edit->cursor = begin;
}
