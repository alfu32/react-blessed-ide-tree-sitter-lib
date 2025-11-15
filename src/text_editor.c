#include "text_editor.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ===== internal helpers ===== */

typedef struct text_line_s {
    char *data;
    size_t length;  /* bytes, excludes terminating '\0' */
    size_t capacity;
} text_line_t;

typedef struct text_line_array_s {
    text_line_t *items;
    size_t count;
    size_t capacity;
} text_line_array_t;

typedef struct text_cursor_s {
    text_editor_position_t position;
} text_cursor_t;

typedef struct text_cursor_array_s {
    text_cursor_t *items;
    size_t count;
    size_t capacity;
} text_cursor_array_t;

typedef struct text_selection_array_s {
    text_editor_selection_t *items;
    size_t count;
    size_t capacity;
} text_selection_array_t;

/* simple ring-buffer event queue */
#define TEXT_EDITOR_EVENT_QUEUE_CAPACITY 64

typedef struct text_editor_event_queue_s {
    text_editor_event_t items[TEXT_EDITOR_EVENT_QUEUE_CAPACITY];
    size_t head;
    size_t tail;
    bool is_full;
} text_editor_event_queue_t;

/* temporary view storage */
typedef struct text_editor_view_storage_s {
    text_editor_view_line_t *lines;
    size_t lines_capacity;

    text_editor_view_cursor_t *cursors;
    size_t cursors_capacity;

    text_editor_view_selection_t *selections;
    size_t selections_capacity;
} text_editor_view_storage_t;

/* main editor struct */
struct text_editor_s {
    text_line_array_t lines;

    text_cursor_array_t cursors;
    text_selection_array_t selections;

    /* simple clipboard (text of last copy/cut) */
    char *clipboard;
    size_t clipboard_length;

    /* configuration */
    char newline_sequence[2];
    size_t newline_length;
    size_t tab_size;
    uint64_t hover_delay_ms;

    /* hover tracking */
    text_editor_position_t last_mouse_position;
    uint64_t last_mouse_move_time_ms;
    bool hover_pending;

    /* event queue */
    text_editor_event_queue_t events;

    /* temp storage for view */
    text_editor_view_storage_t view_storage;
};

/* ===== utility functions ===== */

static error text_editor__ensure_lines_capacity(text_editor_t *self, size_t min_capacity);
static error text_editor__line_reserve(text_line_t *line, size_t min_capacity);
static error text_editor__ensure_cursor_capacity(text_editor_t *self, size_t min_capacity);
static error text_editor__ensure_selection_capacity(text_editor_t *self, size_t min_capacity);

static void text_editor__event_queue_init(text_editor_event_queue_t *queue) {
    queue->head = 0;
    queue->tail = 0;
    queue->is_full = false;
}

static bool text_editor__event_queue_is_empty(const text_editor_event_queue_t *queue) {
    return (!queue->is_full && (queue->head == queue->tail));
}

static bool text_editor__event_queue_is_full(const text_editor_event_queue_t *queue) {
    return queue->is_full;
}

static error text_editor__event_queue_push(text_editor_event_queue_t *queue,
                                           const text_editor_event_t *event) {
    if (text_editor__event_queue_is_full(queue)) {
        return TEXT_EDITOR_ERROR_EVENT_QUEUE_FULL;
    }
    queue->items[queue->tail] = *event;
    queue->tail = (queue->tail + 1) % TEXT_EDITOR_EVENT_QUEUE_CAPACITY;
    if (queue->tail == queue->head) {
        queue->is_full = true;
    }
    return TEXT_EDITOR_ERROR_OK;
}

static error text_editor__event_queue_pop(text_editor_event_queue_t *queue,
                                          text_editor_event_t *out_event) {
    if (text_editor__event_queue_is_empty(queue)) {
        out_event->type = TEXT_EDITOR_EVENT_NONE;
        return TEXT_EDITOR_ERROR_OK;
    }
    *out_event = queue->items[queue->head];
    queue->head = (queue->head + 1) % TEXT_EDITOR_EVENT_QUEUE_CAPACITY;
    queue->is_full = false;
    return TEXT_EDITOR_ERROR_OK;
}

/* word-character helper: letters, digits, underscore */
static bool text_editor__is_word_char(unsigned char c) {
    return (isalnum(c) || c == '_');
}

/* clamp helpers */
static size_t text_editor__min_size(size_t a, size_t b) {
    return a < b ? a : b;
}

static size_t text_editor__max_size(size_t a, size_t b) {
    return a > b ? a : b;
}

/* normalize selection so start <= end lexicographically */
static void text_editor__normalize_selection(text_editor_selection_t *selection) {
    text_editor_position_t a = selection->start;
    text_editor_position_t b = selection->end;
    bool swap = false;
    if (a.line_index > b.line_index) {
        swap = true;
    } else if (a.line_index == b.line_index && a.column_index > b.column_index) {
        swap = true;
    }
    if (swap) {
        selection->start = b;
        selection->end = a;
    }
}

/* ensure position is within document bounds (line, column) */
static void text_editor__clamp_position(text_editor_t *self, text_editor_position_t *pos) {
    if (self->lines.count == 0) {
        pos->line_index = 0;
        pos->column_index = 0;
        return;
    }
    if (pos->line_index >= self->lines.count) {
        pos->line_index = self->lines.count - 1;
    }
    text_line_t *line = &self->lines.items[pos->line_index];
    if (pos->column_index > line->length) {
        pos->column_index = line->length;
    }
}

/* convert (line, column) to byte offset in line->data
   NOTE: assumes 1 byte per column; treat UTF-8 as TODO. */
static size_t text_editor__column_to_byte_offset(const text_line_t *line, size_t column) {
    if (column > line->length) {
        column = line->length;
    }
    return column;
}

/* insert bytes into a line at column index */
static error text_editor__line_insert_bytes(text_line_t *line,
                                            size_t column,
                                            const char *bytes,
                                            size_t byte_count) {
    if (column > line->length) {
        column = line->length;
    }
    size_t offset = text_editor__column_to_byte_offset(line, column);
    size_t new_length = line->length + byte_count;
    error err = text_editor__line_reserve(line, new_length + 1);
    if (err != TEXT_EDITOR_ERROR_OK) {
        return err;
    }
    memmove(line->data + offset + byte_count,
            line->data + offset,
            line->length - offset);
    memcpy(line->data + offset, bytes, byte_count);
    line->length = new_length;
    line->data[line->length] = '\0';
    return TEXT_EDITOR_ERROR_OK;
}

