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
#include <thread.h>
#include <malloc.h>
#include <usb.h>
#include <hwtimer.h>

#include <stk1160_arch.h>
#include "debug.h"

#include "cpu.h"
#include "cpu-conf.h"

#define STK1160_VENDOR_ID 0x05e1
#define STK1160_PRODUCT_ID 0x0408

#define STACK_SIZE 8*1024*1024
#define LIBUSB_EVENT_HANDLER_THREAD_PRIORITY 10
char event_handler_stack[STACK_SIZE];

libusb_device_handle* stk1160_usb_device_handle;
stk1160_process_data_cb_handler cb_handler = NULL;

/**
 * set up something ...
 */
void stk1160_arch_init(void)
{
    stk1160_usb_device_handle = NULL;

    libusb_device** device_list;
    libusb_device*  device_found = NULL;

    int ret_init;
    if ((ret_init = libusb_init(NULL)) != 0) {
        printf("stk1160_arch_init: %s\n", libusb_error_name(ret_init));
    }

    libusb_set_debug(NULL, 3);

    ssize_t device_count = libusb_get_device_list(NULL, &device_list);

    if (device_count < 0) {
        printf("stk1160_arch_init: no devices found\n");
        return;
    }

    for (ssize_t i = 0; i < device_count; i++) {
        libusb_device* device = device_list[i];

        struct libusb_device_descriptor device_desc;

        int r;
        if ((r = libusb_get_device_descriptor(device, &device_desc)) != 0) {
            printf("stk1160_arch_init: %s\n", libusb_error_name(ret_init));
        }

        if (device_desc.idVendor == STK1160_VENDOR_ID && device_desc.idProduct == STK1160_PRODUCT_ID) {
            device_found = device;
            break;
        }
    }

    if (device_found == NULL) {
        printf("stk1160_arch_init: couldn't find device\n");
        return;
    }

    printf("stk1160_arch_init: found device! :-]\n");

    int ret_open;
    if ((ret_open = libusb_open(device_found, &stk1160_usb_device_handle)) != 0) {
        printf("stk1160_arch_init: couldn't open device\n");
        return;
    }

    printf("stk1160_arch_init: opened device! :-)\n");
    
    int ret_claim;
    if ((ret_claim = libusb_claim_interface(stk1160_usb_device_handle, 0)) != 0) {
        printf("stk1160_arch_init: couldn't claim interface\n");
        return;
    }
    
    printf("stk1160_arch_init: claimed interface! :-D\n");
    
    /* hier müssen wir jetzt wissen, das der grabber hören will ... */
}

int usb_control_msg(uint8_t bRequestType,
                    uint8_t bRequest,
                    uint16_t wValue,
                    uint16_t wIndex,
                    uint16_t wLength,
                    void* data,
                    unsigned int timeout)
{
    return libusb_control_transfer(stk1160_usb_device_handle, bRequestType, bRequest, wValue, 
                                   wIndex, (unsigned char*) data, wLength, 
                                   timeout);
}

int init_iso_transfer(int num_iso_packets, int buffer_size, stk1160_process_data_cb_handler handler)
{
   thread_create(event_handler_stack, STACK_SIZE, LIBUSB_EVENT_HANDLER_THREAD_PRIORITY, 0, libusb_event_handling_thread, "libusb event handling thread");

    // TODO: cb_handler in libusb_transfer->user_data reinstopfen
    cb_handler = handler;
    uint8_t *buffer;

    struct libusb_transfer *transfer;
    transfer = libusb_alloc_transfer(num_iso_packets);
    printf("init_iso_transfer: transfer == %p\n", transfer);
    
    buffer = malloc(buffer_size);
    int ret;
    libusb_fill_iso_transfer(transfer, stk1160_usb_device_handle, USB_ENDPOINT_IN | 0x2, buffer, buffer_size, num_iso_packets, iso_handler, NULL, 100);
    
    libusb_set_iso_packet_lengths(transfer, buffer_size);
    
    libusb_set_interface_alt_setting(stk1160_usb_device_handle, 0, 5);
    ret = libusb_submit_transfer(transfer);
    printf("init_iso_transfer: ret2 == %d \"%s\"\n", ret, libusb_error_name(ret));
    
    //hwtimer_wait(HWTIMER_TICKS(10*1000));
    //libusb_handle_events(NULL);
    //hwtimer_wait(HWTIMER_TICKS(10*1000));
    //struct timeval t = {0, 0};
    //libusb_handle_events_timeout_completed(NULL, &t, NULL);
    
    
}

void iso_handler(struct libusb_transfer *transfer)
{
    int i;
    for (i = 0; i < transfer->num_iso_packets; i++)
    {
        printf("iso_handler: status == %d \"%s\"\n", transfer->iso_packet_desc[i].status, libusb_error_name(transfer->iso_packet_desc[i].status));

        if (transfer->iso_packet_desc[i].status == 0)
        {
            uint8_t *data = libusb_get_iso_packet_buffer_simple(transfer, i);
            uint16_t length = transfer->iso_packet_desc[i].length;

            cb_handler(0, data, length);
        }
        else
        {
            /* we are doomed */
            /* explanation: an error happened, rethrow */
            cb_handler(1, NULL, 0);
        }
    }
}

void libusb_event_handling_thread(void)
{
    puts("hey ho!");
    hwtimer_wait(HWTIMER_TICKS(10*1000));
    
    while (1)
    {
        printf("libusb_event_handling_thread ...\n");        hwtimer_wait(HWTIMER_TICKS(10*1000));
        struct timeval t = {0, 0};
        libusb_handle_events_timeout_completed(NULL, &t, NULL);
    }
}