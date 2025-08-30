/*
 * Copyright (c) 2025, Egahp
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "csh.h"

#include "usbh_core.h"
#include "usbh_hid.h"
#include "usbh_core.h"
#include "usb_list.h"

extern usb_slist_t g_bus_head;

static void usbh_list_device(struct usbh_hub *hub, chry_shell_t *csh, bool astree, bool verbose, int dev_addr, int vid, int pid)
{
    static const char *speed_table[] = {
        "UNKNOWN",
        "low-speed",
        "full-speed",
        "high-speed",
        "wireless",
        "super-speed",
        "super-speed-plus",
    };

    static const char *root_speed_table[] = {
        "UNKNOWN",
        "1.1",
        "1.1",
        "2.0",
        "2.5",
        "3.0",
        "3.0",
    };

    static const uint16_t speed_baud[] = {
        0,
        12,
        12,
        480,
        480,
        5000,
        10000,
    };

    struct usbh_bus *bus;
    struct usbh_hubport *hport;
    struct usbh_hub *hub_next;

    uint8_t imbuf[64];
    uint8_t ipbuf[64];

    const uint8_t *pimstr;
    const uint8_t *pipstr;

    bool imvalid = false;
    bool ipvalid = false;

    int ret;

    bus = hub->bus;

    (void)speed_table;

    if (hub->is_roothub) {
        if (astree) {
            csh_printf(csh, "/:  Bus %02u.Port 1: Dev %u, Class=root_hub, Driver=hcd, %uM\r\n",
                       bus->busid, hub->hub_addr, speed_baud[hub->speed]);

        } else {
            if ((dev_addr < 0) || (hub->hub_addr == dev_addr)) {
                if (((vid < 0) || (vid == 0xffff)) && ((pid < 0) || (pid == 0xffff))) {
                    csh_printf(csh, "Bus %03u Device %03u: ID %04x:%04x %s %s root hub\r\n",
                               bus->busid, hub->hub_addr, 0xffff, 0xffff,
                               "Cherry-Embedded", root_speed_table[hub->speed]);
                }
            }
        }
    }

    for (uint8_t port = 0; port < hub->nports; port++) {
        hport = &hub->child[port];
        if (hport->connected) {
            ret = 0;
            if (hport->device_desc.iManufacturer) {
                memset(imbuf, 0, sizeof(imbuf));
                ret |= usbh_get_string_desc(hport, hport->device_desc.iManufacturer, imbuf, sizeof(imbuf));
                if (strnlen((void *)imbuf, sizeof(imbuf))) {
                    imvalid = true;
                }
            }

            if (hport->device_desc.iProduct) {
                memset(ipbuf, 0, sizeof(ipbuf));
                ret |= usbh_get_string_desc(hport, hport->device_desc.iProduct, ipbuf, sizeof(ipbuf));
                if (strnlen((void *)ipbuf, sizeof(ipbuf))) {
                    ipvalid = true;
                }
            }

            if (imvalid) {
                pimstr = imbuf;
            } else {
                pimstr = "Not specified Manufacturer";
            }

            if (ipvalid) {
                pipstr = ipbuf;
            } else {
                pipstr = "Not specified Product";
            }

            if (!astree) {
                if ((dev_addr < 0) || (hport->dev_addr == dev_addr)) {
                    if (((vid < 0) || (vid == hport->device_desc.idVendor)) && ((pid < 0) || (pid == hport->device_desc.idProduct))) {
                        csh_printf(csh, "Bus %03u Device %03u: ID %04x:%04x %s %s\r\n",
                                   bus->busid, hport->dev_addr, hport->device_desc.idVendor, hport->device_desc.idProduct,
                                   pimstr, pipstr);
                    }
                }
            }

            for (uint8_t intf = 0; intf < hport->config.config_desc.bNumInterfaces; intf++) {
                if (hport->config.intf[intf].class_driver && hport->config.intf[intf].class_driver->driver_name) {
                    if (astree) {
                        for (uint8_t j = 0; j < hub->index; j++) {
                            csh_printf(csh, "    ");
                        }

                        csh_printf(csh, "|__ Port %u: Dev %u, If %u, ClassDriver=%s, %uM\r\n",
                                   hport->port, hport->dev_addr, intf, hport->config.intf[intf].class_driver->driver_name, speed_baud[hport->speed]);
                    }

                    if (!strcmp(hport->config.intf[intf].class_driver->driver_name, "hub")) {
                        hub_next = hport->config.intf[intf].priv;

                        if (hub_next && hub_next->connected) {
                            usbh_list_device(hub_next, csh, astree, verbose, dev_addr, vid, pid);
                        }
                    }
                } else if (astree) {
                    for (uint8_t j = 0; j < hub->index; j++) {
                        csh_printf(csh, "    ");
                    }

                    csh_printf(csh, "|__ Port %u: Dev %u, If 0 ClassDriver=none, %uM\r\n",
                               hport->port, hport->dev_addr, speed_baud[hport->speed]);
                }
            }
        }
    }
}

static int lsusb(int argc, char **argv)
{
    chry_shell_t *csh = (void *)argv[argc + 1];
    usb_slist_t *bus_list;
    struct usbh_bus *bus;

    int busid = -1;
    int dev_addr = -1;
    int vid = -1;
    int pid = -1;
    bool astree = false;
    bool verbose = false;

    while (argc > 1) {
        argc--;
        argv++;

        if (!strcmp(*argv, "-V") || !strcmp(*argv, "--version")) {
            csh_printf(csh, "CherryUSB version %s\r\n", CHERRYUSB_VERSION_STR);
            return 0;
        } else if (!strcmp(*argv, "-h") || !strcmp(*argv, "--help")) {
            CSH_CALL_HELP("lsusb");
            return 0;
        } else if (!strcmp(*argv, "-v") || !strcmp(*argv, "--verbose")) {
            verbose = true;
        } else if (!strcmp(*argv, "-t") || !strcmp(*argv, "--tree")) {
            astree = true;
        } else if (!strcmp(*argv, "-s")) {
            if (argc > 1) {
                argc--;
                argv++;

                if (*argv[0] == '-') {
                    continue;
                }

                char *endptr;
                const char *colon = strchr(*argv, ':');
                (void)endptr;

                if (colon != NULL) {
                    const char *str;
                    if (colon > *argv) {
                        busid = strtol(*argv, &endptr, 10);
                    }
                    str = colon + 1;
                    if (*str != '\0') {
                        dev_addr = strtol(str, &endptr, 10);
                        if (dev_addr <= 0 || dev_addr >= 128) {
                            dev_addr = -1;
                        }
                    }
                } else {
                    dev_addr = strtol(*argv, &endptr, 10);
                    if (dev_addr <= 0 || dev_addr >= 128) {
                        dev_addr = -1;
                    }
                }
            }
        } else if (!strcmp(*argv, "-d")) {
            if (argc > 1) {
                argc--;
                argv++;

                if (*argv[0] == '-') {
                    continue;
                }

                char *endptr;
                const char *colon = strchr(*argv, ':');
                (void)endptr;

                if (colon == NULL) {
                    continue;
                }
                const char *str;

                vid = strtol(*argv, &endptr, 16);
                if (vid < 0 || vid > 0xffff) {
                    vid = -1;
                    continue;
                }
                str = colon + 1;
                if (*str != '\0') {
                    pid = strtol(str, &endptr, 16);
                    if (pid < 0 || pid > 0xffff) {
                        pid = -1;
                    }
                }
            }
        }
    }

    if (astree) {
        busid = -1;
        dev_addr = -1;
        vid = -1;
        pid = -1;
        verbose = false;
    }

    usb_slist_for_each(bus_list, &g_bus_head)
    {
        bus = usb_slist_entry(bus_list, struct usbh_bus, list);
        if (busid >= 0) {
            if (bus->busid != busid) {
                continue;
            }
        }

        usbh_list_device(&bus->hcd.roothub, csh, astree, verbose, dev_addr, vid, pid);
    }

    return 0;
}

CSH_SCMD_EXPORT_FULL(
    lsusb,
    "list USB Devices",
    "lsusb [options]\r\n"
    "\r\n"
    "options:\r\n"
    "-v, --verbose\r\n"
    "    - increase verbosity (show descriptors)\r\n"
    "-s [[bus]:][dev_addr]\r\n"
    "    - show only devices with specified device and/or\r\n"
    "      bus numbers (in decimal)\r\n"
    "-d vendor:[product]\r\n"
    "    - show only devices with the specified vendor and\r\n"
    "      product ID numbers (in hexadecimal)\r\n"
    "-t, --tree\r\n"
    "    - dump the physical USB device hierarchy as a tree\r\n"
    "-V, --version\r\n"
    "    - show version of the cherryusb\r\n"
    "-h, --help\r\n"
    "    - show usage and help information\r\n");
