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

int main(void)
{
    board_init();
    board_init_led_pins();

    // default password is : 12345678
    shell_init(BOARD_CONSOLE_BASE, true);

    uint32_t freq = clock_get_frequency(clock_mchtmr0);
    uint64_t time = mchtmr_get_count(HPM_MCHTMR) / (freq / 1000);

    while (1) {
        shell_main();

        uint64_t now = mchtmr_get_count(HPM_MCHTMR) / (freq / 1000);
        if (now > time + 5000) {
            time = now;
            shell_uart_lock();
            printf("other task interval 5S\r\n");
            shell_uart_unlock();
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

    board_led_write(atoi(argv[1]) == 0);
    return 0;
}
CSH_CMD_EXPORT(write_led, );
