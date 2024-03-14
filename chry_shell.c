/*
 * Copyright (c) 2022, Egahp
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include "chry_shell.h"

#ifndef __unused
#define __unused
#endif

#if defined(CONFIG_SHELL_DEBUG) && CONFIG_SHELL_DEBUG

#define CHRY_SHELL_PARAM_CHECK(__expr, __ret) \
    do {                                      \
        if (!(__expr)) {                      \
            return __ret;                     \
        }                                     \
    } while (0)

#else

#define CHRY_SHELL_PARAM_CHECK(__expr, __ret) ((void)0)

#endif

#define CSH_PROMPT_SEG_USER 0
#define CSH_PROMPT_SEG_HOST 2
#define CSH_PROMPT_SEG_PATH 4

#define CSH_STATUS_EXEC_IDLE 0
#define CSH_STATUS_EXEC_FIND 1
#define CSH_STATUS_EXEC_PREP 2
#define CSH_STATUS_EXEC_EXEC 3

#define CSH_SIGNAL_COUNT 7

#define chry_shell_offsetof(type, member)          ((size_t) & (((type *)0)->member))
#define chry_shell_container_of(ptr, type, member) ((type *)((char *)(ptr)-chry_shell_offsetof(type, member)))

static void chry_shell_default_handler(chry_shell_t *csh, int sig);
extern int chry_shell_svc_port(int id, chry_shell_t *csh, void *arg1, void *arg2, void *arg3, void *arg4);

static const uint8_t sigmap[CSH_SIGNAL_COUNT] = { SIGINT, SIGQUIT, SIGKILL, SIGTERM, SIGSTOP, SIGTSTP, SIGCONT };

#if !defined(CONFIG_SHELL_SIGNAL_HANDLER) || (CONFIG_SHELL_SIGNAL_HANDLER == 0)
static chry_sighandler_t sighdl[CSH_SIGNAL_COUNT] = {
    chry_shell_default_handler,
    chry_shell_default_handler,
    chry_shell_default_handler,
    chry_shell_default_handler,
    chry_shell_default_handler,
    chry_shell_default_handler,
    chry_shell_default_handler,
};
#endif

/*****************************************************************************
* @brief        default signal handler
* 
* @param[in]    signum      signal number
*
*****************************************************************************/
static void chry_shell_default_handler(chry_shell_t *csh, int sig)
{
    switch (sig) {
        case SIGINT:
            break;
        case SIGQUIT:
            break;
        case SIGKILL:
            break;
        case SIGTERM:
            break;
        case SIGSTOP:
            break;
        case SIGTSTP:
            break;
        case SIGCONT:
            break;
        default:
            break;
    }
}

/*****************************************************************************
* @brief        conversion signum
* 
* @param[in]    signum      signal number
*
* @retval                   0<= signal index -1:not find
*****************************************************************************/
static int chry_shell_conversion_signum(int signum)
{
    int sigidx = -1;

    for (uint8_t i = 0; i < CSH_SIGNAL_COUNT; i++) {
        if (sigmap[i] == signum) {
            sigidx = i;
            break;
        }
    }

    return sigidx;
}

/*****************************************************************************
* @brief        completion callback
* 
* @param[in]    rl          readline instance
* @param[in]    pre         pre string
* @param[in]    size        strlen(pre)
* @param[in]    plist       list of completion
*
* @retval                   list count
*****************************************************************************/
static uint16_t chry_shell_completion_callback(chry_readline_t *rl, char *pre, uint16_t size, const char **plist[])
{
    static const char *list[64];
    uint16_t count = 0;
    chry_shell_t *csh = chry_shell_container_of(rl, chry_shell_t, rl);

    for (const chry_syscall_t *call = csh->cmd_tbl_beg; call < csh->cmd_tbl_end; call++) {
        if (strncmp(pre, call->name, size) == 0) {
            list[count++] = call->name;
            if (count >= sizeof(list) / sizeof(char *)) {
                break;
            }
        }
    }

    *plist = list;
    return count;
}

