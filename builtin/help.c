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

static void print_cmdline(chry_shell_t *csh, const chry_syscall_t *call, uint32_t longest_name, uint32_t longest_path)
{
    uint16_t len = strlen(call->name);

    csh_printf(csh, "  \e[32m%s\e[m", call->name);

    for (int k = len; k < longest_name; k++) {
        csh_printf(csh, " ");
    }

    csh_printf(csh, " -> ");

    csh_printf(csh, "\e[36m%s\e[m", call->path);

    if (call->usage) {
        csh_printf(csh, " - ");

        len = strlen(call->path);

        for (int k = len; k < longest_path; k++) {
            csh_printf(csh, " ");
        }

        csh_printf(csh, call->usage);
    }

    csh_printf(csh, "\r\n");
}

/*!< help    */
static int help(int argc, char **argv)
{
    chry_shell_t *csh = (void *)argv[argc + 1];
    uint16_t longest_name;
    uint16_t longest_path;
    bool enable_function = true;
    bool enable_variable = true;

    if (argc > 1) {
        for (const chry_syscall_t *call = csh->cmd_tbl_beg; call < csh->cmd_tbl_end; call++) {
            if (!strcmp(argv[1], call->name)) {
                print_cmdline(csh, call, 0, 0);
                csh_printf(csh, "\r\nUsage:\r\n\r\n");
                if (call->help) {
                    csh->rl.sput(&csh->rl, call->help, strlen(call->help));
                    csh_printf(csh, "\r\n");
                } else {
                    csh_printf(csh, "    help message not provided\r\n");
                }
                return 0;
            }
        }

        csh_printf(csh, "%s: command not found\r\n", argv[1]);
        return -1;
    }

    if (enable_function) {
        csh_printf(csh, "total function %d\r\n",
                   ((uintptr_t)csh->cmd_tbl_end - (uintptr_t)csh->cmd_tbl_beg) / sizeof(chry_syscall_t));
    }

    longest_name = 0;
    longest_path = 0;
    for (const chry_syscall_t *call = csh->cmd_tbl_beg; call < csh->cmd_tbl_end; call++) {
        uint16_t len = strlen(call->name);
        longest_name = len > longest_name ? len : longest_name;
        len = strlen(call->path);
        longest_path = len > longest_path ? len : longest_path;
    }

    for (const chry_syscall_t *call = csh->cmd_tbl_beg; call < csh->cmd_tbl_end; call++) {
        print_cmdline(csh, call, longest_name, longest_path);
    }

    if (enable_variable) {
        if (enable_function) {
            csh_printf(csh, "\r\n");
        }

        csh_printf(csh, "total variable %d\r\n",
                   ((uintptr_t)csh->var_tbl_end - (uintptr_t)csh->var_tbl_beg) / sizeof(chry_sysvar_t));
    }

    longest_name = 0;
    for (const chry_sysvar_t *var = csh->var_tbl_beg; var < csh->var_tbl_end; var++) {
        uint16_t len = strlen(var->name);
        longest_name = len > longest_name ? len : longest_name;
    }

    for (const chry_sysvar_t *var = csh->var_tbl_beg; var < csh->var_tbl_end; var++) {
        uint16_t len = strlen(var->name);

        csh_printf(csh, "  $\e[33m%s\e[m", var->name);

        for (int k = len; k < longest_name; k++) {
            csh_printf(csh, " ");
        }

        char cr = var->attr & CSH_VAR_READ ? 'r' : '-';
        char cw = var->attr & CSH_VAR_WRITE ? 'w' : '-';
        csh_printf(csh, " %c%c %3d\r\n", cr, cw, var->attr & CSH_VAR_SIZE);
    }

    return 0;
}

CSH_SCMD_EXPORT_FULL(
    help,
    "display command usage",
    "help\r\n"
    "    - show a brief overview of all commands\r\n"
    "help <command>\r\n"
    "    - display detailed information for the specified 'command'\r\n")