/* delete [column, column+count) columns from line */
static error text_editor__line_delete_range(text_line_t *line,
                                            size_t column,
                                            size_t count) {
    if (column > line->length) {
        return TEXT_EDITOR_ERROR_OK;
    }
    size_t start = text_editor__column_to_byte_offset(line, column);
    size_t end_column = column + count;
    if (end_column > line->length) {
        end_column = line->length;
    }
    size_t end = text_editor__column_to_byte_offset(line, end_column);
    size_t tail = line->length - end;
    memmove(line->data + start, line->data + end, tail);
    line->length -= (end - start);
    line->data[line->length] = '\0';
    return TEXT_EDITOR_ERROR_OK;
}

/* split a line at column into two lines */
static error text_editor__split_line(text_editor_t *self,
                                     size_t line_index,
                                     size_t column) {
    if (line_index >= self->lines.count) {
        return TEXT_EDITOR_ERROR_INDEX_OUT_OF_RANGE;
    }
    text_line_t *line = &self->lines.items[line_index];
    size_t split_offset = text_editor__column_to_byte_offset(line, column);
    size_t tail_length = line->length - split_offset;

    error err = text_editor__ensure_lines_capacity(self, self->lines.count + 1);
    if (err != TEXT_EDITOR_ERROR_OK) {
        return err;
    }

    /* make room for new line */
    memmove(self->lines.items + line_index + 1,
            self->lines.items + line_index,
            (self->lines.count - line_index) * sizeof(text_line_t));

    text_line_t *new_line = &self->lines.items[line_index + 1];
    new_line->capacity = tail_length + 1;
    new_line->data = (char *)malloc(new_line->capacity);
    if (!new_line->data) {
        return TEXT_EDITOR_ERROR_OUT_OF_MEMORY;
    }
    memcpy(new_line->data, line->data + split_offset, tail_length);
    new_line->length = tail_length;
    new_line->data[new_line->length] = '\0';

    line->length = split_offset;
    line->data[line->length] = '\0';

    self->lines.count += 1;
    return TEXT_EDITOR_ERROR_OK;
}

/* join line line_index+1 into line_index */
static error text_editor__join_line_with_next(text_editor_t *self,
                                              size_t line_index) {
    if (line_index + 1 >= self->lines.count) {
        return TEXT_EDITOR_ERROR_OK;
    }
    text_line_t *line = &self->lines.items[line_index];
    text_line_t *next = &self->lines.items[line_index + 1];

    error err = text_editor__line_reserve(line, line->length + next->length + 1);
    if (err != TEXT_EDITOR_ERROR_OK) {
        return err;
    }
    memcpy(line->data + line->length, next->data, next->length);
    line->length += next->length;
    line->data[line->length] = '\0';

    /* remove next line */
    free(next->data);
    memmove(self->lines.items + line_index + 1,
            self->lines.items + line_index + 2,
            (self->lines.count - line_index - 2) * sizeof(text_line_t));
    self->lines.count -= 1;
    return TEXT_EDITOR_ERROR_OK;
}

/* ensure document has at least one line */
static error text_editor__ensure_nonempty(text_editor_t *self) {
    if (self->lines.count > 0) {
        return TEXT_EDITOR_ERROR_OK;
    }
    error err = text_editor__ensure_lines_capacity(self, 1);
    if (err != TEXT_EDITOR_ERROR_OK) {
        return err;
    }
    text_line_t *line = &self->lines.items[0];
    line->capacity = 1;
    line->data = (char *)malloc(1);
    if (!line->data) {
        return TEXT_EDITOR_ERROR_OUT_OF_MEMORY;
    }
    line->data[0] = '\0';
    line->length = 0;
    self->lines.count = 1;
    return TEXT_EDITOR_ERROR_OK;
}

/* initialize a line */
static void text_editor__line_init(text_line_t *line) {
    line->data = NULL;
    line->length = 0;
    line->capacity = 0;
}

/* free a line */
static void text_editor__line_free(text_line_t *line) {
    if (line->data) {
        free(line->data);
    }
    line->data = NULL;
    line->length = 0;
    line->capacity = 0;
}

/* capacity helpers */

static error text_editor__ensure_lines_capacity(text_editor_t *self, size_t min_capacity) {
    if (self->lines.capacity >= min_capacity) {
        return TEXT_EDITOR_ERROR_OK;
    }
    size_t new_capacity = self->lines.capacity ? self->lines.capacity * 2 : 8;
    if (new_capacity < min_capacity) {
        new_capacity = min_capacity;
    }
    text_line_t *new_items = (text_line_t *)malloc(new_capacity * sizeof(text_line_t));
    if (!new_items) {
        return TEXT_EDITOR_ERROR_OUT_OF_MEMORY;
    }
    for (size_t i = 0; i < self->lines.count; ++i) {
        new_items[i] = self->lines.items[i];
    }
    for (size_t i = self->lines.count; i < new_capacity; ++i) {
        text_editor__line_init(&new_items[i]);
    }
    free(self->lines.items);
    self->lines.items = new_items;
    self->lines.capacity = new_capacity;
    return TEXT_EDITOR_ERROR_OK;
}

static error text_editor__line_reserve(text_line_t *line, size_t min_capacity) {
    if (line->capacity >= min_capacity) {
        return TEXT_EDITOR_ERROR_OK;
    }
    size_t new_capacity = line->capacity ? line->capacity * 2 : 16;
    if (new_capacity < min_capacity) {
        new_capacity = min_capacity;
    }
    char *new_data = (char *)realloc(line->data, new_capacity);
    if (!new_data) {
        return TEXT_EDITOR_ERROR_OUT_OF_MEMORY;
    }
    line->data = new_data;
    line->capacity = new_capacity;
    return TEXT_EDITOR_ERROR_OK;
}

static error text_editor__ensure_cursor_capacity(text_editor_t *self, size_t min_capacity) {
    if (self->cursors.capacity >= min_capacity) {
        return TEXT_EDITOR_ERROR_OK;
    }
    size_t new_capacity = self->cursors.capacity ? self->cursors.capacity * 2 : 4;
    if (new_capacity < min_capacity) {
        new_capacity = min_capacity;
    }
    text_cursor_t *new_items = (text_cursor_t *)realloc(self->cursors.items,
                                                        new_capacity * sizeof(text_cursor_t));
    if (!new_items) {
        return TEXT_EDITOR_ERROR_OUT_OF_MEMORY;
    }
    self->cursors.items = new_items;
    self->cursors.capacity = new_capacity;
    return TEXT_EDITOR_ERROR_OK;
}

