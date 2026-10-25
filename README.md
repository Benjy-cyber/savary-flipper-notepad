# Notepad

A simple notepad app for the Flipper Zero. Store and read short notes directly on your device.

## Features

- **Add notes** — enter a title and a body using the on-screen keyboard
- **View notes** — browse your saved notes and read the full body with scrolling
- **Delete notes** — remove any note with a confirmation prompt

## How it works

### Add a note
1. Select **Add note** from the main menu
2. Enter a title (e.g. "Shopping list")
3. Enter the note body
4. The note is saved automatically

### View notes
1. Select **View notes**
2. Scroll with **Up / Down** to browse titles
3. Press **OK** to read the full note
4. Inside the note, use **Up / Down** to scroll through long bodies
5. Press **Back** to return to the list

### Delete a note
1. Select **Delete note**
2. Scroll to the note you want to remove
3. Press **OK** — a confirmation dialog appears
4. Press **OK** again to confirm, or **Back** to cancel

## Storage

Notes are stored as `title,body` (one note per line) in `/ext/notepad.txt`.  
Backslash (`\`) and comma (`,`) are escaped as `\\` and `\,` in the file.

## File layout

```
notepad/
├── application.fam
├── main.c
├── main.h
├── noteStorage/
│   ├── noteStorage.c
│   └── noteStorage.h
├── textInput/
│   ├── textInput.c
│   └── textInput.h
└── views/
    ├── notes_list.c
    ├── notes_list.h
    ├── read_note.c
    ├── read_note.h
    ├── delete_note.c
    └── delete_note.h
```

## Building

```bash
ufbt
```
