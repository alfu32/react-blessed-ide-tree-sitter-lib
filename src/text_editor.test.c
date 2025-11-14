#include "test_framework.h"
#include <string.h>
#include "text_editor.h"

static text_editor_t *create_editor_empty() {
    text_editor_t *editor = NULL;
    error err = text_editor__alloc(&editor);
    ASSERT(err == TEXT_EDITOR_ERROR_OK, "editor allocation");
    ASSERT(editor != NULL, "editor non-null");
    err = text_editor__init(editor);
    ASSERT(err == TEXT_EDITOR_ERROR_OK, "editor initialisation");
    return editor;
}

static text_editor_t *create_editor_with_text(const char *text) {
    text_editor_t *editor = NULL;
    error err = text_editor__alloc(&editor);
    ASSERT(err == TEXT_EDITOR_ERROR_OK, "editor allocation");
    ASSERT(editor != NULL, "editor non-null");
    err = text_editor__init(editor);
    ASSERT(err == TEXT_EDITOR_ERROR_OK, "editor initialisation");
    err = text_editor__set_text_from_c_string(editor, text);
    ASSERT(err == TEXT_EDITOR_ERROR_OK, "set text from c string");
    return editor;
}

static void copy_view_line_to_buffer(const text_editor_view_line_t *line,
                                     char *buffer,
                                     size_t buffer_size) {
    size_t n = (line->byte_length < buffer_size - 1) ? line->byte_length : buffer_size - 1;
    memcpy(buffer, line->text, n);
    buffer[n] = '\0';
}

