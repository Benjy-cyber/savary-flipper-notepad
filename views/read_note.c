#include <furi.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <gui/view.h>
#include <input/input.h>
#include <stdlib.h>
#include <string.h>

#include "read_note.h"

// Flipper canvas: 128 x 64 px
// FontSecondary char width ~6 px, so ~20 chars per line
// Usable lines below header: 4 lines at 12px each (y = 24,36,48,60)

#define CHARS_PER_LINE 20
#define VISIBLE_LINES  4

// Word-wrap: fill lines[] with pointers into a static line-break buffer.
// Returns total number of wrapped lines.
static size_t wrap_body(const char* body, char lines[][CHARS_PER_LINE + 1],
                         size_t max_lines) {
    size_t line_count = 0;
    size_t body_len   = strlen(body);
    size_t pos        = 0;

    while(pos < body_len && line_count < max_lines) {
        size_t remaining = body_len - pos;
        size_t take      = (remaining > CHARS_PER_LINE) ? CHARS_PER_LINE : remaining;

        // Try not to break in the middle of a word — walk back to last space
        if(remaining > CHARS_PER_LINE) {
            size_t cut = take;
            while(cut > 0 && body[pos + cut] != ' ' && body[pos + cut] != '\0') cut--;
            if(cut > 0) take = cut;
        }

        strncpy(lines[line_count], body + pos, take);
        lines[line_count][take] = '\0';
        pos += take;

        // Skip leading space on next line
        if(pos < body_len && body[pos] == ' ') pos++;

        line_count++;
    }
    return line_count;
}

static void read_note_draw_callback(Canvas* canvas, void* model) {
    AppContext** model_ = model;
    AppContext*  app    = *model_;

    canvas_clear(canvas);

    // Header: note title
    canvas_set_font(canvas, FontPrimary);
    canvas_set_color(canvas, ColorBlack);

    // Truncate title display to 21 chars
    char title_display[22];
    strncpy(title_display, app->notes[app->selected].title, 21);
    title_display[21] = '\0';
    canvas_draw_str(canvas, 0, 10, title_display);
    canvas_draw_line(canvas, 0, 12, 128, 12);

    // Wrap body
    char lines[64][CHARS_PER_LINE + 1];
    size_t total_lines = wrap_body(app->notes[app->selected].body, lines, 64);

    canvas_set_font(canvas, FontSecondary);

    size_t start = app->read_scroll;
    size_t end   = start + VISIBLE_LINES;
    if(end > total_lines) end = total_lines;

    for(size_t i = start; i < end; i++) {
        int y = 24 + (int)(i - start) * 12;
        canvas_draw_str(canvas, 0, y, lines[i]);
    }

    // Scroll indicator (right edge)
    if(total_lines > VISIBLE_LINES) {
        int bar_x            = 124;
        int bar_y            = 14;
        int bar_height       = 48;
        int scroll_range     = (int)(total_lines - VISIBLE_LINES);
        int indicator_height = bar_height * VISIBLE_LINES / (int)total_lines;
        if(indicator_height < 6) indicator_height = 6;
        int indicator_y = bar_y +
            (scroll_range > 0
                ? (bar_height - indicator_height) * (int)app->read_scroll / scroll_range
                : 0);
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_box(canvas, bar_x, indicator_y, 3, indicator_height);
    }

    // "Back" hint at bottom if no scrollbar needed
    if(total_lines == 0) {
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_str(canvas, 5, 36, "(empty note)");
    }
}

static bool read_note_input_callback(InputEvent* event, void* context) {
    AppContext* app = context;

    // Recompute total lines so we know scroll bounds
    char   lines[64][CHARS_PER_LINE + 1];
    size_t total_lines = wrap_body(app->notes[app->selected].body, lines, 64);

    if(event->type == InputTypeShort || event->type == InputTypeRepeat) {
        if(event->key == InputKeyUp) {
            if(app->read_scroll > 0) app->read_scroll--;
            return true;
        } else if(event->key == InputKeyDown) {
            if(total_lines > VISIBLE_LINES &&
               app->read_scroll < total_lines - VISIBLE_LINES) {
                app->read_scroll++;
            }
            return true;
        } else if(event->key == InputKeyBack) {
            app->read_scroll = 0;
            view_dispatcher_switch_to_view(app->view_dispatcher, ViewNotesList);
            return true;
        }
    }

    return false;
}

View* read_note_view_alloc(AppContext* app_context) {
    View* view = view_alloc();
    view_set_context(view, app_context);
    view_allocate_model(view, ViewModelTypeLockFree, sizeof(AppContext*));
    AppContext** app_view = view_get_model(view);
    *app_view = app_context;
    view_set_draw_callback(view, read_note_draw_callback);
    view_set_input_callback(view, read_note_input_callback);
    return view;
}
