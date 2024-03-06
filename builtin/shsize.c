/*
 * Copyright (c) 2022, Egahp
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "csh.h"

/*!< shsize */
static int shsize(int argc, char **argv)
{
    chry_shell_t *csh = (void *)argv[argc + 1];

    chry_readline_detect(&csh->rl);

    printf("%d %d\r\n", csh->rl.term.row, csh->rl.term.col);

    return 0;
}

CSH_SCMD_EXPORT(shsize, );
