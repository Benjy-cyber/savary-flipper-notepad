#include "textInput.h"
#include "../main.h"
#include "../noteStorage/noteStorage.h"
#include <string.h>

// Called when user finishes entering the title
static void title_callback(void* context) {
    AppContext* app = context;
    // title is already in app->tmp_title (set as result buffer)
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewTextInputBody);
}

// Called when user finishes entering the body
static void body_callback(void* context) {
    AppContext* app = context;
    // body is already in app->tmp_body

    // Persist to file
    write_note_to_file("/ext/notepad.txt", app->tmp_title, app->tmp_body);

    // Clear temps
    app->tmp_title[0] = '\0';
    app->tmp_body[0]  = '\0';

    view_dispatcher_switch_to_view(app->view_dispatcher, ViewMainMenu);
}

TextInput* note_title_TextInput_alloc(void* context) {
    AppContext* app = context;

    TextInput* ti = text_input_alloc();
    text_input_set_header_text(ti, "Note Title:");
    text_input_set_result_callback(
        ti,
        title_callback,
        app,
        app->tmp_title,
        MAX_TITLE - 1,
        true
    );
    return ti;
}

TextInput* note_body_TextInput_alloc(void* context) {
    AppContext* app = context;

    TextInput* ti = text_input_alloc();
    text_input_set_minimum_length(ti, 0);
    text_input_set_header_text(ti, "Note Body:");
    text_input_set_result_callback(
        ti,
        body_callback,
        app,
        app->tmp_body,
        MAX_BODY - 1,
        true
    );
    return ti;
}
