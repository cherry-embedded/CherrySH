/*
 * Copyright (c) 2025, Egahp
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "csh.h"

static int clear(int argc, char **argv)
{
    chry_shell_t *csh = (void *)argv[argc + 1];
    csh_printf(csh, "\e[2J\e[H");

    return 0;
}

CSH_SCMD_EXPORT_FULL(
    clear,
    "clear the terminal screen",
    "clear\r\n"
    "    -  clear the terminal screen\r\n")
