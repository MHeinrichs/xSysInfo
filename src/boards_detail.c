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
    WORD y = 40;
    BoardInfo *board = (board_list.boards+board_detail_index);
    UBYTE erType = (board->board_type_flags >> 6);
    
    /* Board Address */
    snprintf(buffer, sizeof(buffer), "%s", (unsigned long)board->address_string);
    draw_board_detail_value(y, buffer);
    y += 10;

    /* Total size */
    draw_board_detail_value(y, board->size_string);
    y += 10;

    /* Board type */
    draw_board_detail_value(y, get_board_type_string(board->board_type));
    y += 10;

    /* Product */
    if (app->board_display == BOARD_DISPLAY_NAMES) {
        snprintf(buffer, sizeof(buffer), "%s", board->product_name);
    } else if (app->board_display == BOARD_DISPLAY_HEX) {
        snprintf(buffer, sizeof(buffer),
                board->board_type == BOARD_PCI ? "$%04lX" : "$%02lX",
                (unsigned long)board->product_id);
    } else {
        snprintf(buffer, sizeof(buffer), "%u", board->product_id);
    }    
    draw_board_detail_value(y, buffer);
    y += 10;

    /* Manufactor */
    if (app->board_display == BOARD_DISPLAY_NAMES) {
        snprintf(buffer, sizeof(buffer), "%s", board->manufacturer_name);
    } else if (app->board_display == BOARD_DISPLAY_HEX) {
        snprintf(buffer, sizeof(buffer), "$%04lX",
                 (unsigned long)board->manufacturer_id);
    } else {
        snprintf(buffer, sizeof(buffer), "%u", board->manufacturer_id);
    }
    draw_board_detail_value(y, buffer);
    y += 10;

    /* Serial or PCI class */
    if (board->board_type == BOARD_PCI ||
        app->board_display == BOARD_DISPLAY_NAMES) {
        snprintf(buffer, sizeof(buffer), "%s", board->detail_string);
    } else {
        snprintf(buffer, sizeof(buffer),
                app->board_display == BOARD_DISPLAY_HEX ? "$%08lX" : "%lu",
                (unsigned long)board->serial_number);
    }
    draw_board_detail_value(y, buffer);
    y += 10;

    /* System memory */
    if(board->board_type_flags & (1<<5)){
        snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_YES));
    }
    else{
        snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_NO));
    }
    draw_board_detail_value(y, buffer);
    y += 10;

    /* Is IO Board */
    if(board->board_flags & (1<<7)){
        if(erType == 2){
            snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_BOARD_Z3_MEM_DEVICE));
        }
        else{
            snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_BOARD_Z2_MEM));
        }
    }
    else{
        if(erType == 2){
            snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_BOARD_Z3_IO_DEVICE));
        }
        else{
            snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_BOARD_Z2_ANY));
        }
    }
    draw_board_detail_value(y, buffer);
    y += 10;

    /* Has rom */
    if(board->board_type_flags & (1<<4)){
        snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_YES));
    }
    else{
        snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_NO));
    }
    draw_board_detail_value(y, buffer);
    y += 10;

    if(board->board_type_flags & (1<<4)){
        snprintf(buffer, sizeof(buffer), "$%04X", board->rom_vector);
    }
    else{
        snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_NA));
    }
    draw_board_detail_value(y, buffer);
    y += 10;

    /* Is next board related */
    if(board->board_type_flags & (1<<3)){
        snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_YES));
    }
    else{
        snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_NO));
    }
    draw_board_detail_value(y, buffer);
    y += 10;

    /* can shut up */
    if(board->board_flags & (1<<6)){
        snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_NO));
    }
    else{
        snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_YES));
    }
    draw_board_detail_value(y, buffer);
    y += 10;

    /* Is Z3 (from flags)*/
    if(board->board_flags & (1<<4)){
        snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_YES));
    }
    else{
        snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_NO));
    }
    draw_board_detail_value(y, buffer);
    y += 10;

    /* size extension (Z3)*/
    if(erType == 2){
        if(board->board_flags & (1<<5)){
            snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_YES));
        }
        else{
            snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_NO));
        }
    }
    else{
        snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_NA));
    }
    draw_board_detail_value(y, buffer);
    y += 10;

    if(erType == 2){
        UBYTE size = (board->board_type_flags & 0x7);
        switch(size){
            case 0:
                snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_BOARD_LOGIC_MATCH));
                break;
            case 1:
                snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_BOARD_AUTO_SIZE));
                break;
            case 2:
                snprintf(buffer, sizeof(buffer), "64KB");
                break;
            case 3:
                snprintf(buffer, sizeof(buffer), "128KB");
                break;
            case 4:
                snprintf(buffer, sizeof(buffer), "256KB");
                break;
            case 5:
                snprintf(buffer, sizeof(buffer), "512KB");
                break;
            case 6:
                snprintf(buffer, sizeof(buffer), "1MB");
                break;
            case 7:
                snprintf(buffer, sizeof(buffer), "2MB");
                break;
            case 8:
                snprintf(buffer, sizeof(buffer), "4MB");
                break;
            case 9:
                snprintf(buffer, sizeof(buffer), "6MB");
                break;
            case 10:
                snprintf(buffer, sizeof(buffer), "8MB");
                break;
            case 11:
                snprintf(buffer, sizeof(buffer), "10MB");
                break;
            case 12:
                snprintf(buffer, sizeof(buffer), "12MB");
                break;
            case 13:
                snprintf(buffer, sizeof(buffer), "14MB");
                break;
            case 14:
            case 15:
            default:
                snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_BOARD_RESERVED));
                break;
        }
        buffer[0];
    }
    else{
        snprintf(buffer, sizeof(buffer), "%s", get_string(MSG_NA));
    }
    draw_board_detail_value(y, buffer);
    y += 10;
    
}

