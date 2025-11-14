#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* ownership annotations */
#define OWNED
#define BORROWED
#define OUT

/* global error alias */
typedef int error;

/* package error codes */
typedef enum text_editor_error_code_e {
    TEXT_EDITOR_ERROR_OK = 0,
    TEXT_EDITOR_ERROR_OUT_OF_MEMORY,
    TEXT_EDITOR_ERROR_INVALID_ARGUMENT,
    TEXT_EDITOR_ERROR_INDEX_OUT_OF_RANGE,
    TEXT_EDITOR_ERROR_EVENT_QUEUE_FULL,
    TEXT_EDITOR_ERROR_INTERNAL
} text_editor_error_code_t;

/* modifiers bitmask */
typedef enum text_editor_modifier_e {
    TEXT_EDITOR_MODIFIER_NONE  = 0,
    TEXT_EDITOR_MODIFIER_SHIFT = 1 << 0,
    TEXT_EDITOR_MODIFIER_CTRL  = 1 << 1,
    TEXT_EDITOR_MODIFIER_ALT   = 1 << 2
} text_editor_modifier_t;

/* keys we care abas commands */
typedef enum text_editor_key_code_e {
    TEXT_EDITOR_KEY_UNKNOWN = 0,
    TEXT_EDITOR_KEY_CHARACTER, /* use input_utf8 for actual text */

    TEXT_EDITOR_KEY_LEFT,
    TEXT_EDITOR_KEY_RIGHT,
    TEXT_EDITOR_KEY_UP,
    TEXT_EDITOR_KEY_DOWN,
    TEXT_EDITOR_KEY_HOME,
    TEXT_EDITOR_KEY_END,

    TEXT_EDITOR_KEY_DELETE,
    TEXT_EDITOR_KEY_BACKSPACE,
    TEXT_EDITOR_KEY_TAB,
    TEXT_EDITOR_KEY_ENTER,

    /* chorded keys: ctrl+F, ctrl+C etc. are represented
       by modifiers+these base codes. For copy/cut/paste we use:
       CHARACTER with 'c','x','v' + CTRL. */
    TEXT_EDITOR_KEY_F /* used with CTRL for ctrl+F */
} text_editor_key_code_t;

/* positions are (line, column) in character cells, 0-based, document space */
typedef struct text_editor_position_s {
    size_t line_index;
    size_t column_index;
} text_editor_position_t;

/* selection is [start,end), normalized so start <= end */
typedef struct text_editor_selection_s {
    text_editor_position_t start;
    text_editor_position_t end;
} text_editor_selection_t;

/* mouse */
typedef enum text_editor_mouse_button_e {
    TEXT_EDITOR_MOUSE_BUTTON_NONE = 0,
    TEXT_EDITOR_MOUSE_BUTTON_LEFT,
    TEXT_EDITOR_MOUSE_BUTTON_MIDDLE,
    TEXT_EDITOR_MOUSE_BUTTON_RIGHT
} text_editor_mouse_button_t;

typedef enum text_editor_mouse_event_type_e {
    TEXT_EDITOR_MOUSE_EVENT_MOVE = 0,
    TEXT_EDITOR_MOUSE_EVENT_BUTTON_DOWN,
    TEXT_EDITOR_MOUSE_EVENT_BUTTON_UP
} text_editor_mouse_event_type_t;

/* key event */
typedef struct text_editor_key_event_s {
    text_editor_key_code_t key_code;
    unsigned int modifiers; /* bitwise OR of text_editor_modifier_t */
    char input_utf8[8];     /* valid if key_code == CHARACTER */
    size_t input_utf8_length;
} text_editor_key_event_t;

/* mouse event: coordinates in character units
   position is always in absolute document coordinates (line, column). */
typedef struct text_editor_mouse_event_s {
    text_editor_mouse_event_type_t type;
    text_editor_mouse_button_t button;
    unsigned int modifiers; /* bitwise OR of text_editor_modifier_t */
    text_editor_position_t position; /* document coordinates */
    uint64_t timestamp_ms;           /* caller-provided monotonic ms */
} text_editor_mouse_event_t;

/* viewport definition (character-space), fully external */
typedef struct text_editor_viewport_s {
    size_t first_line_index; /* top line index in buffer */
    size_t first_column;     /* leftmost visible column */
    size_t width;            /* number of visible columns */
    size_t height;           /* number of visible lines */
} text_editor_viewport_t;

/* view line: a visible slice of a buffer line */
typedef struct text_editor_view_line_s {
    size_t buffer_line_index;  /* 0-based in buffer */
    size_t line_number;        /* 1-based for UI */
    const char *text; /* pointer into internal storage */
    size_t byte_length;        /* substring length in bytes */
} text_editor_view_line_t;

/* view cursor: position + char under cursor if any */
typedef struct text_editor_view_cursor_s {
    text_editor_position_t position;
    bool has_character;
    char character; /* raw byte; for UTF-8 this is first byte */
} text_editor_view_cursor_t;

