#include "noteStorage.h"
#include "../main.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Escape parser: reads one line formatted as  title,body
// Backslash escapes: \\ -> \   \, -> ,
// Returns true if exactly 2 fields were found.
// ---------------------------------------------------------------------------
static bool parse_escaped_line(const char* line, char* field1, size_t f1_max,
                                                  char* field2, size_t f2_max) {
    char* fields[2]       = { field1, field2 };
    size_t limits[2]      = { f1_max, f2_max };
    int    current_field  = 0;
    size_t field_letter   = 0;
    bool   escape         = false;

    for(int i = 0; line[i] != '\0'; ++i) {
        char c = line[i];

        if(escape) {
            if(field_letter + 1 < limits[current_field]) {
                fields[current_field][field_letter++] = c;
            }
            escape = false;
        } else if(c == '\\') {
            escape = true;
        } else if(c == ',' && current_field < 1) {
            fields[current_field][field_letter] = '\0';
            current_field++;
            field_letter = 0;
        } else {
            if(field_letter + 1 < limits[current_field]) {
                fields[current_field][field_letter++] = c;
            }
        }
    }

    if(escape) return false;            // dangling backslash
    if(current_field != 1) return false; // need exactly 2 fields

    fields[current_field][field_letter] = '\0';
    return true;
}

// ---------------------------------------------------------------------------
// Read one byte at a time and accumulate a line (strips \r).
// Returns number of bytes placed in buffer, 0 on EOF.
// ---------------------------------------------------------------------------
static ssize_t storage_file_read_line(File* file, char* buffer, size_t max_len) {
    if(!file || !buffer || max_len == 0) return -1;

    size_t  count = 0;
    uint8_t ch;

    while(count < max_len - 1) {
        ssize_t rd = storage_file_read(file, &ch, 1);
        if(rd != 1) break;
        if(ch == '\n') break;
        if(ch != '\r') buffer[count++] = (char)ch;
    }

    if(count == 0) return 0;
    buffer[count] = '\0';
    return (ssize_t)count;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

size_t read_notes_from_file(const char* filename, Note* notes) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(!storage) return 0;

    File* file = storage_file_alloc(storage);
    if(!storage_file_open(file, filename, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FURI_LOG_E("Notepad", "Failed to open file: %s", filename);
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return 0;
    }

    // Line buffer must be large enough for escaped title + comma + escaped body
    char line[MAX_TITLE * 2 + MAX_BODY * 2 + 4];
    char title[MAX_TITLE];
    char body[MAX_BODY];

    size_t i = 0;
    while(i < MAX_NOTES) {
        memset(line, 0, sizeof(line));
        if(storage_file_read_line(file, line, sizeof(line)) <= 0) break;

        if(parse_escaped_line(line, title, MAX_TITLE, body, MAX_BODY)) {
            strncpy(notes[i].title, title, MAX_TITLE - 1);
            notes[i].title[MAX_TITLE - 1] = '\0';
            strncpy(notes[i].body, body, MAX_BODY - 1);
            notes[i].body[MAX_BODY - 1] = '\0';
            i++;
        }
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return i;
}

// Escape special characters before writing
static size_t escape_field(const char* src, char* dst, size_t dst_max) {
    size_t j = 0;
    for(size_t i = 0; src[i] != '\0' && j + 2 < dst_max; i++) {
        if(src[i] == '\\' || src[i] == ',') {
            dst[j++] = '\\';
        }
        dst[j++] = src[i];
    }
    dst[j] = '\0';
    return j;
}

bool write_note_to_file(const char* filename, const char* title, const char* body) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(!storage) return false;

    File* file = storage_file_alloc(storage);
    if(!storage_file_open(file, filename, FSAM_WRITE, FSOM_OPEN_APPEND)) {
        if(!storage_file_open(file, filename, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
            FURI_LOG_E("Notepad", "Failed to create file: %s", filename);
            storage_file_free(file);
            furi_record_close(RECORD_STORAGE);
            return false;
        }
    }

    char esc_title[MAX_TITLE * 2];
    char esc_body[MAX_BODY * 2];
    escape_field(title, esc_title, sizeof(esc_title));
    escape_field(body,  esc_body,  sizeof(esc_body));

    // Format:  title,body\n
    char line[sizeof(esc_title) + sizeof(esc_body) + 4];
    int len = snprintf(line, sizeof(line), "%s,%s\n", esc_title, esc_body);

    bool success = (len > 0) &&
                   (storage_file_write(file, line, (size_t)len) == (size_t)len);

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return success;
}

bool delete_note_from_file(const char* path, size_t line_to_delete) {
    Storage* storage = furi_record_open(RECORD_STORAGE);

    File* source = storage_file_alloc(storage);
    if(!storage_file_open(source, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FURI_LOG_E("Notepad", "Failed to open source file");
        storage_file_free(source);
        furi_record_close(RECORD_STORAGE);
        return false;
    }

    const char* tmp_path = "/ext/notepad_tmp.txt";
    File* tmp = storage_file_alloc(storage);
    if(!storage_file_open(tmp, tmp_path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        FURI_LOG_E("Notepad", "Failed to open temporary file");
        storage_file_close(source);
        storage_file_free(source);
        storage_file_free(tmp);
        furi_record_close(RECORD_STORAGE);
        return false;
    }

    char line[MAX_TITLE * 2 + MAX_BODY * 2 + 4];
    size_t current_line = 0;

    while(true) {
        ssize_t len = storage_file_read_line(source, line, sizeof(line));
        if(len <= 0) break;

        if(current_line != line_to_delete) {
            storage_file_write(tmp, line, (size_t)len);
            storage_file_write(tmp, "\n", 1);
        }
        current_line++;
    }

    storage_file_close(source);
    storage_file_close(tmp);

    storage_simply_remove(storage, path);
    storage_common_rename(storage, tmp_path, path);

    storage_file_free(source);
    storage_file_free(tmp);
    furi_record_close(RECORD_STORAGE);
    return true;
}
