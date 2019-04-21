/*
 * Copyright (C) 2019 Nikolai Kondrashov
 *
 * This file is part of 10moons-tools.
 *
 * Uclogic-tools is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Uclogic-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 10moons-tools; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * @author Nikolai Kondrashov <spbnick@gmail.com>
 */

#include "config.h"

#include <libusb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <strings.h>

#ifndef HAVE_LIBUSB_STRERROR
static const char *
libusb_strerror(enum libusb_error err)
{
    switch (err)
    {
        case LIBUSB_SUCCESS:
            return "Success";
#define MAP(_name, _desc) \
    case LIBUSB_ERROR_##_name:          \
        return _desc " (ERROR_" #_name ")"
	    MAP(IO,
            "Input/output error");
	    MAP(INVALID_PARAM,
            "Invalid parameter");
	    MAP(ACCESS,
            "Access denied (insufficient permissions)");
	    MAP(NO_DEVICE,
            "No such device (it may have been disconnected)");
	    MAP(NOT_FOUND,
            "Entity not found");
	    MAP(BUSY,
            "Resource busy");
	    MAP(TIMEOUT,
            "Operation timed out");
	    MAP(OVERFLOW,
            "Overflow");
	    MAP(PIPE,
            "Pipe error");
	    MAP(INTERRUPTED,
            "System call interrupted (perhaps due to signal)");
	    MAP(NO_MEM,
            "Insufficient memory");
	    MAP(NOT_SUPPORTED,
            "Operation not supported or unimplemented on this platform");
	    MAP(OTHER, "Other error");
#undef MAP
        default:
            return "Unknown error code";
    }
}
#endif

#define GENERIC_ERROR(_fmt, _args...) \
    fprintf(stderr, _fmt "\n", ##_args)

#define GENERIC_FAILURE(_fmt, _args...) \
    GENERIC_ERROR("Failed to " _fmt, ##_args)

#define LIBUSB_FAILURE(_err, _fmt, _args...) \
    GENERIC_FAILURE(_fmt ": %s", ##_args, libusb_strerror(_err))

#define ERROR_CLEANUP(_fmt, _args...) \
    do {                                \
        GENERIC_ERROR(_fmt, ##_args);   \
        goto cleanup;                   \
    } while (0)

#define FAILURE_CLEANUP(_fmt, _args...) \
    do {                                \
        GENERIC_FAILURE(_fmt, ##_args); \
        goto cleanup;                   \
    } while (0)

#define LIBUSB_FAILURE_CLEANUP(_err, _fmt, _args...) \
    do {                                        \
        LIBUSB_FAILURE(_err, _fmt, ##_args);          \
        goto cleanup;                           \
    } while (0)

#define LIBUSB_GUARD(_expr, _fmt, _args...) \
    do {                                                \
        enum libusb_error err = _expr;                  \
        if (err < 0)                                    \
            LIBUSB_FAILURE_CLEANUP(err, _fmt, ##_args); \
    } while (0)

static int
probe(uint8_t bus_num, uint8_t dev_addr)
{
    int                     rc;
    int                     result      = 1;
    libusb_context         *ctx         = NULL;
    libusb_device         **dev_list    = NULL;
    ssize_t                 dev_num;
    size_t                  i;
    libusb_device          *dev;
    libusb_device_handle   *handle      = NULL;
    bool                    claimed     = false;

    LIBUSB_GUARD(libusb_init(&ctx), "initialize libusb");
#if 0
    libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, 
#endif

    LIBUSB_GUARD(dev_num = libusb_get_device_list(ctx, &dev_list),
                 "get device list");

    for (i = 0; i < (size_t)dev_num; i++) {
        dev = dev_list[i];
        if (libusb_get_bus_number(dev) == bus_num &&
            libusb_get_device_address(dev) == dev_addr)
            break;
    }
    if (i == (size_t)dev_num)
        ERROR_CLEANUP("Device not found");
    LIBUSB_GUARD(libusb_open(dev, &handle), "open device");
    libusb_free_device_list(dev_list, true);
    dev_list = NULL;

    LIBUSB_GUARD(libusb_set_auto_detach_kernel_driver(handle, 2),
                 "enable interface auto-detaching");
    LIBUSB_GUARD(libusb_claim_interface(handle, 2), "claim interface");
    rc = libusb_detach_kernel_driver(handle, 2);
    if (rc != 0 && rc != LIBUSB_ERROR_NOT_FOUND) {
        LIBUSB_FAILURE_CLEANUP(rc, "detach kernel driver");
    }

    i = 0;
#define SET_REPORT(_wValue, _bytes...) \
    do {                                                                    \
        const uint8_t report[] = {_bytes};                                  \
        LIBUSB_GUARD(libusb_control_transfer(handle, 0x21, 9, (_wValue), 2, \
                                             (unsigned char *)report,       \
                                             sizeof(report),                \
                                             250),                          \
                     "set report #%zu", i);                                 \
        i++;                                                                \
    } while (0)

#if 0
    SET_REPORT(0x0202,
               0x02, 0x00);
#endif
    SET_REPORT(0x0308,
               0x08, 0x04, 0x1d, 0x01, 0xff, 0xff, 0x06, 0x2e);
    SET_REPORT(0x0308,
               0x08, 0x03, 0x00, 0xff, 0xf0, 0x00, 0xff, 0xf0);
    SET_REPORT(0x0308,
               0x08, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);
    SET_REPORT(0x0308,
               0x08, 0x03, 0x00, 0xff, 0xf0, 0x00, 0xff, 0xf0);

#undef SET_REPORT

    result = 0;

cleanup:

    if (claimed) {
        libusb_release_interface(handle, 2);
    }
    libusb_close(handle);
    libusb_free_device_list(dev_list, true);
    libusb_exit(ctx);
    return result;
}

static void
usage(FILE *file, const char *name)
{
    fprintf(file,
            "Usage: %s BUS_NUM DEV_ADDR\n"
            "Probe a 10moons tablet.\n"
            "\n"
            "Arguments:\n"
            "    BUS_NUM    Bus number.\n"
            "    DEV_ADDR   Device address.\n"
            "\n",
            name);
}

int
main(int argc, char **argv)
{
    const char *name;

    name = rindex(argv[0], '/');
    if (name == NULL)
        name = argv[0];
    else
        name++;

    if (argc != 3) {
        fprintf(stderr, "Invalid number of arguments\n");
        usage(stderr, name);
        exit(1);
    }

    setbuf(stdout, NULL);
    return probe((uint8_t)atoi(argv[1]),
                 (uint8_t)atoi(argv[2]));
}