int main(void) {
    TEST_SUITE_BEGIN(20);

    TEST("editor allocation and initialisation") {
        text_editor_t *editor = NULL;
        error err = text_editor__alloc(&editor);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "alloc returns ok");
        ASSERT(editor != NULL, "editor pointer not null");

        err = text_editor__init(editor);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "init returns ok");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 80;
        viewport.height = 25;

        text_editor_view_t view;
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view returns ok");
        ASSERT(view.line_count >= 1, "at least one line visible");
        ASSERT(view.cursor_count == 1, "single default cursor");

        error snap_err;
        const char *snap_text = NULL;
        size_t snap_length = 0;
        snap_err = text_editor__get_text_snapshot(editor, &snap_text, &snap_length);
        ASSERT(snap_err != TEXT_EDITOR_ERROR_OK, "text snapshot not yet implemented");

        text_editor__free(editor);
    }

    TEST("set text and basic viewport") {
        text_editor_t *editor = create_editor_with_text("abc\ndef");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 10;
        viewport.height = 10;

        text_editor_view_t view;
        error err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view returns ok");
        ASSERT(view.line_count == 2, "two lines visible");
        ASSERT(view.lines[0].line_number == 1, "first line number is 1");
        ASSERT(view.lines[1].line_number == 2, "second line number is 2");

        char buf0[16];
        char buf1[16];
        copy_view_line_to_buffer(&view.lines[0], buf0, sizeof(buf0));
        copy_view_line_to_buffer(&view.lines[1], buf1, sizeof(buf1));

        ASSERT(strcmp(buf0, "abc") == 0, "first line content is abc");
        ASSERT(strcmp(buf1, "def") == 0, "second line content is def");

        text_editor__free(editor);
    }

    TEST("basic cursor movement with arrows") {
        text_editor_t *editor = create_editor_with_text("abc\ndef");

        text_editor_key_event_t ev;
        memset(&ev, 0, sizeof(ev));

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 10;
        viewport.height = 10;
        text_editor_view_t view;

        ev.key_code = TEXT_EDITOR_KEY_RIGHT;
        ev.modifiers = 0;
        ev.input_utf8_length = 0;
        error err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "right arrow returns ok");

        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view returns ok after right");
        ASSERT(view.cursor_count == 1, "single cursor after right");
        ASSERT(view.cursors[0].position.line_index == 0, "cursor on first line");
        ASSERT(view.cursors[0].position.column_index == 1, "cursor column 1 after right");

        ev.key_code = TEXT_EDITOR_KEY_DOWN;
        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "down arrow returns ok");

        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view returns ok after down");
        ASSERT(view.cursors[0].position.line_index == 1, "cursor moved to second line");
        ASSERT(view.cursors[0].position.column_index <= 3, "cursor column within line bounds");

        ev.key_code = TEXT_EDITOR_KEY_LEFT;
        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "left arrow returns ok");

        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view returns ok after left");

        text_editor__free(editor);
    }

    TEST("home and end keys move within line") {
        text_editor_t *editor = create_editor_with_text("abcde\nxyz");

        text_editor_key_event_t ev;
        memset(&ev, 0, sizeof(ev));
        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 10;
        viewport.height = 10;
        text_editor_view_t view;

        ev.key_code = TEXT_EDITOR_KEY_RIGHT;
        error err = text_editor__handle_key(editor, &ev);
        err = text_editor__handle_key(editor, &ev);
        err = text_editor__handle_key(editor, &ev);

        ev.key_code = TEXT_EDITOR_KEY_HOME;
        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "home returns ok");
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after home");
        ASSERT(view.cursors[0].position.column_index == 0, "cursor at column 0 after home");

        ev.key_code = TEXT_EDITOR_KEY_END;
        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "end returns ok");
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after end");
        ASSERT(view.cursors[0].position.column_index == 5, "cursor at line end after end");

        text_editor__free(editor);
    }

    TEST("insert characters without selections") {
        text_editor_t *editor = create_editor_with_text("");

        text_editor_key_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.key_code = TEXT_EDITOR_KEY_CHARACTER;

        ev.modifiers = 0;
        ev.input_utf8[0] = 'a';
        ev.input_utf8_length = 1;
        error err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "insert a ok");

        ev.input_utf8[0] = 'b';
        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "insert b ok");

        ev.input_utf8[0] = 'c';
        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "insert c ok");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 10;
        viewport.height = 10;
        text_editor_view_t view;
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after inserts");
        ASSERT(view.line_count >= 1, "has at least one line");

        char buf[16];
        copy_view_line_to_buffer(&view.lines[0], buf, sizeof(buf));
        ASSERT(strcmp(buf, "abc") == 0, "line content is abc");

        text_editor__free(editor);
    }

    TEST("backspace and delete without selections") {
        text_editor_t *editor = create_editor_with_text("abc");

        text_editor_key_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.key_code = TEXT_EDITOR_KEY_END;
        error err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "end key ok");

        ev.key_code = TEXT_EDITOR_KEY_BACKSPACE;
        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "backspace ok");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 10;
        viewport.height = 10;
        text_editor_view_t view;
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after backspace");
        char buf[16];
        copy_view_line_to_buffer(&view.lines[0], buf, sizeof(buf));
        ASSERT(strcmp(buf, "ab") == 0, "text after backspace is ab");

        ev.key_code = TEXT_EDITOR_KEY_HOME;
        err = text_editor__handle_key(editor, &ev);
        ev.key_code = TEXT_EDITOR_KEY_DELETE;
        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "delete ok");

        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after delete");
        copy_view_line_to_buffer(&view.lines[0], buf, sizeof(buf));
        ASSERT(strcmp(buf, "b") == 0, "text after delete is b");

        text_editor__free(editor);
    }

    TEST("enter key splits lines") {
        text_editor_t *editor = create_editor_with_text("abcd");

        text_editor_key_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.key_code = TEXT_EDITOR_KEY_RIGHT;
        error err = text_editor__handle_key(editor, &ev);
        err = text_editor__handle_key(editor, &ev);

        ev.key_code = TEXT_EDITOR_KEY_ENTER;
        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "enter key ok");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 10;
        viewport.height = 10;
        text_editor_view_t view;
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after enter");
        ASSERT(view.line_count >= 2, "two lines after split");

        char buf0[16];
        char buf1[16];
        copy_view_line_to_buffer(&view.lines[0], buf0, sizeof(buf0));
        copy_view_line_to_buffer(&view.lines[1], buf1, sizeof(buf1));
        ASSERT(strcmp(buf0, "ab") == 0, "first split line is ab");
        ASSERT(strcmp(buf1, "cd") == 0, "second split line is cd");

        text_editor__free(editor);
    }

    TEST("ctrl arrow word navigation") {
        text_editor_t *editor = create_editor_with_text("one two_three  four");

        text_editor_key_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.modifiers = TEXT_EDITOR_MODIFIER_CTRL;
        ev.key_code = TEXT_EDITOR_KEY_RIGHT;

        error err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "ctrl-right first ok");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 80;
        viewport.height = 10;
        text_editor_view_t view;
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after ctrl-right");
        ASSERT(view.cursors[0].position.column_index == 4, "cursor at start of two");

        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "ctrl-right second ok");
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after second ctrl-right");
        ASSERT(view.cursors[0].position.column_index > 4, "cursor moved into three or beyond");

        ev.key_code = TEXT_EDITOR_KEY_LEFT;
        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "ctrl-left ok");

        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after ctrl-left");

        text_editor__free(editor);
    }

    TEST("find_all case-insensitive plain string") {
        text_editor_t *editor = create_editor_with_text("Abc abc ABC");

        text_editor_find_options_t options;
        options.pattern = "abc";
        options.pattern_length = 3;
        options.case_sensitive = false;
        options.whole_word = false;
        options.is_regular_expression = false;

        error err = text_editor__find_all(editor, &options);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "find_all returns ok");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 80;
        viewport.height = 10;
        text_editor_view_t view;
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after find_all");
        ASSERTF(view.selection_count == 3, "selection count is %zu", view.selection_count);

        ASSERT(view.selections[0].range.start.column_index == 0, "first match at 0");
        ASSERT(view.selections[0].range.end.column_index == 3, "first match length 3");
        ASSERT(view.selections[1].range.start.column_index == 4, "second match at 4");
        ASSERT(view.selections[2].range.start.column_index == 8, "third match at 8");

        text_editor__free(editor);
    }

    TEST("find_all whole word only") {
        text_editor_t *editor = create_editor_with_text("abcx abc abcd abc");

        text_editor_find_options_t options;
        options.pattern = "abc";
        options.pattern_length = 3;
        options.case_sensitive = true;
        options.whole_word = true;
        options.is_regular_expression = false;

        error err = text_editor__find_all(editor, &options);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "find_all returns ok");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 80;
        viewport.height = 10;
        text_editor_view_t view;
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after find_all whole word");

        ASSERTF(view.selection_count == 2, "selection count is %zu", view.selection_count);
        ASSERT(view.selections[0].range.start.column_index == 5, "first whole word at 5");
        ASSERT(view.selections[1].range.start.column_index == 14, "second whole word at 14");

        text_editor__free(editor);
    }

    TEST("ctrl-F emits find intent event") {
        text_editor_t *editor = create_editor_with_text("sample");

        text_editor_key_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.key_code = TEXT_EDITOR_KEY_F;
        ev.modifiers = TEXT_EDITOR_MODIFIER_CTRL;
        ev.input_utf8_length = 0;

        error err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "handle ctrl-F ok");

        text_editor_event_t out_event;
        err = text_editor__next_event(editor, &out_event);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "next_event ok");
        ASSERT(out_event.type == TEXT_EDITOR_EVENT_FIND_INTENT, "find intent event emitted");

        text_editor__free(editor);
    }

    TEST("ctrl-click emits intellisense lookup event") {
        text_editor_t *editor = create_editor_with_text("abc");

        text_editor_mouse_event_t mev;
        memset(&mev, 0, sizeof(mev));
        mev.type = TEXT_EDITOR_MOUSE_EVENT_BUTTON_DOWN;
        mev.button = TEXT_EDITOR_MOUSE_BUTTON_LEFT;
        mev.modifiers = TEXT_EDITOR_MODIFIER_CTRL;
        mev.x = 1;
        mev.y = 0;
        mev.timestamp_ms = 0;

        error err = text_editor__handle_mouse(editor, &mev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "handle ctrl-click ok");

        text_editor_event_t ev;
        err = text_editor__next_event(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "next_event ok");
        ASSERT(ev.type == TEXT_EDITOR_EVENT_INTELLISENSE_LOOKUP, "intellisense lookup event type");
        ASSERT(ev.data.intellisense_lookup.position.line_index == 0, "intellisense line index 0");
        ASSERT(ev.data.intellisense_lookup.position.column_index == 1, "intellisense column index 1");

        text_editor__free(editor);
    }

    TEST("hover debounce emits hover word event") {
        text_editor_t *editor = create_editor_with_text("hello");

        error err = text_editor__set_hover_delay_ms(editor, 10);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "set hover delay ok");

        text_editor_mouse_event_t mev;
        memset(&mev, 0, sizeof(mev));
        mev.type = TEXT_EDITOR_MOUSE_EVENT_MOVE;
        mev.button = TEXT_EDITOR_MOUSE_BUTTON_NONE;
        mev.modifiers = 0;
        mev.x = 1;
        mev.y = 0;
        mev.timestamp_ms = 0;

        err = text_editor__handle_mouse(editor, &mev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "first move ok");

        mev.timestamp_ms = 20;
        err = text_editor__handle_mouse(editor, &mev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "second move ok");

        text_editor_event_t ev;
        err = text_editor__next_event(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "next_event ok");
        ASSERT(ev.type == TEXT_EDITOR_EVENT_HOVER_WORD, "hover word event type");
        ASSERT(ev.data.hover_word.position.line_index == 0, "hover position line 0");

        char word_buf[16];
        size_t n = (ev.data.hover_word.word_byte_length < sizeof(word_buf) - 1)
                       ? ev.data.hover_word.word_byte_length
                       : sizeof(word_buf) - 1;
        memcpy(word_buf, ev.data.hover_word.word_text, n);
        word_buf[n] = '\0';
        ASSERT(strcmp(word_buf, "hello") == 0, "hovered word is hello");

        text_editor__free(editor);
    }

    TEST("alt-click toggles additional cursor") {
        text_editor_t *editor = create_editor_with_text("abc\ndef");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 80;
        viewport.height = 10;
        text_editor_view_t view;
        error err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "initial get_view ok");
        ASSERT(view.cursor_count == 1, "initial single cursor");

        text_editor_mouse_event_t mev;
        memset(&mev, 0, sizeof(mev));
        mev.type = TEXT_EDITOR_MOUSE_EVENT_BUTTON_DOWN;
        mev.button = TEXT_EDITOR_MOUSE_BUTTON_LEFT;
        mev.modifiers = TEXT_EDITOR_MODIFIER_ALT;
        mev.x = 1;
        mev.y = 0;
        mev.timestamp_ms = 0;

        err = text_editor__handle_mouse(editor, &mev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "alt-click add cursor ok");

        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after alt-click");
        ASSERT(view.cursor_count == 2, "two cursors after alt-click");

        err = text_editor__handle_mouse(editor, &mev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "alt-click remove cursor ok");

        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after alt-click remove");
        ASSERT(view.cursor_count == 1, "back to single cursor");

        text_editor__free(editor);
    }

    TEST("scroll modifies mouse-to-line mapping") {
        text_editor_t *editor = create_editor_with_text("line0\nline1\nline2\nline3\nline4");

        text_editor_mouse_event_t mev;
        memset(&mev, 0, sizeof(mev));
        mev.type = TEXT_EDITOR_MOUSE_EVENT_BUTTON_DOWN;
        mev.button = TEXT_EDITOR_MOUSE_BUTTON_LEFT;
        mev.modifiers = TEXT_EDITOR_MODIFIER_ALT;
        mev.x = 1;
        mev.y = 2;
        mev.timestamp_ms = 0;

        error err = text_editor__handle_mouse(editor, &mev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "alt-click before scroll ok");

        text_editor_mouse_event_t mev2;
        memset(&mev2, 0, sizeof(mev2));
        mev2.type = TEXT_EDITOR_MOUSE_EVENT_BUTTON_DOWN;
        mev2.button = TEXT_EDITOR_MOUSE_BUTTON_LEFT;
        mev2.modifiers = TEXT_EDITOR_MODIFIER_ALT;
        mev2.x = 2;
        mev2.y = 3;
        mev2.timestamp_ms = 0;

        error err2 = text_editor__handle_mouse(editor, &mev);
        ASSERT(err2 == TEXT_EDITOR_ERROR_OK, "alt-click 2 before scroll ok");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 80;
        viewport.height = 10;
        text_editor_view_t view;
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view before scroll");
        ASSERT(view.cursor_count >= 2, "two cursors before scroll");
        ASSERT(view.cursors[1].position.line_index == 0, "second cursor at line 0 before scroll");

        text_editor_scroll_command_t scroll;
        scroll.mode = TEXT_EDITOR_SCROLL_MODE_RELATIVE;
        scroll.delta_lines = 2;
        scroll.delta_columns = 0;
        scroll.absolute_first_line = 0;
        scroll.absolute_first_column = 0;
        err = text_editor__scroll(editor, &scroll);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "scroll relative ok");

        mev.timestamp_ms = 1;
        err = text_editor__handle_mouse(editor, &mev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "alt-click after scroll ok");

        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after scroll");
        ASSERT(view.cursor_count >= 3, "three cursors after second alt-click");
        ASSERT(view.cursors[2].position.line_index == 2, "new cursor at line 2 after scroll");

        text_editor__free(editor);
    }

    TEST("ctrl-shift-arrow selects word (missing feature)") {
        text_editor_t *editor = create_editor_with_text("hello world");

        text_editor_key_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.key_code = TEXT_EDITOR_KEY_RIGHT;
        ev.modifiers = TEXT_EDITOR_MODIFIER_CTRL | TEXT_EDITOR_MODIFIER_SHIFT;

        error err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "ctrl-shift-right ok");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 80;
        viewport.height = 10;
        text_editor_view_t view;
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after ctrl-shift-right");

        ASSERT(view.selection_count >= 1, "word selection created by ctrl-shift-right");
        if (view.selection_count >= 1) {
            ASSERT(view.selections[0].range.start.column_index == 0, "selection start at 0");
            ASSERT(view.selections[0].range.end.column_index == 5, "selection end at 5");
        }

        text_editor__free(editor);
    }

    TEST("mouse drag selection creates selection (missing feature)") {
        text_editor_t *editor = create_editor_with_text("abcdef");

        text_editor_mouse_event_t mev;
        memset(&mev, 0, sizeof(mev));
        mev.button = TEXT_EDITOR_MOUSE_BUTTON_LEFT;
        mev.modifiers = 0;

        mev.type = TEXT_EDITOR_MOUSE_EVENT_BUTTON_DOWN;
        mev.x = 1;
        mev.y = 0;
        mev.timestamp_ms = 0;
        error err = text_editor__handle_mouse(editor, &mev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "mouse down ok");

        mev.type = TEXT_EDITOR_MOUSE_EVENT_MOVE;
        mev.x = 4;
        mev.timestamp_ms = 1;
        err = text_editor__handle_mouse(editor, &mev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "mouse move ok");

        mev.type = TEXT_EDITOR_MOUSE_EVENT_BUTTON_UP;
        mev.x = 4;
        mev.timestamp_ms = 2;
        err = text_editor__handle_mouse(editor, &mev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "mouse up ok");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 80;
        viewport.height = 10;
        text_editor_view_t view;
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after drag selection");

        ASSERT(view.selection_count >= 1, "selection created by drag");
        if (view.selection_count >= 1) {
            ASSERT(view.selections[0].range.start.column_index == 1, "selection start at 1");
            ASSERT(view.selections[0].range.end.column_index == 5, "selection end at 5");
        }

        text_editor__free(editor);
    }

    TEST("ctrl-c ctrl-v copy paste selections (missing feature)") {
        text_editor_t *editor = create_editor_with_text("hello world");

        text_editor_find_options_t options;
        options.pattern = "hello";
        options.pattern_length = 5;
        options.case_sensitive = true;
        options.whole_word = true;
        options.is_regular_expression = false;

        error err = text_editor__find_all(editor, &options);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "find_all for hello ok");

        text_editor_key_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.key_code = TEXT_EDITOR_KEY_CHARACTER;
        ev.modifiers = TEXT_EDITOR_MODIFIER_CTRL;
        ev.input_utf8[0] = 'c';
        ev.input_utf8_length = 1;

        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "ctrl-c ok");

        ev.key_code = TEXT_EDITOR_KEY_END;
        ev.modifiers = 0;
        ev.input_utf8_length = 0;
        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "end before paste ok");

        ev.key_code = TEXT_EDITOR_KEY_CHARACTER;
        ev.modifiers = TEXT_EDITOR_MODIFIER_CTRL;
        ev.input_utf8[0] = 'v';
        ev.input_utf8_length = 1;
        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "ctrl-v ok");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 80;
        viewport.height = 10;
        text_editor_view_t view;
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after ctrl-v");

        char buf[64];
        copy_view_line_to_buffer(&view.lines[0], buf, sizeof(buf));
        ASSERT(strcmp(buf, "hello worldhello") == 0, "line reflects pasted selection at end");

        text_editor__free(editor);
    }

    TEST("shift-tab dedents line or block (missing feature)") {
        text_editor_t *editor = create_editor_with_text("    line1\n    line2");

        text_editor_key_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.key_code = TEXT_EDITOR_KEY_HOME;
        ev.modifiers = 0;
        error err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "home ok");

        ev.key_code = TEXT_EDITOR_KEY_TAB;
        ev.modifiers = TEXT_EDITOR_MODIFIER_SHIFT;
        err = text_editor__handle_key(editor, &ev);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "shift-tab ok");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 80;
        viewport.height = 10;
        text_editor_view_t view;
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after shift-tab");

        char buf0[32];
        char buf1[32];
        copy_view_line_to_buffer(&view.lines[0], buf0, sizeof(buf0));
        copy_view_line_to_buffer(&view.lines[1], buf1, sizeof(buf1));

        ASSERT(strcmp(buf0, "line1") == 0, "first line dedented to no leading spaces");
        ASSERT(strcmp(buf1, "    line2") == 0, "second line unchanged");

        text_editor__free(editor);
    }

    TEST("regexp search finds matches (missing feature)") {
        text_editor_t *editor = create_editor_with_text("abc 123 def 456");

        text_editor_find_options_t options;
        options.pattern = "[0-9]+";
        options.pattern_length = 5;
        options.case_sensitive = true;
        options.whole_word = false;
        options.is_regular_expression = true;

        error err = text_editor__find_all(editor, &options);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "find_all regexp ok");

        text_editor_viewport_t viewport;
        viewport.first_line_index = 0;
        viewport.first_column = 0;
        viewport.width = 80;
        viewport.height = 10;
        text_editor_view_t view;
        err = text_editor__get_view(editor, &viewport, &view);
        ASSERT(err == TEXT_EDITOR_ERROR_OK, "get_view after regexp find_all");

        ASSERTF(view.selection_count == 2, "regexp selections count is %zu", view.selection_count);

        text_editor__free(editor);
    }

    TEST_SUITE_END();
    return 0;
}
