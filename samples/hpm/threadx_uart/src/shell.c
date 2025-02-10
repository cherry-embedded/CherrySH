#include "tx_api.h"
#include <stdio.h>
#include <stdbool.h>
#include "board.h"
#include "hpm_uart_drv.h"
#include "chry_ringbuffer.h"
#include "csh.h"

#ifndef THREAD_REPL_PRIORITY
#define THREAD_REPL_PRIORITY (TX_MAX_PRIORITIES - 5U)
#endif

#ifndef THREAD_EXEC_PRIORITY
#define THREAD_EXEC_PRIORITY (TX_MAX_PRIORITIES - 4U)
#endif

static chry_shell_t csh;
static UART_Type *shell_uart = NULL;
static volatile bool login = false;
static chry_ringbuffer_t shell_rb;
static uint8_t mempool[1024];

static TX_THREAD thread_buffer_repl;
static TX_THREAD thread_buffer_exec;
static volatile bool thread_exec_created;

static uint32_t thread_stack_repl[1024];
static uint32_t thread_stack_exec[1024];

static TX_EVENT_FLAGS_GROUP event_hdl;

void shell_uart_isr(void)
{
    uint8_t irq_id = uart_get_irq_id(shell_uart);

    if ((irq_id == uart_intr_id_rx_data_avail) || (irq_id == uart_intr_id_rx_timeout)) {
        while (uart_check_status(shell_uart, uart_stat_data_ready)) {
            uint8_t byte = uart_read_byte(shell_uart);
            chry_ringbuffer_write_byte(&shell_rb, byte);
        }

        tx_event_flags_set(&event_hdl, 0x10, TX_OR);
    }
}

static uint16_t csh_sput_cb(chry_readline_t *rl, const void *data, uint16_t size)
{
    uint16_t i;
    (void)rl;
    for (i = 0; i < size; i++) {
        if (status_success != uart_send_byte(shell_uart, ((uint8_t *)data)[i])) {
            break;
        }
    }

    return i;
}

static uint16_t csh_sget_cb(chry_readline_t *rl, void *data, uint16_t size)
{
    (void)rl;
    return chry_ringbuffer_read(&shell_rb, data, size);
}

static void wait_char(void)
{
    ULONG event;

wait:
    tx_event_flags_get(&event_hdl, (0x10 | 0x01 | 0x04), TX_OR_CLEAR, &event, TX_WAIT_FOREVER);
    if ((event & 0x10) == 0) {
        if (event & 0x01) {
            chry_readline_erase_line(&csh.rl);
            tx_event_flags_set(&event_hdl, 0x02, TX_OR);
        }
        if (event & 0x04) {
            chry_readline_edit_refresh(&csh.rl);
            tx_event_flags_set(&event_hdl, 0x08, TX_OR);
        }

        goto wait;
    }
}

static void thread_repl(ULONG param)
{
    (void)param;
    int ret;
    volatile uint8_t *pexec = (void *)&csh.exec;

    for (;;) {
    restart:
        if (login) {
            goto repl;
        } else {
        }

        ret = csh_login(&csh);
        if (ret == 0) {
            login = true;
        } else if (ret == 1) {
            /*!< no enough char */
            wait_char();
            continue;
        } else {
            continue;
        }

    repl:
        ret = chry_shell_task_repl(&csh);

        if (ret == -1) {
            /*!< error */
            goto restart;
        } else if (ret == 1) {
            /*!< no enough char */
            wait_char();
        } else {
            /*!< restart */
        }

        /*!< check flag */
        if (*pexec == CSH_STATUS_EXEC_DONE) {
            *pexec = CSH_STATUS_EXEC_IDLE;
            chry_readline_auto_refresh(&csh.rl, true);
            chry_readline_ignore(&csh.rl, false);
            chry_readline_edit_refresh(&csh.rl);
        }

        if (login == false) {
            chry_readline_erase_line(&csh.rl);
            csh.rl.noblock = false;
        }
    }
}

