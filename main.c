#include "main.h"
#include "views/notes_list.h"
#include "views/read_note.h"
#include "views/delete_note.h"
#include "textInput/textInput.h"
#include "noteStorage/noteStorage.h"

// ---------------------------------------------------------------------------
// Main menu render
// ---------------------------------------------------------------------------
static void render_main_menu(Canvas* canvas, void* model) {
    if(!model) {
        FURI_LOG_E("Notepad", "render_main_menu: model is NULL!");
        return;
    }

    AppContext** model_ = model;
    AppContext*  app    = *model_;

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    canvas_set_color(canvas, ColorBlack);
    canvas_draw_str(canvas, 35, 10, "Notepad");
    canvas_draw_line(canvas, 0, 12, 128, 12);

    for(size_t i = 0; i < MENU_ITEMS; i++) {
        int y = 25 + (int)i * 12;
        if(i == app->selected) {
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_box(canvas, 0, y - 10, 128, 12);
            canvas_set_color(canvas, ColorWhite);
        } else {
            canvas_set_color(canvas, ColorBlack);
        }
        canvas_draw_str(canvas, 5, y, app->items[i]);
    }
}

// ---------------------------------------------------------------------------
// Main menu input
// ---------------------------------------------------------------------------
static bool handle_main_menu_input(InputEvent* event, void* context) {
    AppContext* app = context;

    if(!context) {
        FURI_LOG_E("Notepad", "handle_main_menu_input: context is NULL!");
        return false;
    }

    if(event->type == InputTypeShort) {
        if(event->key == InputKeyUp) {
            if(app->selected > 0) app->selected--;
            return true;

        } else if(event->key == InputKeyDown) {
            if(app->selected + 1 < MENU_ITEMS) app->selected++;
            return true;

        } else if(event->key == InputKeyBack) {
            app->running = false;
            view_dispatcher_stop(app->view_dispatcher);
            return true;

        } else if(event->key == InputKeyOk) {

            if(app->selected == 0) {
                // View notes
                app->notes_number = read_notes_from_file("/ext/notepad.txt", app->notes);
                app->selected      = 0;
                app->scroll_offset = 0;
                view_dispatcher_switch_to_view(app->view_dispatcher, ViewNotesList);
                return true;

            } else if(app->selected == 1) {
                // Add note
                app->tmp_title[0] = '\0';
                app->tmp_body[0]  = '\0';
                app->selected      = 0;
                app->scroll_offset = 0;
                view_dispatcher_switch_to_view(app->view_dispatcher, ViewTextInputTitle);
                return true;

            } else if(app->selected == 2) {
                // Delete note
                app->notes_number = read_notes_from_file("/ext/notepad.txt", app->notes);
                app->selected      = 0;
                app->scroll_offset = 0;
                view_dispatcher_switch_to_view(app->view_dispatcher, ViewDeleteNote);
                return true;
            }
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// Navigation event callback (Back from any view → main menu)
// ---------------------------------------------------------------------------
static bool view_dispatcher_navigation_event_callback(void* context) {
    AppContext* app = context;
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewMainMenu);
    return true;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int32_t notepad_app(void* p) {
    UNUSED(p);

    AppContext* app = malloc(sizeof(AppContext));
    if(!app) return -1;

    memset(app, 0, sizeof(AppContext));

    app->selected      = 0;
    app->scroll_offset = 0;
    app->running       = true;
    app->confirm_delete = false;
    app->read_scroll   = 0;

    app->items[0] = "View notes";
    app->items[1] = "Add note";
    app->items[2] = "Delete note";

    app->notes_number = read_notes_from_file("/ext/notepad.txt", app->notes);

    // GUI
    app->gui = furi_record_open(RECORD_GUI);
    if(!app->gui) { free(app); return -2; }

    app->view_dispatcher = view_dispatcher_alloc();
    if(!app->view_dispatcher) {
        furi_record_close(RECORD_GUI);
        free(app);
        return -3;
    }

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, view_dispatcher_navigation_event_callback);
    view_dispatcher_attach_to_gui(
        app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // ---- Main menu view ----
    app->main_menu_view = view_alloc();
    if(!app->main_menu_view) {
        view_dispatcher_free(app->view_dispatcher);
        furi_record_close(RECORD_GUI);
        free(app);
        return -4;
    }
    view_set_context(app->main_menu_view, app);
    view_allocate_model(app->main_menu_view, ViewModelTypeLockFree, sizeof(AppContext*));
    AppContext** app_view = view_get_model(app->main_menu_view);
    *app_view = app;
    view_set_draw_callback(app->main_menu_view, render_main_menu);
    view_set_input_callback(app->main_menu_view, handle_main_menu_input);
    view_dispatcher_add_view(app->view_dispatcher, ViewMainMenu, app->main_menu_view);

    // ---- Notes list view ----
    app->notes_list_view = notes_list_view_alloc(app);
    view_dispatcher_add_view(app->view_dispatcher, ViewNotesList, app->notes_list_view);

    // ---- Read note view ----
    app->read_note_view = read_note_view_alloc(app);
    view_dispatcher_add_view(app->view_dispatcher, ViewReadNote, app->read_note_view);

    // ---- Delete note view ----
    app->delete_note_view = delete_note_view_alloc(app);
    view_dispatcher_add_view(app->view_dispatcher, ViewDeleteNote, app->delete_note_view);

    // ---- Text input: title ----
    app->textInput_title = note_title_TextInput_alloc(app);
    view_dispatcher_add_view(
        app->view_dispatcher, ViewTextInputTitle,
        text_input_get_view(app->textInput_title));

    // ---- Text input: body ----
    app->textInput_body = note_body_TextInput_alloc(app);
    view_dispatcher_add_view(
        app->view_dispatcher, ViewTextInputBody,
        text_input_get_view(app->textInput_body));

    // Start
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewMainMenu);
    view_dispatcher_run(app->view_dispatcher);

    // ---- Cleanup ----
    view_dispatcher_remove_view(app->view_dispatcher, ViewMainMenu);
    view_free(app->main_menu_view);

    view_dispatcher_remove_view(app->view_dispatcher, ViewNotesList);
    view_free(app->notes_list_view);

    view_dispatcher_remove_view(app->view_dispatcher, ViewReadNote);
    view_free(app->read_note_view);

    view_dispatcher_remove_view(app->view_dispatcher, ViewDeleteNote);
    view_free(app->delete_note_view);

    view_dispatcher_remove_view(app->view_dispatcher, ViewTextInputTitle);
    view_free(text_input_get_view(app->textInput_title));

    view_dispatcher_remove_view(app->view_dispatcher, ViewTextInputBody);
    view_free(text_input_get_view(app->textInput_body));

    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);
    free(app);

    return 0;
}