static error text_editor__ensure_selection_capacity(text_editor_t *self, size_t min_capacity) {
    if (self->selections.capacity >= min_capacity) {
        return TEXT_EDITOR_ERROR_OK;
    }
    size_t new_capacity = self->selections.capacity ? self->selections.capacity * 2 : 4;
    if (new_capacity < min_capacity) {
        new_capacity = min_capacity;
    }
    text_editor_selection_t *new_items =
        (text_editor_selection_t *)realloc(self->selections.items,
                                           new_capacity * sizeof(text_editor_selection_t));
    if (!new_items) {
        return TEXT_EDITOR_ERROR_OUT_OF_MEMORY;
    }
    self->selections.items = new_items;
    self->selections.capacity = new_capacity;
    return TEXT_EDITOR_ERROR_OK;
}

/* set internal clipboard contents to text[0..length) */
static error text_editor__set_clipboard(text_editor_t *self,
                                        const char *text,
                                        size_t length) {
    char *new_data = NULL;
    if (length > 0) {
        new_data = (char *)malloc(length + 1);
        if (!new_data) {
            return TEXT_EDITOR_ERROR_OUT_OF_MEMORY;
        }
        memcpy(new_data, text, length);
        new_data[length] = '\0';
    }

    if (self->clipboard) {
        free(self->clipboard);
    }
    self->clipboard = new_data;
    self->clipboard_length = length;
    return TEXT_EDITOR_ERROR_OK;
}

/* copy current selections into clipboard; optionally cut them from buffer.
   Currently handles only the first selection and only single-line selection.
   TODO: full multi-selection and multi-line support. */
static error text_editor__copy_or_cut(text_editor_t *self, bool is_cut) {
    if (self->selections.count == 0) {
        /* no selection: do not change clipboard, but still emit event */
        text_editor_event_t ev;
        ev.type = is_cut ? TEXT_EDITOR_EVENT_CUT : TEXT_EDITOR_EVENT_COPY;
        return text_editor__event_queue_push(&self->events, &ev);
    }

    text_editor_selection_t sel = self->selections.items[0];
    text_editor__normalize_selection(&sel);
    text_editor__clamp_position(self, &sel.start);
    text_editor__clamp_position(self, &sel.end);

    if (sel.start.line_index != sel.end.line_index) {
        /* multi-line selection not implemented yet */
        return TEXT_EDITOR_ERROR_INTERNAL;
    }

    if (sel.start.line_index >= self->lines.count) {
        return TEXT_EDITOR_ERROR_INDEX_OUT_OF_RANGE;
    }

    text_line_t *line = &self->lines.items[sel.start.line_index];
    size_t start_byte = text_editor__column_to_byte_offset(line,
                                                           sel.start.column_index);
    size_t end_byte = text_editor__column_to_byte_offset(line,
                                                         sel.end.column_index);
    if (end_byte < start_byte || end_byte > line->length) {
        return TEXT_EDITOR_ERROR_INTERNAL;
    }

    error err = text_editor__set_clipboard(self,
                                           line->data + start_byte,
                                           end_byte - start_byte);
    if (err != TEXT_EDITOR_ERROR_OK) {
        return err;
    }

    if (is_cut) {
        /* delete selected range and place cursor at start */
        err = text_editor__line_delete_range(line,
                                             sel.start.column_index,
                                             sel.end.column_index -
                                                 sel.start.column_index);
        if (err != TEXT_EDITOR_ERROR_OK) {
            return err;
        }
        err = text_editor__ensure_cursor_capacity(self, 1);
        if (err != TEXT_EDITOR_ERROR_OK) {
            return err;
        }
        self->cursors.count = 1;
        self->cursors.items[0].position = sel.start;
    }

    /* for both copy and cut, selections are cleared after operation */
    self->selections.count = 0;

    text_editor_event_t ev;
    ev.type = is_cut ? TEXT_EDITOR_EVENT_CUT : TEXT_EDITOR_EVENT_COPY;
    return text_editor__event_queue_push(&self->events, &ev);
}

/* ===== public API implementation ===== */

error text_editor__alloc(text_editor_t **out_editor) {
    if (!out_editor) {
        return TEXT_EDITOR_ERROR_INVALID_ARGUMENT;
    }
    text_editor_t *editor = (text_editor_t *)calloc(1, sizeof(text_editor_t));
    if (!editor) {
        *out_editor = NULL;
        return TEXT_EDITOR_ERROR_OUT_OF_MEMORY;
    }
    *out_editor = editor;
    return TEXT_EDITOR_ERROR_OK;
}

error text_editor__init(text_editor_t *self) {
    if (!self) {
        return TEXT_EDITOR_ERROR_INVALID_ARGUMENT;
    }

    self->lines.items = NULL;
    self->lines.count = 0;
    self->lines.capacity = 0;

    self->cursors.items = NULL;
    self->cursors.count = 0;
    self->cursors.capacity = 0;

    self->selections.items = NULL;
    self->selections.count = 0;
    self->selections.capacity = 0;

    self->clipboard = NULL;
    self->clipboard_length = 0;

    /* defaults */
    self->newline_sequence[0] = '\n';
    self->newline_length = 1;
    self->tab_size = 4;
    self->hover_delay_ms = 1000;

    self->last_mouse_position.line_index = 0;
    self->last_mouse_position.column_index = 0;
    self->last_mouse_move_time_ms = 0;
    self->hover_pending = false;

    text_editor__event_queue_init(&self->events);

    self->view_storage.lines = NULL;
    self->view_storage.lines_capacity = 0;
    self->view_storage.cursors = NULL;
    self->view_storage.cursors_capacity = 0;
    self->view_storage.selections = NULL;
    self->view_storage.selections_capacity = 0;

    /* one empty line and a single cursor at (0,0) */
    error err = text_editor__ensure_nonempty(self);
    if (err != TEXT_EDITOR_ERROR_OK) {
        return err;
    }

    err = text_editor__ensure_cursor_capacity(self, 1);
    if (err != TEXT_EDITOR_ERROR_OK) {
        return err;
    }
    self->cursors.count = 1;
    self->cursors.items[0].position.line_index = 0;
    self->cursors.items[0].position.column_index = 0;

    self->selections.count = 0;

    return TEXT_EDITOR_ERROR_OK;
}

error text_editor__free(text_editor_t *self) {
    if (!self) {
        return TEXT_EDITOR_ERROR_OK;
    }

    for (size_t i = 0; i < self->lines.count; ++i) {
        text_editor__line_free(&self->lines.items[i]);
    }
    free(self->lines.items);

    free(self->cursors.items);
    free(self->selections.items);

    free(self->clipboard);

    free(self->view_storage.lines);
    free(self->view_storage.cursors);
    free(self->view_storage.selections);

    free(self);
    return TEXT_EDITOR_ERROR_OK;
}