static void thread_exec(ULONG param)
{
    (void)param;

    /*!< execute shell command */
    chry_shell_task_exec(&csh);

    /*!< notify REPL thread execute done */
    tx_event_flags_set(&event_hdl, 0x10, TX_OR);

    /*!< wait for REPL thread delete */
    tx_thread_suspend(&thread_buffer_exec);
}

int chry_shell_port_create_context(chry_shell_t *csh, int argc, const char **argv)
{
    (void)csh;
    (void)argc;
    (void)argv;

    if (thread_exec_created) {
        if (TX_SUCCESS != tx_thread_terminate(&thread_buffer_exec)) {
            return -2;
        }
        if (TX_SUCCESS != tx_thread_delete(&thread_buffer_exec)) {
            return -3;
        }
        thread_exec_created = false;
    }

    if (TX_SUCCESS != tx_thread_create(
                          &thread_buffer_exec,
                          "csh buffer exec",
                          thread_exec,
                          0, /* param */
                          thread_stack_exec,
                          sizeof(thread_stack_exec),
                          THREAD_EXEC_PRIORITY,
                          THREAD_EXEC_PRIORITY,
                          TX_NO_TIME_SLICE,
                          TX_AUTO_START)) {
        return -1;
    }

    thread_exec_created = true;

    return 0;
}

void chry_shell_port_default_handler(chry_shell_t *csh, int sig)
{
    volatile uint8_t *pexec = (void *)&csh->exec;

    switch (sig) {
        case CSH_SIGINT:
        case CSH_SIGQUIT:
        case CSH_SIGKILL:
        case CSH_SIGTERM:
            break;
        default:
            return;
    }

    /*!< force delete thread */
    if (thread_exec_created) {
        if (TX_SUCCESS != tx_thread_terminate(&thread_buffer_exec)) {
            csh->rl.sput(&csh->rl, "context terminate error" CONFIG_CSH_NEWLINE CONFIG_CSH_NEWLINE,
                         23 + (sizeof(CONFIG_CSH_NEWLINE) ? (sizeof(CONFIG_CSH_NEWLINE) - 1) * 2 : 0));
            return;
        }
        if (TX_SUCCESS != tx_thread_delete(&thread_buffer_exec)) {
            csh->rl.sput(&csh->rl, "context delete error" CONFIG_CSH_NEWLINE CONFIG_CSH_NEWLINE,
                         20 + (sizeof(CONFIG_CSH_NEWLINE) ? (sizeof(CONFIG_CSH_NEWLINE) - 1) * 2 : 0));
            return;
        }
        thread_exec_created = false;
    }

    switch (sig) {
        case CSH_SIGINT:
            csh->rl.sput(&csh->rl, "^SIGINT" CONFIG_CSH_NEWLINE, sizeof("^SIGINT" CONFIG_CSH_NEWLINE) - 1);
            break;
        case CSH_SIGQUIT:
            csh->rl.sput(&csh->rl, "^SIGQUIT" CONFIG_CSH_NEWLINE, sizeof("^SIGQUIT" CONFIG_CSH_NEWLINE) - 1);
            break;
        case CSH_SIGKILL:
            csh->rl.sput(&csh->rl, "^SIGKILL" CONFIG_CSH_NEWLINE, sizeof("^SIGKILL" CONFIG_CSH_NEWLINE) - 1);
            break;
        case CSH_SIGTERM:
            csh->rl.sput(&csh->rl, "^SIGTERM" CONFIG_CSH_NEWLINE, sizeof("^SIGTERM" CONFIG_CSH_NEWLINE) - 1);
            break;
        default:
            return;
    }

    *pexec = CSH_STATUS_EXEC_IDLE;
    chry_readline_auto_refresh(&csh->rl, true);
    chry_readline_ignore(&csh->rl, false);
    chry_readline_edit_refresh(&csh->rl);
}

