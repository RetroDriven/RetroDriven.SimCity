#include "name_edit.h"
#include <string.h>

void sim_name_edit_init(sim_name_edit *edit, const char *name)
{
    unsigned i = 0;
    if (!edit) return;
    memset(edit, 0, sizeof(*edit));
    if (!name) return;
    while (i < 17 && name[i]) { edit->text[i] = edit->original[i] = name[i]; ++i; }
    edit->length = (int)i;
}

int sim_name_edit_allowed(unsigned key)
{
    return key >= 32 && key <= 127 && strchr(":;,.=+-_\\/*", (int)key) == NULL;
}

int sim_name_edit_key(sim_name_edit *edit, unsigned key)
{
    int remove = 0;
    if (!edit) return 0;
    if (edit->done) return 1;
    switch (key) {
    case 2: case 0x847: edit->cursor = 0; break;
    case 5: case 0x84f: edit->cursor = edit->length; break;
    case 0x84b:
        if (edit->cursor) --edit->cursor;
        break;
    case 8:
        /* The original beeps at position0 but still deletes its first byte. */
        if (edit->cursor) --edit->cursor;
        remove = 1;
        break;
    case 4: case 0x853: remove = 1; break;
    case 9: case 0x852:
        if (edit->length < 16 && edit->cursor != edit->length) {
            memmove(edit->text+edit->cursor+1, edit->text+edit->cursor,
                    (size_t)(edit->length-edit->cursor+1));
            edit->text[edit->cursor] = ' ';
            ++edit->length;
        }
        break;
    case 11: case 0x893:
        if (!edit->length) { edit->done = edit->cancelled = 1; break; }
        edit->length = edit->cursor;
        edit->text[edit->length] = 0;
        break;
    case 10: case 13: edit->done = 1; break;
    case 21: case 0x84d:
        if (edit->cursor < edit->length) ++edit->cursor;
        break;
    case 27:
        if (!edit->length) edit->done = edit->cancelled = 1;
        else edit->cursor = edit->length = 0;
        edit->text[0] = 0;
        break;
    default:
        if (edit->cursor < 17 && sim_name_edit_allowed(key)) {
            edit->text[edit->cursor++] = (char)key;
            if (edit->cursor > edit->length) edit->length = edit->cursor;
        }
        break;
    }
    if (remove && edit->length && edit->cursor != edit->length) {
        memmove(edit->text+edit->cursor, edit->text+edit->cursor+1,
                (size_t)(edit->length-edit->cursor));
        --edit->length;
    }
    edit->text[edit->length] = 0;
    return edit->done;
}

void sim_name_edit_result(const sim_name_edit *edit, char out[19])
{
    const char *value;
    if (!edit || !out) return;
    value = edit->text;
    while (*value == ' ') ++value;
    if (edit->cancelled || !*value) value = edit->original;
    strcpy(out, value);
}
