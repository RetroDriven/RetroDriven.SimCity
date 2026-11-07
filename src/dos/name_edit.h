#ifndef SIMCITY_DOS_NAME_EDIT_H
#define SIMCITY_DOS_NAME_EDIT_H

/* DOS new-city name editor 0x29e36, called with field length18, mode1.
 * At most17 typed bytes. Rendering/blink timing and keyboard transport belong
 * to the adapter. DOS control codes are accepted, not physical HID usages. */
typedef struct sim_name_edit {
    char text[19];
    char original[19];
    int cursor, length, done, cancelled;
} sim_name_edit;

void sim_name_edit_init(sim_name_edit *edit, const char *name);
int sim_name_edit_allowed(unsigned ascii);
int sim_name_edit_key(sim_name_edit *edit, unsigned key); /* nonzero when done */
void sim_name_edit_result(const sim_name_edit *edit, char out[19]);
#endif
