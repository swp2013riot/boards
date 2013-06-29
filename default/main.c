/*
 * Copyright (C) 2008, 2009, 2010  Kaspar Schleiser <kaspar@schleiser.de>
 */

#include <stdio.h>
#include <string.h>

#include <posix_io.h>
#include <ltc4150.h>
#include <shell.h>
#include <shell_commands.h>
#include <board_uart0.h>
#include <transceiver.h>

static int shell_readc(void) {
    char c = 0;
    (void) posix_read(uart0_handler_pid, &c, 1);
    return c;
}

static void shell_putchar(int c) {
    (void) putchar(c);
}

int main(void) {
    shell_t shell;
    (void) posix_open(uart0_handler_pid, 0);
#ifdef MODULE_LTC4150
    ltc4150_start();
#endif
#ifdef MODULE_TRANSCEIVER
    transceiver_init(TRANSCEIVER_CC1100);
    (void) transceiver_start();
#endif
    
    (void) puts("Welcome to RIOT!");

    shell_init(&shell, NULL, shell_readc, shell_putchar);

    shell_run(&shell);
    return 0;
}