int shell_init(UART_Type *uart, bool need_login)
{
    chry_shell_init_t csh_init;

    if (uart == NULL) {
        return -1;
    }

    if (chry_ringbuffer_init(&shell_rb, mempool, sizeof(mempool))) {
        return -1;
    }

    if (need_login) {
        login = false;
    } else {
        login = true;
    }

    shell_uart = uart;

    /*!< I/O callback */
    csh_init.sput = csh_sput_cb;
    csh_init.sget = csh_sget_cb;

#if defined(CONFIG_CSH_SYMTAB) && CONFIG_CSH_SYMTAB
    extern const int __fsymtab_start;
    extern const int __fsymtab_end;
    extern const int __vsymtab_start;
    extern const int __vsymtab_end;

    /*!< get table from ld symbol */
    csh_init.command_table_beg = &__fsymtab_start;
    csh_init.command_table_end = &__fsymtab_end;
    csh_init.variable_table_beg = &__vsymtab_start;
    csh_init.variable_table_end = &__vsymtab_end;
#endif

#if defined(CONFIG_CSH_PROMPTEDIT) && CONFIG_CSH_PROMPTEDIT
    static char csh_prompt_buffer[128];

    /*!< set prompt buffer */
    csh_init.prompt_buffer = csh_prompt_buffer;
    csh_init.prompt_buffer_size = sizeof(csh_prompt_buffer);
#endif

#if defined(CONFIG_CSH_HISTORY) && CONFIG_CSH_HISTORY
    static char csh_history_buffer[128];

    /*!< set history buffer */
    csh_init.history_buffer = csh_history_buffer;
    csh_init.history_buffer_size = sizeof(csh_history_buffer);
#endif

#if defined(CONFIG_CSH_LNBUFF_STATIC) && CONFIG_CSH_LNBUFF_STATIC
    static char csh_line_buffer[128];

    /*!< set linebuffer */
    csh_init.line_buffer = csh_line_buffer;
    csh_init.line_buffer_size = sizeof(csh_line_buffer);
#endif

    csh_init.uid = 0;
    csh_init.user[0] = "cherry";

    /*!< The port hash function is required,
         and the strcmp attribute is used weakly by default,
         int chry_shell_port_hash_strcmp(const char *hash, const char *str); */
    csh_init.hash[0] = "12345678"; /*!< If there is no password, set to NULL */
    csh_init.host = BOARD_NAME;
    csh_init.user_data = NULL;

    int ret = chry_shell_init(&csh, &csh_init);
    if (ret) {
        return -1;
    }

    thread_exec_created = false;

    tx_event_flags_create(&event_hdl, "csh event");

    tx_thread_create(
        &thread_buffer_repl,
        "csh buffer repl",
        thread_repl,
        0, /* param */
        thread_stack_repl,
        sizeof(thread_stack_repl),
        THREAD_REPL_PRIORITY,
        THREAD_REPL_PRIORITY,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    return 0;
}

void shell_lock(void)
{
    ULONG actual_flags_ptr;
    (void)actual_flags_ptr;
    tx_event_flags_set(&event_hdl, 0x01, TX_OR);
    tx_event_flags_get(&event_hdl, 0x02, TX_OR_CLEAR, &actual_flags_ptr, TX_WAIT_FOREVER);
}

void shell_unlock(void)
{
    ULONG actual_flags_ptr;
    (void)actual_flags_ptr;
    tx_event_flags_set(&event_hdl, 0x04, TX_OR);
    tx_event_flags_get(&event_hdl, 0x08, TX_OR_CLEAR, &actual_flags_ptr, TX_WAIT_FOREVER);
}

static int csh_exit(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    login = false;

    return 0;
}
CSH_SCMD_EXPORT_ALIAS(csh_exit, exit, );

#define __ENV_PATH "/sbin:/bin"
const char ENV_PATH[] = __ENV_PATH;
CSH_RVAR_EXPORT(ENV_PATH, PATH, sizeof(__ENV_PATH));

#define __ENV_ZERO ""
const char ENV_ZERO[] = __ENV_ZERO;
CSH_RVAR_EXPORT(ENV_ZERO, ZERO, sizeof(__ENV_ZERO));
