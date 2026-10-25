#include <furi.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <gui/view.h>
#include <input/input.h>
#include <stdlib.h>
#include <string.h>

#include "notes_list.h"

#define MAX_VISIBLE 4

static void notes_list_draw_callback(Canvas* canvas, void* model) {
    AppContext** model_ = model;
    AppContext*  app    = *model_;

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    canvas_set_color(canvas, ColorBlack);
    canvas_draw_str(canvas, 30, 10, "My Notes");
    canvas_draw_line(canvas, 0, 12, 128, 12);

    if(app->notes_number == 0) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 10, 36, "No notes yet. Add one!");
        return;
    }

    size_t start = app->scroll_offset;
    size_t end   = start + MAX_VISIBLE;
    if(end > app->notes_number) end = app->notes_number;

    for(size_t i = start; i < end; i++) {
        int y = 25 + (int)(i - start) * 12;
        if(i == app->selected) {
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_box(canvas, 0, y - 10, 120, 12);
            canvas_set_color(canvas, ColorWhite);
        } else {
            canvas_set_color(canvas, ColorBlack);
        }
        canvas_draw_str(canvas, 5, y, app->notes[i].title);
    }

    // Scroll bar
    if(app->notes_number > MAX_VISIBLE) {
        int bar_x            = 124;
        int bar_y            = 14;
        int bar_height       = 48;
        int scroll_range     = (int)(app->notes_number - MAX_VISIBLE);
        int indicator_height = bar_height * MAX_VISIBLE / (int)app->notes_number;
        if(indicator_height < 6) indicator_height = 6;
        int indicator_y = bar_y +
            (bar_height - indicator_height) * (int)app->scroll_offset / scroll_range;

        canvas_set_color(canvas, ColorBlack);
        canvas_draw_box(canvas, bar_x, indicator_y, 3, indicator_height);
    }
}

static bool notes_list_input_callback(InputEvent* event, void* context) {
    AppContext* app = context;

    if(event->type == InputTypeShort) {
        if(event->key == InputKeyUp) {
            if(app->selected > 0) app->selected--;
            if(app->selected < app->scroll_offset) app->scroll_offset--;
            return true;

        } else if(event->key == InputKeyDown) {
            if(app->selected + 1 < app->notes_number) app->selected++;
            if(app->selected >= app->scroll_offset + MAX_VISIBLE) app->scroll_offset++;
            return true;

        } else if(event->key == InputKeyBack) {
            app->selected     = 0;
            app->scroll_offset = 0;
            return false; // let navigation callback take over → main menu

        } else if(event->key == InputKeyOk) {
            if(app->notes_number > 0) {
                app->read_scroll = 0;
                view_dispatcher_switch_to_view(app->view_dispatcher, ViewReadNote);
            }
            return true;
        }
    }

    return false;
}

View* notes_list_view_alloc(AppContext* app_context) {
    View* view = view_alloc();
    view_set_context(view, app_context);
    view_allocate_model(view, ViewModelTypeLockFree, sizeof(AppContext*));
    AppContext** app_view = view_get_model(view);
    *app_view = app_context;
    view_set_draw_callback(view, notes_list_draw_callback);
    view_set_input_callback(view, notes_list_input_callback);
    return view;
}
