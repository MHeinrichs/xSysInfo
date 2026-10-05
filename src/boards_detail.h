// SPDX-License-Identifier: BSD-2-Clause
// SPDX-FileCopyrightText: 202 Matthias Heinrichs

/*
 * xSysInfo -Board detail header
 */

#ifndef BOARD_DETAIL_H
#define BOARD_DETAIL_H

#include "xsysinfo.h"


/* Draw board detail view */
void draw_board_detail_view(void);

/*
 * Handle button press for board detail view
 */
void board_detail_view_handle_button(ButtonID id);

/*
 * Update buttons for Board detail view
 */
void board_detail_view_update_buttons(void);


#endif /* BOARD_DETAIL_H */