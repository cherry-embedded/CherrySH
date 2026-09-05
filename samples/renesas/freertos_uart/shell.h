/*
 * Copyright (c) 2022, Egahp
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_H
#define SHELL_H

#include "hal_data.h"
#include "csh.h"

extern int shell_init(const uart_instance_t *shell_uart, bool need_login);
extern void cherrysh_uart_callback(uart_callback_args_t *p_args);
extern void shell_lock(void);
extern void shell_unlock(void);

#endif