error text_editor__set_newline(text_editor_t *self,
                               const char *newline_sequence,
                               size_t newline_length) {
    if (!self || !newline_sequence || newline_length == 0 || newline_length > 2) {
        return TEXT_EDITOR_ERROR_INVALID_ARGUMENT;
    }
    memcpy(self->newline_sequence, newline_sequence, newline_length);
    self->newline_length = newline_length;
    return TEXT_EDITOR_ERROR_OK;
}

error text_editor__set_tab_size(text_editor_t *self,
                                size_t tab_size) {
    if (!self || tab_size == 0) {
        return TEXT_EDITOR_ERROR_INVALID_ARGUMENT;
    }
    self->tab_size = tab_size;
    return TEXT_EDITOR_ERROR_OK;
}

error text_editor__set_hover_delay_ms(text_editor_t *self,
                                      uint64_t hover_delay_ms) {
    if (!self) {
        return TEXT_EDITOR_ERROR_INVALID_ARGUMENT;
    }
    self->hover_delay_ms = hover_delay_ms;
    return TEXT_EDITOR_ERROR_OK;
}

/* load entire text from a C string, splitting by configured newline.
   NOTE: currently only handles '\n' and '\r\n' reasonably. */
error text_editor__set_text_from_c_string(text_editor_t *self,
                                          const char *text) {
    if (!self || !text) {
        return TEXT_EDITOR_ERROR_INVALID_ARGUMENT;
    }

    /* free old lines */
    for (size_t i = 0; i < self->lines.count; ++i) {
        text_editor__line_free(&self->lines.items[i]);
    }
    self->lines.count = 0;

    size_t capacity = 8;
    error err = text_editor__ensure_lines_capacity(self, capacity);
    if (err != TEXT_EDITOR_ERROR_OK) {
        return err;
    }

    const char *p = text;
    while (*p) {
        const char *line_start = p;
        while (*p && *p != '\n' && *p != '\r') {
            ++p;
        }
        const char *line_end = p;

        if (self->lines.count >= self->lines.capacity) {
            err = text_editor__ensure_lines_capacity(self, self->lines.count + 1);
            if (err != TEXT_EDITOR_ERROR_OK) {
                return err;
            }
        }

        text_line_t *line = &self->lines.items[self->lines.count];
        size_t length = (size_t)(line_end - line_start);
        line->capacity = length + 1;
        line->data = (char *)malloc(line->capacity);
        if (!line->data) {
            return TEXT_EDITOR_ERROR_OUT_OF_MEMORY;
        }
        memcpy(line->data, line_start, length);
        line->length = length;
        line->data[length] = '\0';
        self->lines.count += 1;

        if (*p == '\r') {
            ++p;
            if (*p == '\n') {
                ++p;
            }
        } else if (*p == '\n') {
            ++p;
        }
    }

    err = text_editor__ensure_nonempty(self);
    if (err != TEXT_EDITOR_ERROR_OK) {
        return err;
    }

    /* reset cursors and selections */
    self->cursors.count = 1;
    err = text_editor__ensure_cursor_capacity(self, 1);
    if (err != TEXT_EDITOR_ERROR_OK) {
        return err;
    }
    self->cursors.items[0].position.line_index = 0;
    self->cursors.items[0].position.column_index = 0;
    self->selections.count = 0;

    return TEXT_EDITOR_ERROR_OK;
}

error text_editor__get_text_snapshot(text_editor_t *self,
                                     const char **out_text,
                                     size_t *out_length) {
    if (!self || !out_text || !out_length) {
        return TEXT_EDITOR_ERROR_INVALID_ARGUMENT;
    }

    /* flatten lines into a temporary contiguous string is not implemented.
       For now we just return NULL and 0 and document that this is a TODO. */

    *out_text = NULL;
    *out_length = 0;
    return TEXT_EDITOR_ERROR_INTERNAL; /* TODO: implement if needed */
}

/* move all cursors by dx,dy in character space */
static void text_editor__move_cursors(text_editor_t *self, int dx, int dy) {
    for (size_t i = 0; i < self->cursors.count; ++i) {
        text_editor_position_t pos = self->cursors.items[i].position;
        /* vertical */
        if (dy < 0) {
            size_t delta = (size_t)(-dy);
            if (delta > pos.line_index) {
                pos.line_index = 0;
            } else {
                pos.line_index -= delta;
            }
        } else if (dy > 0) {
            size_t max_line = self->lines.count ? self->lines.count - 1 : 0;
            size_t tmp = pos.line_index + (size_t)dy;
            if (tmp > max_line) {
                pos.line_index = max_line;
            } else {
                pos.line_index = tmp;
            }
        }
        /* horizontal */
        text_line_t *line = &self->lines.items[pos.line_index];
        if (dx < 0) {
            size_t delta = (size_t)(-dx);
            if (delta > pos.column_index) {
                pos.column_index = 0;
            } else {
                pos.column_index -= delta;
            }
        } else if (dx > 0) {
            size_t max_col = line->length;
            size_t tmp = pos.column_index + (size_t)dx;
            if (tmp > max_col) {
                pos.column_index = max_col;
            } else {
                pos.column_index = tmp;
            }
        }
        text_editor__clamp_position(self, &pos);
        self->cursors.items[i].position = pos;
    }
}

/* clear selections and leave only cursors */
static void text_editor__clear_selections(text_editor_t *self) {
    self->selections.count = 0;
}

/* replace selections or insert at cursors */
static error text_editor__insert_text_at_cursors(text_editor_t *self,
                                                 const char *bytes,
                                                 size_t byte_count) {
    /* if there are selections, replace each selection with bytes.
       For simplicity, we currently only handle no-selections or
       a single selection. TODO: full multi-selection support. */
    if (self->selections.count > 0) {
        /* handle first selection only as skeleton implementation */
        text_editor_selection_t sel = self->selections.items[0];
        text_editor__normalize_selection(&sel);
        text_editor__clamp_position(self, &sel.start);
        text_editor__clamp_position(self, &sel.end);

        if (sel.start.line_index == sel.end.line_index) {
            text_line_t *line = &self->lines.items[sel.start.line_index];
            size_t count = sel.end.column_index - sel.start.column_index;
            error err = text_editor__line_delete_range(line, sel.start.column_index, count);
            if (err != TEXT_EDITOR_ERROR_OK) {
                return err;
            }
            err = text_editor__line_insert_bytes(line,
                                                 sel.start.column_index,
                                                 bytes,
                                                 byte_count);
            if (err != TEXT_EDITOR_ERROR_OK) {
                return err;
            }
        } else {
            /* multi-line selection replace: TODO */
            return TEXT_EDITOR_ERROR_INTERNAL;
        }

        /* collapse to a single cursor at the end of insertion */
        error err = text_editor__ensure_cursor_capacity(self, 1);
        if (err != TEXT_EDITOR_ERROR_OK) {
            return err;
        }
        self->cursors.count = 1;
        self->cursors.items[0].position.line_index = sel.start.line_index;
        self->cursors.items[0].position.column_index =
            sel.start.column_index + byte_count;
        self->selections.count = 0;
        return TEXT_EDITOR_ERROR_OK;
    }

    /* no selections: insert at each cursor, naive implementation from top to bottom */
    for (size_t i = 0; i < self->cursors.count; ++i) {
        text_cursor_t *cur = &self->cursors.items[i];
        text_line_t *line = &self->lines.items[cur->position.line_index];
        error err = text_editor__line_insert_bytes(line,
                                                   cur->position.column_index,
                                                   bytes,
                                                   byte_count);
        if (err != TEXT_EDITOR_ERROR_OK) {
            return err;
        }
        cur->position.column_index += byte_count;
    }

    return TEXT_EDITOR_ERROR_OK;
}

