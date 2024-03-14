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
#include "chry_readline.h"

#ifndef CONFIG_SHELL_DEBUG
#define CONFIG_SHELL_DEBUG 0
#endif

#ifndef CONFIG_SHELL_MAXLEN_PATH
#define CONFIG_SHELL_MAXLEN_PATH 64
#endif

#ifndef CONFIG_SHELL_MAX_USER
#define CONFIG_SHELL_MAX_USER 1
#endif

#ifndef CONFIG_SHELL_MAX_ARG
#define CONFIG_SHELL_MAX_ARG 8
#endif

#ifndef CONFIG_SHELL_LNBUFF_STATIC
#define CONFIG_SHELL_LNBUFF_STATIC 1
#endif

#ifndef CONFIG_SHELL_LNBUFF_SIZE
#define CONFIG_SHELL_LNBUFF_SIZE 128
#endif

#ifndef CONFIG_SHELL_EXEC_TASK
#define CONFIG_SHELL_EXEC_TASK 1
#endif

#ifndef CONFIG_SHELL_MULTI_THREAD
#define CONFIG_SHELL_MULTI_THREAD 0
#endif

#ifndef CONFIG_SHELL_SIGNAL_HANDLER
#define CONFIG_SHELL_SIGNAL_HANDLER 0
#endif

#ifndef CONFIG_SHELL_SVC_RAISE
#define CONFIG_SHELL_SVC_RAISE 0x8001
#endif

#if CONFIG_SHELL_MAXLEN_PATH && CONFIG_SHELL_MAXLEN_PATH
#if CONFIG_SHELL_MAXLEN_PATH > 255
#error "CONFIG_SHELL_MAXLEN_PATH cannot be greater than 255."
#endif
#endif

#define CSH_S_ISBLK(m)  (((m) & _CSH_IFMT) == _CSH_IFBLK)
#define CSH_S_ISCHR(m)  (((m) & _CSH_IFMT) == _CSH_IFCHR)
#define CSH_S_ISDIR(m)  (((m) & _CSH_IFMT) == _CSH_IFDIR)
#define CSH_S_ISFIFO(m) (((m) & _CSH_IFMT) == _CSH_IFIFO)
#define CSH_S_ISREG(m)  (((m) & _CSH_IFMT) == _CSH_IFREG)
#define CSH_S_ISLNK(m)  (((m) & _CSH_IFMT) == _CSH_IFLNK)
#define CSH_S_ISSOCK(m) (((m) & _CSH_IFMT) == _CSH_IFSOCK)

#define _CSH_IFMT   0170000 /* type of file */
#define _CSH_IFDIR  0040000 /* directory */
#define _CSH_IFCHR  0020000 /* character special */
#define _CSH_IFBLK  0060000 /* block special */
#define _CSH_IFREG  0100000 /* regular */
#define _CSH_IFLNK  0120000 /* symbolic link */
#define _CSH_IFSOCK 0140000 /* socket */
#define _CSH_IFIFO  0010000 /* fifo */

#define CSH_S_ISUID 0004000 /*!< (reserved) set user id on execution */
#define CSH_S_ISGID 0002000 /*!< (reserved) set group id on execution */
#define CSH_S_ISVTX 0001000 /*!< (reserved) save swapped text even after use */
#define CSH_S_IRWXU (CSH_S_IRUSR | CSH_S_IWUSR | CSH_S_IXUSR)
#define CSH_S_IRUSR 0000400 /*!< read permission, owner */
#define CSH_S_IWUSR 0000200 /*!< write permission, owner */
#define CSH_S_IXUSR 0000100 /*!< execute/search permission, owner */
#define CSH_S_IRWXG (CSH_S_IRGRP | CSH_S_IWGRP | CSH_S_IXGRP)
#define CSH_S_IRGRP 0000040 /*!< (reserved) read permission, group */
#define CSH_S_IWGRP 0000020 /*!< (reserved) write permission, grougroup */
#define CSH_S_IXGRP 0000010 /*!< (reserved) execute/search permission, group */
#define CSH_S_IRWXO (CSH_S_IROTH | CSH_S_IWOTH | CSH_S_IXOTH)
#define CSH_S_IROTH 0000004 /*!< read permission, other */
#define CSH_S_IWOTH 0000002 /*!< write permission, other */
#define CSH_S_IXOTH 0000001 /*!< execute/search permission, other */

typedef int (*chry_syscall_func_t)(int argc, const char *const *argv);

typedef struct {
    const char *path;
    const char *name;
    chry_syscall_func_t func;
    uint32_t mode;
} chry_syscall_t;