/* view selection: clipped to viewport, includes text snapshot (simple impl) */
typedef struct text_editor_view_selection_s {
    text_editor_selection_t range;
    const char *text; /* pointer into internal storage */
    size_t byte_length;
} text_editor_view_selection_t;

/* scrollbar info normalized to [0,1] for renderer;
   these are purely derived from document + viewport. */
typedef struct text_editor_scrollbar_s {
    float vertical_thumb_position; /* 0 = top, 1 = bottom */
    float vertical_thumb_size;     /* 0 = tiny, 1 = full */
} text_editor_scrollbar_t;

/* complete view snapshot */
typedef struct text_editor_view_s {
    const text_editor_view_line_t *lines;
    size_t line_count;

    const text_editor_view_cursor_t *cursors;
    size_t cursor_count;

    const text_editor_view_selection_t *selections;
    size_t selection_count;

    text_editor_scrollbar_t scrollbar;
} text_editor_view_t;

/* search options */
typedef struct text_editor_find_options_s {
    const char *pattern;
    size_t pattern_length;
    bool case_sensitive;
    bool whole_word;
    bool is_regular_expression; /* TODO: currently ignored in implementation */
} text_editor_find_options_t;

/* events emitted by editor core */
typedef enum text_editor_event_type_e {
    TEXT_EDITOR_EVENT_NONE = 0,
    TEXT_EDITOR_EVENT_INTELLISENSE_LOOKUP, /* ctrl+click */
    TEXT_EDITOR_EVENT_HOVER_WORD,          /* hover delay elapses */
    TEXT_EDITOR_EVENT_FIND_INTENT,         /* ctrl+F */
    TEXT_EDITOR_EVENT_FIND_RESULTS_UPDATED,/* after find_all selections */
    TEXT_EDITOR_EVENT_SCROLL_INTENT        /* editor requests scroll */
} text_editor_event_type_t;

/* scroll command from host into editor:
   the editor does not modify any internal viewport; it emits a SCROLL_INTENT
   event so that other systems can react if needed. */
typedef enum text_editor_scroll_mode_e {
    TEXT_EDITOR_SCROLL_MODE_RELATIVE = 0,
    TEXT_EDITOR_SCROLL_MODE_ABSOLUTE
} text_editor_scroll_mode_t;

typedef struct text_editor_scroll_command_s {
    text_editor_scroll_mode_t mode;
    int delta_lines;
    int delta_columns;
    size_t absolute_first_line;
    size_t absolute_first_column;
} text_editor_scroll_command_t;

typedef struct text_editor_event_s {
    text_editor_event_type_t type;
    union {
        struct {
            text_editor_position_t position;
        } intellisense_lookup;
        struct {
            text_editor_position_t position;
            /* word text is returned as a boddowed pointer into editor buffer */
            const char *word_text;
            size_t word_byte_length;
        } hover_word;
        struct {
            text_editor_scroll_mode_t mode;
            int delta_lines;
            int delta_columns;
            size_t absolute_first_line;
            size_t absolute_first_column;
        } scroll_intent;
    } data;
} text_editor_event_t;

/* main editor type */
typedef struct text_editor_s text_editor_t;

/* API */

/* allocation / lifecycle */
error        text_editor__alloc                  (text_editor_t **out_editor);
error        text_editor__init                   (text_editor_t *self);
error        text_editor__free                   (text_editor_t *self);

/* configuration (call before feeding input if you want non-defaults) */
error        text_editor__set_newline            (text_editor_t *self, const char *newline_sequence, size_t newline_length);

error        text_editor__set_tab_size           (text_editor_t *self, size_t tab_size);

error        text_editor__set_hover_delay_ms     (text_editor_t *self, uint64_t hover_delay_ms);

/* text content management */
error        text_editor__set_text_from_c_string (text_editor_t *self, const char *text);

/* TODO: implement flattening; currently returns INTERNAL error. */
error        text_editor__get_text_snapshot      (text_editor_t *self, const char **out_text, size_t *out_length);

/* input handling */
error        text_editor__handle_key             (text_editor_t *self, const text_editor_key_event_t *key_event);

error        text_editor__handle_mouse           (text_editor_t *self, const text_editor_mouse_event_t *mouse_event);

/* scroll command:
   does NOT change any internal viewport; only emits SCROLL_INTENT event. */
error        text_editor__scroll                 (text_editor_t *self, const text_editor_scroll_command_t *command);

/* find: selects all matches according to options */
error        text_editor__find_all               (text_editor_t *self, const text_editor_find_options_t *options);

/* view export: returns arrays valid until next mutating call.
   Host passes the viewport; editor fills out_view accordingly. */
error        text_editor__get_view               (text_editor_t *self, const text_editor_viewport_t *viewport, text_editor_view_t *out_view);

/* event polling: single-consumer queue */
error        text_editor__next_event             (text_editor_t *self, text_editor_event_t *out_event);

/* error to message */
const char * text_editor__error_message          (error err);

#endif /* TEXT_EDITOR_H */