/* backspace: delete selection or char before cursor */
static error text_editor__backspace(text_editor_t *self) {
    if (self->selections.count > 0) {
        /* TODO: multi-selection delete; for now, handle first only */
        text_editor_selection_t sel = self->selections.items[0];
        text_editor__normalize_selection(&sel);
        text_editor__clamp_position(self, &sel.start);
        text_editor__clamp_position(self, &sel.end);
        if (sel.start.line_index == sel.end.line_index) {
            text_line_t *line = &self->lines.items[sel.start.line_index];
            size_t count = sel.end.column_index - sel.start.column_index;
            error err = text_editor__line_delete_range(line,
                                                       sel.start.column_index,
                                                       count);
            if (err != TEXT_EDITOR_ERROR_OK) {
                return err;
            }
            /* cursor at start */
            self->cursors.count = 1;
            self->cursors.items[0].position = sel.start;
        } else {
            /* multi-line delete: TODO */
            return TEXT_EDITOR_ERROR_INTERNAL;
        }
        self->selections.count = 0;
        return TEXT_EDITOR_ERROR_OK;
    }

    for (size_t i = 0; i < self->cursors.count; ++i) {
        text_cursor_t *cur = &self->cursors.items[i];
        if (cur->position.column_index > 0) {
            text_line_t *line = &self->lines.items[cur->position.line_index];
            error err = text_editor__line_delete_range(line,
                                                       cur->position.column_index - 1,
                                                       1);
            if (err != TEXT_EDITOR_ERROR_OK) {
                return err;
            }
            cur->position.column_index -= 1;
        } else if (cur->position.line_index > 0) {
            /* join with previous line */
            size_t prev_line = cur->position.line_index - 1;
            text_line_t *pline = &self->lines.items[prev_line];
            size_t old_length = pline->length;
            error err = text_editor__join_line_with_next(self, prev_line);
            if (err != TEXT_EDITOR_ERROR_OK) {
                return err;
            }
            cur->position.line_index = prev_line;
            cur->position.column_index = old_length;
        }
    }
    return TEXT_EDITOR_ERROR_OK;
}

/* delete: delete char after cursor */
static error text_editor__delete(text_editor_t *self) {
    if (self->selections.count > 0) {
        /* reuse backspace logic for selection */
        return text_editor__backspace(self);
    }

    for (size_t i = 0; i < self->cursors.count; ++i) {
        text_cursor_t *cur = &self->cursors.items[i];
        text_line_t *line = &self->lines.items[cur->position.line_index];
        if (cur->position.column_index < line->length) {
            error err = text_editor__line_delete_range(line,
                                                       cur->position.column_index,
                                                       1);
            if (err != TEXT_EDITOR_ERROR_OK) {
                return err;
            }
        } else if (cur->position.line_index + 1 < self->lines.count) {
            /* delete newline: join with next line */
            error err = text_editor__join_line_with_next(self,
                                                         cur->position.line_index);
            if (err != TEXT_EDITOR_ERROR_OK) {
                return err;
            }
        }
    }
    return TEXT_EDITOR_ERROR_OK;
}

/* home/end on current lines */
static void text_editor__home(text_editor_t *self) {
    for (size_t i = 0; i < self->cursors.count; ++i) {
        self->cursors.items[i].position.column_index = 0;
    }
    text_editor__clear_selections(self);
}

static void text_editor__end(text_editor_t *self) {
    for (size_t i = 0; i < self->cursors.count; ++i) {
        text_cursor_t *cur = &self->cursors.items[i];
        text_line_t *line = &self->lines.items[cur->position.line_index];
        cur->position.column_index = line->length;
    }
    text_editor__clear_selections(self);
}

/* naive word navigation: move to next/previous word break */
static void text_editor__move_cursors_word(text_editor_t *self, bool forward) {
    for (size_t i = 0; i < self->cursors.count; ++i) {
        text_cursor_t *cur = &self->cursors.items[i];
        text_line_t *line = &self->lines.items[cur->position.line_index];
        size_t col = cur->position.column_index;
        if (forward) {
            /* skip current word chars */
            while (col < line->length &&
                   text_editor__is_word_char((unsigned char)line->data[col])) {
                ++col;
            }
            /* skip non-word chars */
            while (col < line->length &&
                   !text_editor__is_word_char((unsigned char)line->data[col])) {
                ++col;
            }
        } else {
            if (col == 0) {
                if (cur->position.line_index > 0) {
                    cur->position.line_index -= 1;
                    line = &self->lines.items[cur->position.line_index];
                    col = line->length;
                } else {
                    col = 0;
                }
            }
            if (col > 0) {
                size_t idx = col - 1;
                while (idx > 0 &&
                       !text_editor__is_word_char((unsigned char)line->data[idx])) {
                    --idx;
                }
                while (idx > 0 &&
                       text_editor__is_word_char((unsigned char)line->data[idx - 1])) {
                    --idx;
                }
                col = idx;
            }
        }
        cur->position.column_index = col;
        text_editor__clamp_position(self, &cur->position);
    }
}

/* ctrl+F => find intent event */
static error text_editor__emit_find_intent(text_editor_t *self) {
    text_editor_event_t ev;
    ev.type = TEXT_EDITOR_EVENT_FIND_INTENT;
    return text_editor__event_queue_push(&self->events, &ev);
}

/* ctrl-click => intellisense lookup event at position */
static error text_editor__emit_intellisense_lookup(text_editor_t *self,
                                                   text_editor_position_t position) {
    text_editor_event_t ev;
    ev.type = TEXT_EDITOR_EVENT_INTELLISENSE_LOOKUP;
    ev.data.intellisense_lookup.position = position;
    return text_editor__event_queue_push(&self->events, &ev);
}

