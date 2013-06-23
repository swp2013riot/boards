/**
 * Native Board stk1160_arch.h implementation
 *
 * Only measures time at the moment. Uses POSIX real-time extension
 * timer to generate periodic signal/interrupt.
 *
 * Copyright (C) 2013 Philipp Rosenkranz, Maximilian Mueller
 *
 * This file subject to the terms and conditions of the GNU General Public
 * License. See the file LICENSE in the top level directory for more details.
 *
 * @ingroup native_board
 * @ingroup stk1160
 * @{
 * @file
 * @author  Philipp Rosenkranz <philipp.rosenkranz@fu-berlin.de>
 * @author  Maximilian Mueller <m.f.mueller@fu-berlin.de>
 */

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <err.h>
#include <libusb.h>

#include "stk1160_arch.h"
#include "debug.h"

#include "cpu.h"
#include "cpu-conf.h"

/**
 * set up something ...
 */
void stk1160_arch_init(void)
{
    int ret = libusb_init(NULL);
    printf("libusb_init ... %d\n", ret);

    puts("Native STK1160 initialized.");
}
