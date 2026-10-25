#pragma once
#include <furi.h>
#include <storage/storage.h>
#include <string.h>
#include <stdlib.h>
#include "../main.h"

size_t read_notes_from_file(const char* filename, Note* notes);
bool write_note_to_file(const char* filename, const char* title, const char* body);
bool delete_note_from_file(const char* path, size_t line_to_delete);