/*****************************************************************************
* @brief        user callback
* 
* @param[in]    rl          readline instance
* @param[in]    exec        exec code
*
* @retval                   status
*****************************************************************************/
static int chry_shell_user_callback(chry_readline_t *rl, uint8_t exec)
{
    chry_shell_t *csh = chry_shell_container_of(rl, chry_shell_t, rl);
    volatile uint8_t *pexec = (volatile uint8_t *)&csh->exec;
    volatile int *pcode = (volatile int *)&csh->exec_code;

    /*!< user event callback will not output newline automatically */

    switch (exec) {
        case CHRY_READLINE_EXEC_EOF:
            chry_readline_newline(rl);
            break;
        case CHRY_READLINE_EXEC_SIGINT:
            chry_shell_raise(csh, SIGINT);

            *pcode = -1;
            *pexec = CSH_STATUS_EXEC_IDLE;
            rl->sput(rl, "^SIGINT" CONFIG_READLINE_NEWLINE, sizeof("^SIGINT" CONFIG_READLINE_NEWLINE) - 1);
            chry_readline_auto_refresh(rl, true);
            chry_readline_edit_refresh(rl);
            chry_readline_ignore(rl, false);
            break;
        case CHRY_READLINE_EXEC_SIGQUIT:
            *pcode = -1;
            *pexec = CSH_STATUS_EXEC_IDLE;
            rl->sput(rl, "^SIGQUIT" CONFIG_READLINE_NEWLINE, sizeof("^SIGQUIT" CONFIG_READLINE_NEWLINE) - 1);
            chry_readline_auto_refresh(rl, true);
            chry_readline_edit_refresh(rl);
            chry_readline_ignore(rl, false);
            break;
        case CHRY_READLINE_EXEC_SIGCONT:
            return 1;
            break;
        case CHRY_READLINE_EXEC_SIGSTOP:
            return 1;
            break;
        case CHRY_READLINE_EXEC_SIGTSTP:
            return 1;
            break;
        case CHRY_READLINE_EXEC_F1 ... CHRY_READLINE_EXEC_F12:
            chry_readline_newline(rl);
            break;
        case CHRY_READLINE_EXEC_USER ... CHRY_READLINE_EXEC_END:
            chry_readline_newline(rl);
            break;
        default:
            return -1;
    }

    /*!< return 1 will not refresh */
    /*!< return 0 to refresh whole line (include prompt) */
    /*!< return -1 to end readline (error) */
    return 0;
}

/*****************************************************************************
* @brief        init shell
* 
* @param[in]    csh         shell instance
* @param[in]    init        init config
*
* @retval                   0:Success -1:Error
*****************************************************************************/
int chry_shell_init(chry_shell_t *csh, const chry_shell_init_t *init)
{
    CHRY_SHELL_PARAM_CHECK(NULL != csh, -1);
    CHRY_SHELL_PARAM_CHECK(NULL != init, -1);
    CHRY_SHELL_PARAM_CHECK(NULL != init->sget, -1);
    CHRY_SHELL_PARAM_CHECK(NULL != init->sput, -1);
    CHRY_SHELL_PARAM_CHECK(NULL != init->prompt_buffer, -1);

#if defined(CONFIG_READLINE_HISTORY) && CONFIG_READLINE_HISTORY
    CHRY_SHELL_PARAM_CHECK(NULL != init->history_buffer, -1);
#endif

#if defined(CONFIG_SHELL_LNBUFF_STATIC) && CONFIG_SHELL_LNBUFF_STATIC
    CHRY_SHELL_PARAM_CHECK(NULL != init->line_buffer, -1);
#endif

    int ret = 0;
    chry_readline_init_t rl_init;

    /*!< do readline init */
    rl_init.prompt = init->prompt_buffer;
    rl_init.pptsize = init->prompt_buffer_size;
    rl_init.history = init->history_buffer;
    rl_init.histsize = init->history_buffer_size;
    rl_init.sput = init->sput;
    rl_init.sget = init->sget;

    if (chry_readline_init(&csh->rl, &rl_init)) {
        return -1;
    }

    csh->cmd_tbl_beg = init->command_table_beg;
    csh->cmd_tbl_end = init->command_table_end;
    csh->var_tbl_beg = init->variable_table_beg;
    csh->var_tbl_end = init->variable_table_end;

#if defined(CONFIG_SHELL_LNBUFF_STATIC) && CONFIG_SHELL_LNBUFF_STATIC
    csh->linebuff = init->line_buffer;
    csh->buffsize = init->line_buffer_size;
    csh->linesize = 0;
#endif

    csh->exec_code = 0;

    /*!< set default user id */
    if ((init->uid >= CONFIG_SHELL_MAX_USER) || (init->uid < 0)) {
        return -1;
    }
    csh->uid = init->uid;

    /*!< set host name */
    if (strnlen(init->host, 255) == 255) {
        return -1;
    }
    csh->host = init->host;

    /*!< set users name */
    for (uint8_t i = 0; i < CONFIG_SHELL_MAX_USER; i++) {
        if (strnlen(init->user[i], 255) == 255) {
            return -1;
        }
        csh->user[i] = init->user[i];
    }

    /*!< set default path */
#if defined(CONFIG_SHELL_MAXLEN_PATH) && CONFIG_SHELL_MAXLEN_PATH
    csh->path[0] = '/';
    csh->path[1] = '\0';
#else
    csh->path = "/";
#endif

    csh->exec = CSH_STATUS_EXEC_IDLE;

    csh->data = NULL;
    csh->user_data = init->user_data;

    chry_readline_set_completion_cb(&csh->rl, chry_shell_completion_callback);
    chry_readline_set_user_cb(&csh->rl, chry_shell_user_callback);

    ret |= chry_readline_prompt_edit(&csh->rl, CSH_PROMPT_SEG_USER, (chry_readline_sgr_t){ .foreground = CHRY_READLINE_SGR_GREEN, .bold = 1 }.raw, csh->user[csh->uid]);
    ret |= chry_readline_prompt_edit(&csh->rl, CSH_PROMPT_SEG_USER + 1, (chry_readline_sgr_t){ .foreground = CHRY_READLINE_SGR_GREEN, .bold = 1 }.raw, "@");
    ret |= chry_readline_prompt_edit(&csh->rl, CSH_PROMPT_SEG_HOST, (chry_readline_sgr_t){ .foreground = CHRY_READLINE_SGR_GREEN, .bold = 1 }.raw, csh->host);
    ret |= chry_readline_prompt_edit(&csh->rl, CSH_PROMPT_SEG_HOST + 1, 0, ":");
    ret |= chry_readline_prompt_edit(&csh->rl, CSH_PROMPT_SEG_PATH, (chry_readline_sgr_t){ .foreground = CHRY_READLINE_SGR_BLUE, .bold = 1 }.raw, csh->path);
    ret |= chry_readline_prompt_edit(&csh->rl, CSH_PROMPT_SEG_PATH + 1, 0, "$ ");

    return ret;
}