/*
 * Draw board detail data area (info panel and navigation buttons - no title)
 */
static void draw_board_detail_data(BOOL full_redraw)
{
    struct RastPort *rp = app->rp;
    char buffer[4];
    WORD y = 40;

    if (full_redraw) {
        /* Draw memory info panel with 3D border */
        draw_panel(100, 28, 520, 158, NULL);
    } else {
        draw_board_detail_values();
        draw_board_detail_buttons();
        return;
    }

    //now draw the fix text values

    /* Board Address */
    draw_label_value(128, y, get_string(MSG_BOARD_ADDRESS), buffer, 168);
    y += 10;

    /* Total size */
    draw_label_value(128, y, get_string(MSG_BOARD_SIZE), buffer, 168);
    y += 10;

    /* Board type */
    draw_label_value_max(128, y, get_string(MSG_BOARD_TYPE), buffer, 168, 618);
    y += 10;

    /* Product */
    draw_label_value(128, y, get_string(MSG_PRODUCT), buffer, 168);
    y += 10;

    /* Manufactor */
    draw_label_value(128, y, get_string(MSG_MANUFACTURER), buffer, 168);
    y += 10;

    /* Serial or PCI class */
    draw_label_value(128, y, get_string(MSG_SERIAL_NO), buffer, 168);
    y += 10;

    /* System memory */
    draw_label_value(128, y, get_string(MSG_BOARD_SYS_MEM), buffer, 168);
    y += 10;

    /* is an IO board*/
    draw_label_value(128, y, get_string(MSG_BOARD_IO), buffer, 168);
    y += 10;

    /* Has rom */
    draw_label_value(128, y, get_string(MSG_BOARD_ROM), buffer, 168);
    y += 10;

    /* ROM vector offset */
    draw_label_value(128, y, get_string(MSG_BOARD_ROM_VECTOR), buffer, 168);
    y += 10;

    /* is next board related*/
    draw_label_value(128, y, get_string(MSG_BOARD_NEXT_RELATED), buffer, 168);
    y += 10;

    /* Can shut up */
    draw_label_value(128, y, get_string(MSG_BOARD_CAN_SHUTUP), buffer, 168);
    y += 10;

    /* type from flags */
    draw_label_value(128, y, get_string(MSG_BOARD_TYPE_FLAG), buffer, 168);
    y += 10;

    /* uses size extension */
    draw_label_value(128, y, get_string(MSG_BOARD_SIZE_EXTENSION), buffer, 168);
    y += 10;

    /* Sub size */
    draw_label_value(128, y, get_string(MSG_BOARD_SUB_SIZE), buffer, 168);
    y += 10;

    draw_board_detail_values(); //fill the values

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