typedef struct {
    const char *name;
    void *var;
    uint32_t type;
} chry_sysvar_t;

typedef struct {
    uint8_t exec;
    uint8_t rsvd1;
    uint16_t rsvd2;

    /*!< readline section */
#if defined(CONFIG_SHELL_LNBUFF_STATIC) && CONFIG_SHELL_LNBUFF_STATIC
    chry_readline_t rl; /*!< readline instance */
    char *linebuff;     /*!< readline buffer */
    uint16_t buffsize;  /*!< readline buffer size */
    uint16_t linesize;  /*!< readline size */
#else
    chry_readline_t rl; /*!< readline instance */
#endif

    /*!< commmand table section */
    const chry_syscall_t *cmd_tbl_beg; /*!< command table begin */
    const chry_syscall_t *cmd_tbl_end; /*!< command table end */

    /*!< variable table section */
    const chry_sysvar_t *var_tbl_beg; /*!< variable table begin */
    const chry_sysvar_t *var_tbl_end; /*!< variable table end */

    /*!< execute section */
#if defined(CONFIG_SHELL_EXEC_TASK) && CONFIG_SHELL_EXEC_TASK
    int exec_code;                             /*!< exec return code */
    int exec_argc;                             /*!< exec argument count */
    char *exec_argv[CONFIG_SHELL_MAX_ARG + 3]; /*!< exec argument value */
#else
    int exec_code; /*!< exec return code    */
    /*!< (on stack)     exec argument count */
    /*!< (on stack)     exec argument value */
#endif

    /*!< user host and path section */
#if defined(CONFIG_SHELL_MAXLEN_PATH) && CONFIG_SHELL_MAXLEN_PATH
    int uid;                                 /*!< now user id */
    const char *host;                        /*!< host name   */
    const char *user[CONFIG_SHELL_MAX_USER]; /*!< user name   */
    char path[CONFIG_SHELL_MAXLEN_PATH];     /*!< path        */
#else
    int uid;                                 /*!< now user id */
    const char *host;                        /*!< host name   */
    const char *user[CONFIG_SHELL_MAX_USER]; /*!< user name   */
    const char *path;                        /*!< path        */
#endif

#if !defined(CONFIG_SHELL_MULTI_THREAD) || (CONFIG_SHELL_MULTI_THREAD == 0)
#if defined(CONFIG_SHELL_EXEC_TASK) && CONFIG_SHELL_EXEC_TASK
    jmp_buf env;
#endif
#endif

    /*!< reserved section */
    void *data;      /*!< frame data */
    void *user_data; /*!< user data  */
} chry_shell_t;

typedef void (*chry_sighandler_t)(chry_shell_t *csh, int signum);

#define CSH_SIG_DFL ((chry_sighandler_t)0)  /* Default action */
#define CSH_SIG_IGN ((chry_sighandler_t)1)  /* Ignore action */
#define CSH_SIG_ERR ((chry_sighandler_t)-1) /* Error return */

typedef struct {
    /*!< I/O section */
    uint16_t (*sput)(chry_readline_t *rl, void *, uint16_t); /*!< output callback */
    uint16_t (*sget)(chry_readline_t *rl, void *, uint16_t); /*!< input callback  */

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
#if defined(CONFIG_SHELL_LNBUFF_STATIC) && CONFIG_SHELL_LNBUFF_STATIC
    char *line_buffer;         /*!< line buffer */
    uint32_t line_buffer_size; /*!< line buffer size */
#endif

    /*!< user host section */
    int uid;                                 /*!< default user id */
    const char *host;                        /*!< host name */
    const char *user[CONFIG_SHELL_MAX_USER]; /*!< user name */

    /*!< reserved section */
    void *user_data;
} chry_shell_init_t;

int chry_shell_init(chry_shell_t *csh, const chry_shell_init_t *init);
int chry_shell_parse(char *line, uint32_t linesize, char **argv);
int chry_shell_task_repl(chry_shell_t *csh);
void chry_shell_task_exec(chry_shell_t *csh);

int chry_shell_set_host(chry_shell_t *csh, const char *host);
int chry_shell_set_user(chry_shell_t *csh, uint8_t uid, const char *user);
int chry_shell_set_path(chry_shell_t *csh, uint8_t size, const char *path);
int chry_shell_get_path(chry_shell_t *csh, uint8_t size, char *path);
int chry_shell_substitute_user(chry_shell_t *csh, uint8_t uid);

chry_sighandler_t chry_shell_signal(chry_shell_t *csh, int signum, chry_sighandler_t handler);
int chry_shell_raise(chry_shell_t *csh, int signum);

#ifdef __cplusplus
}
#endif

#endif
