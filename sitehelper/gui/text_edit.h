#ifndef TEXT_EDIT_H
#define TEXT_EDIT_H

#include <stddef.h>

#define TEXT_EDIT_CAPACITY 128 /* Includes the NUL terminator. */
typedef struct {
    char text[TEXT_EDIT_CAPACITY];
    size_t length;
    size_t cursor; /* UTF-8 byte offset, always a code point boundary. */
    int active;
} TextEdit;

typedef enum { TEXT_EDIT_OK, TEXT_EDIT_FULL, TEXT_EDIT_INVALID } TextEditResult;
typedef enum {
    TEXT_EDIT_LEFT, TEXT_EDIT_RIGHT, TEXT_EDIT_HOME, TEXT_EDIT_END,
    TEXT_EDIT_BACKSPACE, TEXT_EDIT_DELETE
} TextEditOperation;

void text_edit_begin(TextEdit *edit);
void text_edit_clear(TextEdit *edit);
void text_edit_end(TextEdit *edit); /* Cancel/end clears text and releases state. */
/* Atomic insertion of valid UTF-8; rejects controls and oversize input. */
TextEditResult text_edit_insert(TextEdit *edit, const char *text);
void text_edit_apply(TextEdit *edit, TextEditOperation operation);

#endif
