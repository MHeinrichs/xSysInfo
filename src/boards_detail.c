// SPDX-License-Identifier: BSD-2-Clause
// SPDX-FileCopyrightText: 2025 Stefan Reinauer

/*
 * xSysInfo - Memory information and view
 */

#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "xsysinfo.h"
#include "gui.h"
#include "locale_str.h"
#include "benchmark.h"
#include "debug.h"
#include "hardware.h"
#include "boards.h"
#include "gui.h"
#include "locale_str.h"
#include "debug.h"
#ifdef __KICK13__
#include "tagitem.h"
#endif

/* Global memory region list */
LONG board_detail_index = 0;

#define BOARD_DETAIL_INFO_X        128
#define BOARD_DETAIL_VALUE_OFFSET  168
#define BOARD_DETAIL_VALUE_X       (BOARD_DETAIL_INFO_X + BOARD_DETAIL_VALUE_OFFSET)
#define BOARD_DETAIL_VALUE_MAX_X   618

/* External references */
extern BoardList board_list;
extern AppContext *app;






void format_board_detail_hex_val( ULONG val, char *buffer,
                                size_t size)
{
        if (val >= (1024*1024)) {
            snprintf(buffer, size, "%lu.%lu MB",
                     (unsigned long)(val / (1024*1024)),
                     (unsigned long)((val % (1024*1024)) / (1024*1024)));
        } else if (val >= 1024) {
            snprintf(buffer, size, "%lu.%lu KB",
                     (unsigned long)(val / 1024),
                     (unsigned long)((val % 1024) / 1024));
        } else if (val > 0) {
            snprintf(buffer, size, "%lu B", (unsigned long)val);
        } else {
            snprintf(buffer, size, "---");
        }
}

static void draw_board_detail_buttons(void)
{
    Button *btn;

    btn = find_button(BTN_BOARD_DETAIL_PREV);
    if (btn) draw_button(btn);
    btn = find_button(BTN_BOARD_DETAIL_COUNTER);
    if (btn) draw_button(btn);
    btn = find_button(BTN_BOARD_DETAIL_NEXT);
    if (btn) draw_button(btn);
    btn = find_button(BTN_BOARD_DETAIL_DISPLAY);
    if (btn) draw_button(btn);
    btn = find_button(BTN_BOARD_DETAIL_EXIT);
    if (btn) draw_button(btn);
}

static void draw_board_detail_value(WORD y, const char *value)
{
    struct RastPort *rp = app->rp;

    SetAPen(rp, COLOR_PANEL_BG);
    RectFill(rp, BOARD_DETAIL_VALUE_X, y - 8, BOARD_DETAIL_VALUE_MAX_X, y + 2);
    SetAPen(rp, COLOR_HIGHLIGHT);
    SetBPen(rp, COLOR_PANEL_BG);
    draw_text_clipped(BOARD_DETAIL_VALUE_X, y, value,
                      BOARD_DETAIL_VALUE_MAX_X - BOARD_DETAIL_VALUE_X);
}

static void draw_board_detail_values(void)
{
    char buffer[64];
    WORD y = 44;
  
  
    draw_board_detail_value(y, buffer);
}

/*
 * Draw board detail data area (info panel and navigation buttons - no title)
 */
static void draw_board_detail_data(BOOL full_redraw)
{
    struct RastPort *rp = app->rp;
    char buffer[64];
    WORD y;

    if (full_redraw) {
        /* Draw memory info panel with 3D border */
        draw_panel(100, 28, 520, 150, NULL);
    } else {
        draw_board_detail_values();
        draw_board_detail_buttons();
        return;
    }


    /* Draw navigation buttons */
    draw_board_detail_buttons();
}

void draw_board_detail_view(void)
{
    /* Draw title panel */
    draw_panel(100, 0, 520, 24, NULL);

    draw_text_centered(100, 14, 520, get_string(MSG_BOARDS_DETAIL_INFO), COLOR_TEXT);

    /* Draw data area with full panel borders */
    draw_board_detail_data(TRUE);
}

/*
 * Update buttons for Board detail view
 */
void board_detail_view_update_buttons(void)
{
 
     static const LocaleStringID display_labels[BOARD_DISPLAY_COUNT] = {
        MSG_BOARD_NAMES, MSG_BOARD_DECIMAL, MSG_BOARD_HEX
    };
    static char counter_str[16];
    snprintf(counter_str, sizeof(counter_str), "%" PRId32 " / %lu",
             board_detail_index + 1, (unsigned long)board_list.count);
    add_button(100, 188, 52, 12,
               get_string(MSG_BTN_PREV), BTN_BOARD_DETAIL_PREV,
               board_detail_index > 0);
    add_button(160, 188, 52, 12,
               counter_str, BTN_BOARD_DETAIL_COUNTER, FALSE);
    add_button(220, 188, 52, 12,
               get_string(MSG_BTN_NEXT), BTN_BOARD_DETAIL_NEXT,
               board_detail_index < (LONG)board_list.count - 1);
    add_button(280, 188, 52, 12,
               get_string(display_labels[app->board_display]),
               BTN_BOARD_DETAIL_DISPLAY, board_list.count > 0);
    add_button(340, 188, 52, 12,
               get_string(MSG_BTN_EXIT), BTN_BOARD_DETAIL_EXIT, TRUE);
}

/*
 * Handle button press for board detail view
 */
void board_detail_view_handle_button(ButtonID id)
{
    switch (id) {
        case BTN_BOARD_DETAIL_PREV:
            if (board_detail_index > 0) {
                board_detail_index--;
                /* Only redraw data area, not the entire screen */
                update_button_states();
                draw_board_detail_data(FALSE);
            }
            break;

        case BTN_BOARD_DETAIL_NEXT:
            if (board_detail_index < (LONG)board_list.count - 1) {
                board_detail_index++;
                /* Only redraw data area, not the entire screen */
                update_button_states();
                draw_board_detail_data(FALSE);
            }
            break;

        case BTN_BOARD_DETAIL_DISPLAY:
            app->board_display = (app->board_display + 1) % BOARD_DISPLAY_COUNT;
            redraw_current_view();
            break;

        case BTN_BOARD_DETAIL_EXIT:
            switch_to_view(VIEW_MAIN);
            break;

        default:
            break;
    }
}