/* hover event */
static error text_editor__emit_hover_event(text_editor_t *self,
                                           text_editor_position_t position) {
    /* find word under position */
    if (position.line_index >= self->lines.count) {
        return TEXT_EDITOR_ERROR_OK;
    }
    text_line_t *line = &self->lines.items[position.line_index];
    if (position.column_index >= line->length) {
        return TEXT_EDITOR_ERROR_OK;
    }
    size_t col = position.column_index;
    if (!text_editor__is_word_char((unsigned char)line->data[col])) {
        return TEXT_EDITOR_ERROR_OK;
    }

    size_t start = col;
    while (start > 0 &&
           text_editor__is_word_char((unsigned char)line->data[start - 1])) {
        --start;
    }
    size_t end = col;
    while (end < line->length &&
           text_editor__is_word_char((unsigned char)line->data[end])) {
        ++end;
    }
    text_editor_event_t ev;
    ev.type = TEXT_EDITOR_EVENT_HOVER_WORD;
    ev.data.hover_word.position = position;
    ev.data.hover_word.word_text = line->data + start;
    ev.data.hover_word.word_byte_length = end - start;
    return text_editor__event_queue_push(&self->events, &ev);
}

/* handle keyboard input */
error text_editor__handle_key(text_editor_t *self,
                              const text_editor_key_event_t *key_event) {
    if (!self || !key_event) {
        return TEXT_EDITOR_ERROR_INVALID_ARGUMENT;
    }

    bool ctrl = (key_event->modifiers & TEXT_EDITOR_MODIFIER_CTRL) != 0;
    bool shift = (key_event->modifiers & TEXT_EDITOR_MODIFIER_SHIFT) != 0;
    bool alt = (key_event->modifiers & TEXT_EDITOR_MODIFIER_ALT) != 0;
    (void)alt; /* currently unused in keyboard handling */

    switch (key_event->key_code) {
    case TEXT_EDITOR_KEY_LEFT:
        if (ctrl) {
            text_editor__move_cursors_word(self, false);
        } else {
            text_editor__move_cursors(self, -1, 0);
        }
        if (!shift) {
            text_editor__clear_selections(self);
        }
        break;
    case TEXT_EDITOR_KEY_RIGHT:
        if (ctrl) {
            text_editor__move_cursors_word(self, true);
        } else {
            text_editor__move_cursors(self, +1, 0);
        }
        if (!shift) {
            text_editor__clear_selections(self);
        }
        break;
    case TEXT_EDITOR_KEY_UP:
        text_editor__move_cursors(self, 0, -1);
        if (!shift) {
            text_editor__clear_selections(self);
        }
        break;
    case TEXT_EDITOR_KEY_DOWN:
        text_editor__move_cursors(self, 0, +1);
        if (!shift) {
            text_editor__clear_selections(self);
        }
        break;
    case TEXT_EDITOR_KEY_HOME:
        text_editor__home(self);
        break;
    case TEXT_EDITOR_KEY_END:
        text_editor__end(self);
        break;
    case TEXT_EDITOR_KEY_BACKSPACE:
        return text_editor__backspace(self);
    case TEXT_EDITOR_KEY_DELETE:
        return text_editor__delete(self);
    case TEXT_EDITOR_KEY_TAB:
        if (shift) {
            /* TODO: block dedent for selections */
        } else {
            const char spaces[4] = { ' ', ' ', ' ', ' ' };
            size_t count = self->tab_size <= 4 ? self->tab_size : 4;
            return text_editor__insert_text_at_cursors(self, spaces, count);
        }
        break;
    case TEXT_EDITOR_KEY_ENTER: {
        /* split lines at cursors */
        for (size_t i = 0; i < self->cursors.count; ++i) {
            text_cursor_t *cur = &self->cursors.items[i];
            error err = text_editor__split_line(self,
                                                cur->position.line_index,
                                                cur->position.column_index);
            if (err != TEXT_EDITOR_ERROR_OK) {
                return err;
            }
            cur->position.line_index += 1;
            cur->position.column_index = 0;
        }
        text_editor__clear_selections(self);
    } break;
    case TEXT_EDITOR_KEY_F:
        if (ctrl && !shift && !alt) {
            return text_editor__emit_find_intent(self);
        }
        /* otherwise treat as character if input_utf8 set */
        goto handle_character_key;
    case TEXT_EDITOR_KEY_CHARACTER:
    default:
handle_character_key:
        /* ctrl+letter: copy, cut, paste or others.
           We special-case c/x/v when ctrl is down. */
        /* plain character insertion when not ctrl */
        if (ctrl && key_event->key_code == TEXT_EDITOR_KEY_CHARACTER &&
            key_event->input_utf8_length == 1) {
            char ch = (char)key_event->input_utf8[0];
            unsigned char lower = (unsigned char)tolower((unsigned char)ch);

            if (lower == 'c') {
                return text_editor__copy_or_cut(self, false);
            } else if (lower == 'x') {
                return text_editor__copy_or_cut(self, true);
            } else if (lower == 'v') {
                /* paste intent: editor cannot access OS clipboard,
                   so just emit an event and let caller invoke paste(). */
                text_editor_event_t ev;
                ev.type = TEXT_EDITOR_EVENT_PASTE_INTENT;
                return text_editor__event_queue_push(&self->events, &ev);
            }
            /* fall through to normal handling for other ctrl+char if desired */
        }

        if (!ctrl && key_event->key_code == TEXT_EDITOR_KEY_CHARACTER &&
            key_event->input_utf8_length > 0) {
            return text_editor__insert_text_at_cursors(self,
                                                       key_event->input_utf8,
                                                       key_event->input_utf8_length);
        }
        break;
    }

    return TEXT_EDITOR_ERROR_OK;
}

