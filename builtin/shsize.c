/*
 * Copyright (c) 2022, Egahp
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "csh.h"

/*!< shsize */
static int shsize(int argc, char **argv)
{
    chry_shell_t *csh = (void *)argv[argc + 1];

    if ((argc == 2) && !strcmp(argv[1], "update")) {
        if (5 != csh->rl.sput(&csh->rl, "\e[18t", 5)) {
            csh_printf(csh, "Error: Failed to request window size update\r\n");
            return -1;
        }
        csh_printf(csh, "Requested to update window size successfully\r\n");
        return 0;
    } else if ((argc == 4) && !strcmp(argv[1], "config")) {
        int row = atoi(argv[2]);
        int col = atoi(argv[3]);
        if (row > 10 && row < 4096) {
            csh->rl.term.row = row;
            csh_printf(csh, "Set terminal row to %d\r\n", row);
        } else {
            csh_printf(csh, "Error: Illegal row value %d. Must be between 11 and 4095\r\n", row);
        }
        if (col > 10 && col < 4096) {
            csh->rl.term.col = col;
            csh_printf(csh, "Set terminal column to %d\r\n", col);
        } else {
            csh_printf(csh, "Error: Illegal column value %d. Must be between 11 and 4095\r\n", col);
        }
    } else if (((argc == 2) && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) || (argc == 3) || (argc > 4)) {
        CSH_CALL_HELP("shsize");
        return -1;
    }

    csh_printf(csh, "Current terminal window size: Rows: %d, Columns: %d\r\n", csh->rl.term.row, csh->rl.term.col);

    return 0;
}

CSH_SCMD_EXPORT_FULL(
    shsize,
    "get or set the terminal window size",
    "shsize\r\n"
    "    - show current terminal window size\r\n"
    "shsize udpate\r\n"
    "    - request to update the window size to match the terminal\r\n"
    "shsize config <row> <col>\r\n"
    "    - force configuration of terminal window size to specified rows and columns\r\n"
    "    - row and column values must be between 11 and 4095\r\n"
    "shsize -h\r\n"
    "shsize --help\r\n"
    "    - show usage and help information\r\n");