/*****************************************************************************
* @brief        execute task internal
* 
* @param[in]    csh         shell instance
*
*****************************************************************************/
static void chry_shell_task_exec_internal(chry_shell_t *csh, int argc, char **argv)
{
    volatile uint8_t *pexec = (volatile uint8_t *)&csh->exec;
    volatile int *pcode = (volatile int *)&csh->exec_code;

    /*!< if stage find */
    if (*pexec == CSH_STATUS_EXEC_FIND) {
        /*!< stage prepare */
        *pexec = CSH_STATUS_EXEC_PREP;

#if defined(CONFIG_SHELL_EXEC_TASK) && CONFIG_SHELL_EXEC_TASK && \
    (!defined(CONFIG_SHELL_MULTI_THREAD) || (CONFIG_SHELL_MULTI_THREAD == 0))
        int ret = setjmp(csh->env);

        if (0 == ret) {
            /*!< stage execute */
            *pexec = CSH_STATUS_EXEC_EXEC;
            *pcode = ((chry_syscall_func_t)argv[argc + 2])(argc, (void *)argv);
        } else {
            int sigidx = chry_shell_conversion_signum(ret);

            if (sigidx == -1) {
                /*!< if not found, call default handler */
                chry_shell_default_handler(csh, -1);
            } else {
                /*!< call handler */
                sighdl[sigidx](csh, sigidx);
            }
        }

        /*!< reset signal handler */
        for (uint8_t i = 0; i < CSH_SIGNAL_COUNT; i++) {
            sighdl[i] = chry_shell_default_handler;
        }

        /*!< TODO should call refresh in SVC */

#else
        *pcode = ((chry_syscall_func_t)argv[argc + 2])(argc, (void *)argv);
#endif

        /*!< stage idle */
        *pexec = CSH_STATUS_EXEC_IDLE;
    }
}

/*****************************************************************************
* @brief        execute task
* 
* @param[in]    csh         shell instance
*
*****************************************************************************/
void chry_shell_task_exec(chry_shell_t *csh)
{
    CHRY_SHELL_PARAM_CHECK(NULL != csh, );
#if defined(CONFIG_SHELL_EXEC_TASK) && CONFIG_SHELL_EXEC_TASK
    chry_shell_task_exec_internal(csh, csh->exec_argc, csh->exec_argv);
#endif
}