/* handle mouse input */
error text_editor__handle_mouse(text_editor_t *self,
                                const text_editor_mouse_event_t *mouse_event) {
    if (!self || !mouse_event) {
        return TEXT_EDITOR_ERROR_INVALID_ARGUMENT;
    }

    bool ctrl = (mouse_event->modifiers & TEXT_EDITOR_MODIFIER_CTRL) != 0;
    bool alt = (mouse_event->modifiers & TEXT_EDITOR_MODIFIER_ALT) != 0;
    bool shift = (mouse_event->modifiers & TEXT_EDITOR_MODIFIER_SHIFT) != 0;
    (void)shift; /* not currently used */

    text_editor_position_t pos = mouse_event->position;
    text_editor__clamp_position(self, &pos);

    if (mouse_event->type == TEXT_EDITOR_MOUSE_EVENT_MOVE) {
        /* update hover state */
        if (pos.line_index != self->last_mouse_position.line_index ||
            pos.column_index != self->last_mouse_position.column_index) {
            self->last_mouse_position = pos;
            self->last_mouse_move_time_ms = mouse_event->timestamp_ms;
            self->hover_pending = true;
        } else {
            if (self->hover_pending &&
                mouse_event->timestamp_ms - self->last_mouse_move_time_ms >=
                    self->hover_delay_ms) {
                self->hover_pending = false;
                (void)text_editor__emit_hover_event(self, pos);
            }
        }
        return TEXT_EDITOR_ERROR_OK;
    }

    if (mouse_event->button != TEXT_EDITOR_MOUSE_BUTTON_LEFT) {
        return TEXT_EDITOR_ERROR_OK;
    }

    if (mouse_event->type == TEXT_EDITOR_MOUSE_EVENT_BUTTON_DOWN) {
        if (ctrl) {
            /* ctrl-click intellisense lookup */
            return text_editor__emit_intellisense_lookup(self, pos);
        }

        if (!alt) {
            /* plain click: single cursor, clear selections */
            error err = text_editor__ensure_cursor_capacity(self, 1);
            if (err != TEXT_EDITOR_ERROR_OK) {
                return err;
            }
            self->cursors.count = 1;
            self->cursors.items[0].position = pos;
            self->selections.count = 0;
        } else {
            /* alt-click: add/remove cursor or selection at pos.
               Simplified: only alt-click toggles extra cursor here. */
            bool found = false;
            for (size_t i = 0; i < self->cursors.count; ++i) {
                if (self->cursors.items[i].position.line_index == pos.line_index &&
                    self->cursors.items[i].position.column_index == pos.column_index) {
                    /* remove this cursor */
                    memmove(&self->cursors.items[i],
                            &self->cursors.items[i + 1],
                            (self->cursors.count - i - 1) * sizeof(text_cursor_t));
                    self->cursors.count -= 1;
                    found = true;
                    break;
                }
            }
            if (!found) {
                error err = text_editor__ensure_cursor_capacity(self,
                                                                self->cursors.count + 1);
                if (err != TEXT_EDITOR_ERROR_OK) {
                    return err;
                }
                self->cursors.items[self->cursors.count].position = pos;
                self->cursors.count += 1;
            }
        }
    }

    /* click-drag-release and alt-click-drag-release multi-selections:
       TODO: needs tracking of drag start and button state in editor. */

    return TEXT_EDITOR_ERROR_OK;
}

error text_editor__paste(OWNED text_editor_t *self,
                         BORROWED const char *text,
                         size_t text_length) {
    if (!self || (!text && text_length > 0)) {
        return TEXT_EDITOR_ERROR_INVALID_ARGUMENT;
    }
    if (text_length == 0) {
        return TEXT_EDITOR_ERROR_OK;
    }

    /* TODO: support multi-line paste and multi-selection.
       For now we treat the text as a flat byte sequence with no newlines. */
    error err = text_editor__set_clipboard(self, text, text_length);
    if (err != TEXT_EDITOR_ERROR_OK) {
        return err;
    }
    return text_editor__insert_text_at_cursors(self, text, text_length);
}

/* scrolling */
error text_editor__scroll(text_editor_t *self,
                          const text_editor_scroll_command_t *command) {
    if (!self || !command) {
        return TEXT_EDITOR_ERROR_INVALID_ARGUMENT;
    }

    text_editor_event_t ev;
    ev.type = TEXT_EDITOR_EVENT_SCROLL_INTENT;
    ev.data.scroll_intent.mode = command->mode;
    ev.data.scroll_intent.delta_lines = command->delta_lines;
    ev.data.scroll_intent.delta_columns = command->delta_columns;
    ev.data.scroll_intent.absolute_first_line = command->absolute_first_line;
    ev.data.scroll_intent.absolute_first_column = command->absolute_first_column;

    return text_editor__event_queue_push(&self->events, &ev);
}

/* find_all: naive substring search, case-sensitive/insensitive,
   TODO: whole_word, regexp. */
error text_editor__find_all(text_editor_t *self,
                            const text_editor_find_options_t *options) {
    if (!self || !options || !options->pattern || options->pattern_length == 0) {
        return TEXT_EDITOR_ERROR_INVALID_ARGUMENT;
    }

    self->selections.count = 0;

    error err = TEXT_EDITOR_ERROR_OK;

    for (size_t li = 0; li < self->lines.count; ++li) {
        text_line_t *line = &self->lines.items[li];
        size_t i = 0;
        while (i + options->pattern_length <= line->length) {
            bool match = true;
            for (size_t j = 0; j < options->pattern_length; ++j) {
                unsigned char a = (unsigned char)line->data[i + j];
                unsigned char b = (unsigned char)options->pattern[j];
                if (!options->case_sensitive) {
                    a = (unsigned char)tolower(a);
                    b = (unsigned char)tolower(b);
                }
                if (a != b) {
                    match = false;
                    break;
                }
            }
            if (match) {
                if (options->whole_word) {
                    bool left_ok = (i == 0) ||
                                   !text_editor__is_word_char((unsigned char)line->data[i - 1]);
                    bool right_ok = (i + options->pattern_length >= line->length) ||
                                    !text_editor__is_word_char(
                                        (unsigned char)line->data[i + options->pattern_length]);
                    if (!left_ok || !right_ok) {
                        i += 1;
                        continue;
                    }
                }

                err = text_editor__ensure_selection_capacity(self,
                                                             self->selections.count + 1);
                if (err != TEXT_EDITOR_ERROR_OK) {
                    return err;
                }
                text_editor_selection_t *sel =
                    &self->selections.items[self->selections.count];
                sel->start.line_index = li;
                sel->start.column_index = i;
                sel->end.line_index = li;
                sel->end.column_index = i + options->pattern_length;
                self->selections.count += 1;

                i += options->pattern_length;
            } else {
                i += 1;
            }
        }
    }

    /* emit event that selections updated */
    text_editor_event_t ev;
    ev.type = TEXT_EDITOR_EVENT_FIND_RESULTS_UPDATED;
    err = text_editor__event_queue_push(&self->events, &ev);
    if (err != TEXT_EDITOR_ERROR_OK) {
        return err;
    }

    return TEXT_EDITOR_ERROR_OK;
}

