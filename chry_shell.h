/*
 * Copyright (c) 2022, Egahp
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CHRY_SHELL_H
#define CHRY_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <setjmp.h>
#include <signal.h>
#include "CherryRL/chry_readline.h"

/*!< check maxlen */
#if defined(CONFIG_CSH_MAXLEN_PATH) && CONFIG_CSH_MAXLEN_PATH
#if CONFIG_CSH_MAXLEN_PATH > 255
#error "CONFIG_CSH_MAXLEN_PATH cannot be greater than 255."
#endif
#endif

/*!< check exec task and lnbuff static */
#if (defined(CONFIG_CSH_EXEC_TASK) && CONFIG_CSH_EXEC_TASK)
#if !defined(CONFIG_CSH_LNBUFF_STATIC) || (CONFIG_CSH_LNBUFF_STATIC == 0)
#error "CONFIG_CSH_LNBUFF_STATIC and CONFIG_CSH_EXEC_TASK cannot be enabled at the same time."
#endif
#endif

/*!< check exec task and multi-thread */
#if ((defined(CONFIG_CSH_EXEC_TASK) && CONFIG_CSH_EXEC_TASK) &&                \
     (!defined(CONFIG_CSH_MULTI_THREAD) || (CONFIG_CSH_MULTI_THREAD == 0))) || \
    ((defined(CONFIG_CSH_MULTI_THREAD) && CONFIG_CSH_MULTI_THREAD) &&          \
     (!defined(CONFIG_CSH_EXEC_TASK) || (CONFIG_CSH_EXEC_TASK == 0)))
#error "CONFIG_CSH_EXEC_TASK and CONFIG_CSH_MULTI_THREAD must be enabled at the same time."
#endif

#if ((defined(CONFIG_CSH_NOBLOCK) && CONFIG_CSH_NOBLOCK) && \
     (!defined(CONFIG_CSH_LNBUFF_STATIC) || (CONFIG_CSH_LNBUFF_STATIC == 0)))
#error "CONFIG_CSH_LNBUFF_STATIC and CONFIG_CSH_NOBLOCK must be enabled at the same time."
#endif

#define CSH_SIGINT  1
#define CSH_SIGQUIT 3
#define CSH_SIGKILL 9
#define CSH_SIGTERM 15
#define CSH_SIGSTOP 17
#define CSH_SIGTSTP 18
#define CSH_SIGCONT 19

#define CSH_VAR_READ  0x80000000
#define CSH_VAR_WRITE 0x40000000
#define CSH_VAR_SIZE  0x3fffffff

#define CSH_SIG_DFL ((chry_sighandler_t)0)  /* Default action */
#define CSH_SIG_IGN ((chry_sighandler_t)1)  /* Ignore action */
#define CSH_SIG_ERR ((chry_sighandler_t)-1) /* Error return */

typedef int (*chry_syscall_func_t)(int argc, char **argv);

typedef struct {
    const char *path;
    const char *name;
    chry_syscall_func_t func;
} chry_syscall_t;

typedef struct {
    const char *name;
    void *var;
    uint32_t attr;
} chry_sysvar_t;

