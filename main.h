#pragma once
#include <gui/gui.h>
#include <gui/view_port.h>
#include <furi.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <gui/view_port.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/text_input.h>
#include <gui/modules/dialog_ex.h>
#include <input/input.h>
#include <stdlib.h>
#include <string.h>

#define MENU_ITEMS 3
#define MAX_NOTES  100
#define MAX_TITLE  100
#define MAX_BODY   200

// Define view IDs
typedef enum {
    ViewMainMenu,
    ViewNotesList,
    ViewReadNote,
    ViewDeleteNote,
    ViewTextInputTitle,
    ViewTextInputBody
} ViewID;

typedef struct {
    char title[MAX_TITLE];
    char body[MAX_BODY];
} Note;

typedef struct {
    const char* items[MENU_ITEMS];
    size_t selected;
    size_t scroll_offset;
    bool running;

    // For view management
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    View* main_menu_view;
    View* notes_list_view;
    View* read_note_view;
    View* delete_note_view;

    Note notes[MAX_NOTES];
    size_t notes_number;

    char tmp_title[MAX_TITLE];
    char tmp_body[MAX_BODY];
    TextInput* textInput_title;
    TextInput* textInput_body;

    bool confirm_delete;

    // Read-note scroll state
    size_t read_scroll; // line offset when reading a note body

} AppContext;