/* build view for given viewport */
error text_editor__get_view(text_editor_t *self,
                            const text_editor_viewport_t *viewport,
                            text_editor_view_t *out_view) {
    if (!self || !viewport || !out_view) {
        return TEXT_EDITOR_ERROR_INVALID_ARGUMENT;
    }

    size_t from_line = viewport->first_line_index;
    size_t to_line = text_editor__min_size(from_line + viewport->height,
                                           self->lines.count);

    /* ensure line view capacity */
    size_t visible_lines = (to_line > from_line) ? (to_line - from_line) : 0;
    if (self->view_storage.lines_capacity < visible_lines) {
        text_editor_view_line_t *new_lines =
            (text_editor_view_line_t *)realloc(self->view_storage.lines,
                                               visible_lines *
                                                   sizeof(text_editor_view_line_t));
        if (!new_lines) {
            return TEXT_EDITOR_ERROR_OUT_OF_MEMORY;
        }
        self->view_storage.lines = new_lines;
        self->view_storage.lines_capacity = visible_lines;
    }

    for (size_t i = 0; i < visible_lines; ++i) {
        size_t line_index = from_line + i;
        text_line_t *line = &self->lines.items[line_index];

        size_t start_col = viewport->first_column;
        size_t end_col = start_col + viewport->width;
        if (start_col > line->length) {
            start_col = line->length;
        }
        if (end_col > line->length) {
            end_col = line->length;
        }

        size_t start_byte = text_editor__column_to_byte_offset(line, start_col);
        size_t end_byte = text_editor__column_to_byte_offset(line, end_col);

        text_editor_view_line_t *vl = &self->view_storage.lines[i];
        vl->buffer_line_index = line_index;
        vl->line_number = line_index + 1;
        vl->text = line->data + start_byte;
        vl->byte_length = end_byte - start_byte;
    }

    /* cursors: we simply pass all cursors; renderer can clip */
    if (self->view_storage.cursors_capacity < self->cursors.count) {
        text_editor_view_cursor_t *new_curs =
            (text_editor_view_cursor_t *)realloc(self->view_storage.cursors,
                                                 self->cursors.count *
                                                     sizeof(text_editor_view_cursor_t));
        if (!new_curs) {
            return TEXT_EDITOR_ERROR_OUT_OF_MEMORY;
        }
        self->view_storage.cursors = new_curs;
        self->view_storage.cursors_capacity = self->cursors.count;
    }

    for (size_t ci = 0; ci < self->cursors.count; ++ci) {
        text_cursor_t *cur = &self->cursors.items[ci];
        text_editor_view_cursor_t *vc = &self->view_storage.cursors[ci];
        vc->position = cur->position;
        vc->has_character = false;
        vc->character = '\0';
        if (cur->position.line_index < self->lines.count) {
            text_line_t *line = &self->lines.items[cur->position.line_index];
            if (cur->position.column_index < line->length) {
                size_t byte = text_editor__column_to_byte_offset(line,
                                                                 cur->position.column_index);
                vc->has_character = true;
                vc->character = line->data[byte];
            }
        }
    }

    /* selections: we pass them as-is; for simplicity we do not clip text
       to viewport, only provide full selected text per-line selection. */
    if (self->view_storage.selections_capacity < self->selections.count) {
        text_editor_view_selection_t *new_sel =
            (text_editor_view_selection_t *)realloc(self->view_storage.selections,
                                                    self->selections.count *
                                                        sizeof(text_editor_view_selection_t));
        if (!new_sel) {
            return TEXT_EDITOR_ERROR_OUT_OF_MEMORY;
        }
        self->view_storage.selections = new_sel;
        self->view_storage.selections_capacity = self->selections.count;
    }

    for (size_t si = 0; si < self->selections.count; ++si) {
        text_editor_view_selection_t *vs = &self->view_storage.selections[si];
        vs->range = self->selections.items[si];
        /* single-line selection snapshot only */
        text_editor_selection_t sel = self->selections.items[si];
        text_editor__normalize_selection(&sel);
        if (sel.start.line_index == sel.end.line_index &&
            sel.start.line_index < self->lines.count) {
            text_line_t *line = &self->lines.items[sel.start.line_index];
            size_t start_byte = text_editor__column_to_byte_offset(line,
                                                                   sel.start.column_index);
            size_t end_byte = text_editor__column_to_byte_offset(line,
                                                                 sel.end.column_index);
            vs->text = line->data + start_byte;
            vs->byte_length = end_byte - start_byte;
        } else {
            vs->text = NULL;
            vs->byte_length = 0;
        }
    }

    /* scrollbar: simple approximation */
    float thumb_pos = 0.0f;
    float thumb_size = 1.0f;
    if (self->lines.count > 0 && viewport->height > 0) {
        float total_lines = (float)self->lines.count;
        float visible = (float)viewport->height;
        if (visible > total_lines) {
            visible = total_lines;
        }
        thumb_size = visible / total_lines;
        if (thumb_size < 0.05f) {
            thumb_size = 0.05f;
        }
        float max_scroll = (float)(self->lines.count > viewport->height
                                       ? self->lines.count - viewport->height
                                       : 1);
        thumb_pos = (float)viewport->first_line_index / max_scroll;
        if (thumb_pos < 0.0f) thumb_pos = 0.0f;
        if (thumb_pos > 1.0f) thumb_pos = 1.0f;
    }

    out_view->lines = self->view_storage.lines;
    out_view->line_count = visible_lines;
    out_view->cursors = self->view_storage.cursors;
    out_view->cursor_count = self->cursors.count;
    out_view->selections = self->view_storage.selections;
    out_view->selection_count = self->selections.count;
    out_view->scrollbar.vertical_thumb_position = thumb_pos;
    out_view->scrollbar.vertical_thumb_size = thumb_size;

    return TEXT_EDITOR_ERROR_OK;
}

/* event polling */
error text_editor__next_event(text_editor_t *self,
                              text_editor_event_t *out_event) {
    if (!self || !out_event) {
        return TEXT_EDITOR_ERROR_INVALID_ARGUMENT;
    }
    return text_editor__event_queue_pop(&self->events, out_event);
}

/* error message */
const char *text_editor__error_message(error err) {
    switch (err) {
    case TEXT_EDITOR_ERROR_OK:
        return "ok";
    case TEXT_EDITOR_ERROR_OUT_OF_MEMORY:
        return "out of memory";
    case TEXT_EDITOR_ERROR_INVALID_ARGUMENT:
        return "invalid argument";
    case TEXT_EDITOR_ERROR_INDEX_OUT_OF_RANGE:
        return "index out of range";
    case TEXT_EDITOR_ERROR_EVENT_QUEUE_FULL:
        return "event queue full";
    case TEXT_EDITOR_ERROR_INTERNAL:
        return "internal error";
    default:
        return "unknown error";
    }
}