/*****************************************************************************
* @brief        read eval print loop task
* 
* @param[in]    csh         shell instance
*
* @retval                   0:Success -1:Error 1:Continue
*****************************************************************************/
int chry_shell_task_repl(chry_shell_t *csh)
{
    CHRY_SHELL_PARAM_CHECK(NULL != csh, -1);

    char *line = chry_readline(&csh->rl, csh->linebuff, csh->buffsize, &csh->linesize);
    if (line == NULL) {
        return -1;
    } else if (line == (void *)-1) {
        return 1;
    } else if (csh->linesize) {
        int *argc;
        char **argv;

#if defined(CONFIG_SHELL_EXEC_TASK) && CONFIG_SHELL_EXEC_TASK
        argc = &csh->exec_argc;
        argv = &csh->exec_argv[0];
#else
        int csh_exec_argc;
        char *csh_exec_argv[CONFIG_SHELL_MAX_ARG + 3];
        argc = &csh_exec_argc;
        argv = &csh_exec_argv[0];
#endif

        *argc = chry_shell_parse(line, csh->linesize, argv);
        argv[*argc + 1] = (void *)csh;
        argv[*argc + 2] = NULL;

        if (*argc) {
            for (const chry_syscall_t *call = csh->cmd_tbl_beg; call < csh->cmd_tbl_end; call++) {
                if (strcmp(argv[0], call->name) == 0) {
                    argv[*argc + 2] = (void *)call->func;
                }
            }

            if (NULL == argv[*argc + 2]) {
                csh->rl.sput(&csh->rl, argv[0], strlen(argv[0]));
                csh->rl.sput(&csh->rl, ": command not found\r\n", 21);
                csh->exec = CSH_STATUS_EXEC_IDLE;
            } else {
                *((volatile uint8_t *)&csh->exec) = CSH_STATUS_EXEC_FIND;
#if defined(CONFIG_SHELL_EXEC_TASK) && CONFIG_SHELL_EXEC_TASK
                chry_readline_ignore(&csh->rl, true);
                chry_readline_auto_refresh(&csh->rl, false);
#else
                chry_shell_task_exec_internal(csh, *argc, argv);
#endif
            }
        }

        return 0;
    }

    return 0;
}

/*****************************************************************************
* @brief        parse line to argc,argv[]
* 
* @param[in]    line        read line
* @param[in]    linesize    strlen(line)
* @param[out]   argv        argument value
*
* @retval                   argc:argument count, max is CONFIG_SHELL_MAX_ARG
*****************************************************************************/
int chry_shell_parse(char *line, uint32_t linesize, char **argv)
{
    int argc = 0;
    bool ignore = false;
    bool start = false;
    bool escape = false;

    while (linesize) {
        char c = *line;

        if (c == '\0') {
            break;
        } else if ((escape == false) && (c == '\\')) {
            escape = true;
            memmove(line, line + 1, linesize);
            line--;
        } else if ((escape == false) && (ignore == false) && (c == ' ')) {
            start = false;
            *line = '\0';
        } else if ((escape == false) && (c == '"')) {
            memmove(line, line + 1, linesize);
            line--;
            ignore = !ignore;
        } else if (!start) {
            argv[argc] = line;
            if (argc < CONFIG_SHELL_MAX_ARG) {
                argc++;
            } else {
                break;
            }
            start = true;
            escape = false;
        } else {
            escape = false;
        }

        line++;
        linesize--;
    }

    *line = '\0';
    argv[argc] = NULL;

    return argc;
}

/*****************************************************************************
* @brief        set host
* 
* @param[in]    csh         shell instance
* @param[in]    host        pointer of host
*
* @retval                   0:Success -1:Error
*****************************************************************************/
int chry_shell_set_host(chry_shell_t *csh, const char *host)
{
    CHRY_SHELL_PARAM_CHECK(NULL != csh, -1);
    CHRY_SHELL_PARAM_CHECK(NULL != host, -1);

    if (strnlen(host, 255) == 255) {
        return -1;
    }

    csh->host = host;

    return chry_readline_prompt_edit(&csh->rl, CSH_PROMPT_SEG_HOST, (chry_readline_sgr_t){ .foreground = CHRY_READLINE_SGR_GREEN, .bold = 1 }.raw, csh->host);
}

/*****************************************************************************
* @brief        set user
* 
* @param[in]    csh         shell instance
* @param[in]    uid         user id
* @param[in]    user        pointer of user
*
* @retval                   0:Success -1:Error
*****************************************************************************/
int chry_shell_set_user(chry_shell_t *csh, uint8_t uid, const char *user)
{
    CHRY_SHELL_PARAM_CHECK(NULL != csh, -1);
    CHRY_SHELL_PARAM_CHECK(NULL != user, -1);

    if (uid < CONFIG_SHELL_MAX_USER) {
        if (strnlen(user, 255) == 255) {
            return -1;
        }

        csh->user[uid] = user;
        if (uid == csh->uid) {
            return chry_readline_prompt_edit(&csh->rl, CSH_PROMPT_SEG_USER, (chry_readline_sgr_t){ .foreground = CHRY_READLINE_SGR_GREEN, .bold = 1 }.raw, csh->user[uid]);
        }
        return 0;
    }

    return -1;
}

