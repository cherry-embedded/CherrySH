/*
 * Copyright (c) 2021 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include "board.h"
#include "hpm_clock_drv.h"
#include "hpm_mchtmr_drv.h"
#include "shell.h"

SDK_DECLARE_EXT_ISR_M(BOARD_CONSOLE_UART_IRQ, shell_uart_isr)

int main(void)
{
    board_init();
    board_init_led_pins();

    uart_config_t shell_uart_config = { 0 };
    uart_default_config(BOARD_CONSOLE_UART_BASE, &shell_uart_config);
    shell_uart_config.src_freq_in_hz = clock_get_frequency(BOARD_CONSOLE_UART_CLK_NAME);
    shell_uart_config.rx_fifo_level = uart_rx_fifo_trg_not_empty;

    printf("init uart\r\n");
    if (status_success != uart_init(BOARD_CONSOLE_UART_BASE, &shell_uart_config)) {
        /* uart failed to be initialized */
        printf("failed to initialize uart\r\n");
        while (1)
            ;
    }

    uart_enable_irq(BOARD_CONSOLE_UART_BASE, uart_intr_rx_data_avail_or_timeout);
    intc_m_enable_irq_with_priority(BOARD_CONSOLE_UART_IRQ, 1);

    /* default password is : 12345678 */
    shell_init(BOARD_CONSOLE_UART_BASE, true);

    uint32_t freq = clock_get_frequency(clock_mchtmr0);
    uint64_t time = mchtmr_get_count(HPM_MCHTMR) / (freq / 1000);

    while (1) {
        shell_main();

        uint64_t now = mchtmr_get_count(HPM_MCHTMR) / (freq / 1000);
        if (now > time + 5000) {
            time = now;
            shell_lock();
            printf("other task interval 5S\r\n");
            shell_unlock();
        }
    }

    return 0;
}

static int test(int argc, char **argv)
{
    printf("test: \r\n");
    printf("argc=<%d>\r\n", argc);
    for (uint8_t i = 0; i < argc; i++) {
        printf("argv[%d]:0x%08x=<%s>\r\n", i, (uintptr_t)argv[i], argv[i]);
    }

    printf("argv[%d]=<0x%08x>\r\n", argc, argv[argc]);
    printf("argv[%d]=<0x%08x>\r\n\r\n", argc + 1, argv[argc + 1]);

    return 0;
}
CSH_CMD_EXPORT(test, );

static int toggle_led(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    board_led_toggle();
    return 0;
}
CSH_CMD_EXPORT(toggle_led, );

static int write_led(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: write_led <status>\r\n\r\n");
        printf("  status    0 or 1\r\n\r\n");
        return -1;
    }

    board_led_write(!board_get_led_gpio_off_level() ^ (atoi(argv[1]) == 0));
    return 0;
}
CSH_CMD_EXPORT(write_led, );