typedef struct {
    uint32_t exec;

    /*!< readline section */
#if defined(CONFIG_CSH_LNBUFF_STATIC) && CONFIG_CSH_LNBUFF_STATIC
    chry_readline_t rl; /*!< readline instance */
    char *linebuff;     /*!< readline buffer */
    uint16_t buffsize;  /*!< readline buffer size */
    uint16_t linesize;  /*!< readline size */
#else
    chry_readline_t rl; /*!< readline instance */
    /*!< (on stack)          readline buffer */
    /*!< (on stack)          readline buffer size */
    /*!< (on stack)          readline size */
#endif

    /*!< commmand table section */
    const chry_syscall_t *cmd_tbl_beg; /*!< command table begin */
    const chry_syscall_t *cmd_tbl_end; /*!< command table end */

    /*!< variable table section */
    const chry_sysvar_t *var_tbl_beg; /*!< variable table begin */
    const chry_sysvar_t *var_tbl_end; /*!< variable table end */

    /*!< execute section */
#if defined(CONFIG_CSH_EXEC_TASK) && CONFIG_CSH_EXEC_TASK
    int exec_code;                           /*!< exec return code */
    int exec_argc;                           /*!< exec argument count */
    char *exec_argv[CONFIG_CSH_MAX_ARG + 3]; /*!< exec argument value */
#else
    int exec_code; /*!< exec return code    */
    /*!< (on stack)     exec argument count */
    /*!< (on stack)     exec argument value */
#endif

    /*!< user host and path section */
#if defined(CONFIG_CSH_MAXLEN_PATH) && CONFIG_CSH_MAXLEN_PATH
    int uid;                               /*!< now user id        */
    const char *host;                      /*!< host name          */
    const char *user[CONFIG_CSH_MAX_USER]; /*!< user name          */
    const char *hash[CONFIG_CSH_MAX_USER]; /*!< user password hash */
    char path[CONFIG_CSH_MAXLEN_PATH];     /*!< path               */
#else
    int uid;                               /*!< now user id        */
    const char *host;                      /*!< host name          */
    const char *user[CONFIG_CSH_MAX_USER]; /*!< user name          */
    const char *hash[CONFIG_CSH_MAX_USER]; /*!< user password hash */
    const char *path;                      /*!< path               */
#endif

#if (!defined(CONFIG_CSH_MULTI_THREAD) || (CONFIG_CSH_MULTI_THREAD == 0)) && \
    (defined(CONFIG_CSH_EXEC_TASK) && (CONFIG_CSH_EXEC_TASK))
    jmp_buf env;
#endif

    /*!< reserved section */
    void *data;      /*!< frame data */
    void *user_data; /*!< user data  */
} chry_shell_t;

typedef void (*chry_sighandler_t)(chry_shell_t *csh, int signum);

typedef struct {
    /*!< I/O section */
    uint16_t (*sput)(chry_readline_t *rl, const void *, uint16_t); /*!< output callback */
    uint16_t (*sget)(chry_readline_t *rl, void *, uint16_t);       /*!< input callback  */

    /*!< command table section */
    const void *command_table_beg; /*!< static command table begin */
    const void *command_table_end; /*!< static command table end   */

    /*!< variable table section */
    const void *variable_table_beg; /*!< static environment variable begin */
    const void *variable_table_end; /*!< static environment variable end   */

    /*!< prompt buffer setcion */
    char *prompt_buffer;         /*!< prompt buffer      */
    uint16_t prompt_buffer_size; /*!< prompt buffer size */

    /*!< history buffer setcion */
    char *history_buffer;         /*!< history buffer      */
    uint16_t history_buffer_size; /*!< history buffer size */

    /*!< line buffer setcion */
    char *line_buffer;         /*!< line buffer */
    uint32_t line_buffer_size; /*!< line buffer size */

    /*!< user host section */
    int uid;                               /*!< default user id */
    const char *host;                      /*!< host name */
    const char *user[CONFIG_CSH_MAX_USER]; /*!< user name */
    const char *hash[CONFIG_CSH_MAX_USER]; /*!< user password hash */

    /*!< reserved section */
    void *user_data;
} chry_shell_init_t;

int chry_shell_init(chry_shell_t *csh, const chry_shell_init_t *init);
int chry_shell_task_repl(chry_shell_t *csh);
void chry_shell_task_exec(chry_shell_t *csh);

int chry_shell_parse(char *line, uint32_t linesize, const char **argv, uint8_t argcmax);
int chry_shell_path_resolve(const char *cur, const char *path, const char **argv, uint8_t *argl, uint8_t argcmax);

int chry_shell_set_host(chry_shell_t *csh, const char *host);
int chry_shell_set_user(chry_shell_t *csh, uint8_t uid, const char *user, const char *hash);
int chry_shell_set_path(chry_shell_t *csh, uint8_t size, const char *path);
void chry_shell_get_path(chry_shell_t *csh, uint8_t size, char *path);
int chry_shell_substitute_user(chry_shell_t *csh, uint8_t uid, const char *password);

int csh_login(chry_shell_t *csh);

char *chry_shell_getenv(chry_shell_t *csh, const char *name);

#ifdef __cplusplus
}
#endif

#endif