/*****************************************************************************
* @brief        set path
* 
* @param[in]    csh         shell instance
* @param[in]    size        strlen(path) + 1, include \0. 
*                           ignore when CONFIG_SHELL_MAXLEN_PATH = 0
* @param[in]    path        shell instance
*
* @retval                   0:Success -1:Error
*****************************************************************************/
int chry_shell_set_path(chry_shell_t *csh, uint8_t size, const char *path)
{
    CHRY_SHELL_PARAM_CHECK(NULL != csh, -1);
    CHRY_SHELL_PARAM_CHECK(NULL != path, -1);

#if defined(CONFIG_SHELL_MAXLEN_PATH) && CONFIG_SHELL_MAXLEN_PATH
    if (size > CONFIG_SHELL_MAXLEN_PATH) {
        return -1;
    }

    memcpy(csh->path, path, size);
    csh->path[size] = '\0';

#else
    if (strnlen(path, 255) == 255) {
        return -1;
    }
    csh->path = path;
#endif

    return chry_readline_prompt_edit(&csh->rl, CSH_PROMPT_SEG_PATH, (chry_readline_sgr_t){ .foreground = CHRY_READLINE_SGR_BLUE, .bold = 1 }.raw, csh->path);
}

/*****************************************************************************
* @brief        get path
* 
* @param[in]    csh         shell instance
* @param[in]    size        path buffer size 
* @param[in]    path        path buffer
*
* @retval                   0:Success -1:Error
*****************************************************************************/
int chry_shell_get_path(chry_shell_t *csh, uint8_t size, char *path)
{
    CHRY_SHELL_PARAM_CHECK(NULL != csh, -1);
    CHRY_SHELL_PARAM_CHECK(NULL != path, -1);

    int len = strnlen(csh->path, CONFIG_SHELL_MAXLEN_PATH);
    memcpy(path, csh->path, len);
    path[len] = '\0';

    return 0;
}

/*****************************************************************************
* @brief        substitute user
* 
* @param[in]    csh         shell instance
* @param[in]    uid         user id
*
* @retval                   0:Success -1:Error
*****************************************************************************/
int chry_shell_substitute_user(chry_shell_t *csh, uint8_t uid)
{
    CHRY_SHELL_PARAM_CHECK(NULL != csh, -1);

    if (uid < CONFIG_SHELL_MAX_USER) {
        if (uid == csh->uid) {
            return chry_readline_prompt_edit(&csh->rl, CSH_PROMPT_SEG_USER, (chry_readline_sgr_t){ .foreground = CHRY_READLINE_SGR_GREEN, .bold = 1 }.raw, csh->user[uid]);
        }
        return 0;
    }

    return -1;
}

/*****************************************************************************
* @brief        signal
* 
* @param[in]    signum      sig number
*                           SIGINT      2
*                           SIGQUIT     3
*                           SIGKILL     9
*                           SIGTERM     15
*                           SIGSTOP     17
*                           SIGTSTP     18
*                           SIGCONT     19
* @param[in]    handler     new handler
*                           CSH_SIG_DFL     0
*                           CSH_SIG_IGN     1
*
* @retval                   old handler / CSH_SIG_ERR
*****************************************************************************/
chry_sighandler_t chry_shell_signal(chry_shell_t *csh, int signum, chry_sighandler_t handler)
{
    int sigidx = chry_shell_conversion_signum(signum);
    chry_sighandler_t old = CSH_SIG_ERR;

    if (sigidx != -1) {
        old = sighdl[sigidx];

        if (handler == CSH_SIG_DFL) {
            sighdl[sigidx] = chry_shell_default_handler;
        } else {
            sighdl[sigidx] = handler;
        }
    }

    return old;
}

int chry_shell_raise(chry_shell_t *csh, int signum)
{
    return chry_shell_svc_port(CONFIG_SHELL_SVC_RAISE, csh, signum, NULL, NULL, NULL);
}

void chry_shell_psignal(chry_shell_t *csh, int signum, const char *str)
{
}

void chry_shell_abort(chry_shell_t *csh)
{
}

void chry_shell_exit(chry_shell_t *csh, int code)
{
}

int chry_shell_execl(const char *path, const char *arg, ...);
int chry_shell_execlp(const char *file, const char *arg, ...);
int chry_shell_execle(const char *path, const char *arg, ...);
int chry_shell_execv(const char *path, char *const argv[]);
int chry_shell_execvp(const char *file, char *const argv[]);
int chry_shell_execve(const char *path, char *const argv[], char *const envp[]);
