/*
 * Copyright (c) 2020-2023 ndeadly
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <switch.h>
#include <cstdio>
#include <cstring>
#include <map>

#ifdef __cplusplus
extern "C" {
#endif

void userAppInit(void) {
    Result rc = usbHsInitialize();
    if (R_FAILED(rc)) {
        fatalThrow(rc);
    }
}

void userAppExit(void) {
    usbHsExit();
}

#ifdef __cplusplus
}
#endif

u8 g_event_index = 0;
Event g_interface_available_event = {};

const UsbHsInterfaceFilter g_interface_filter = {
    .Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_bcdDevice_Min,
    .idVendor = 0x045e,
    .bcdDevice_Min = 0
};

constexpr size_t MaxUsbHsInterfacesCount = 16;
UsbHsInterface g_interfaces[MaxUsbHsInterfacesCount] = {};

constexpr size_t UsbBufferSize = 0x1000;
alignas(0x1000) constinit u8 g_usb_buffer[UsbBufferSize];

struct XboxControllerFirmwareVersion {
    u16 major;
    u16 minor;
    u16 micro;
    u16 rev;
};

struct XboxControllerInfo {
    bool supports_bluetooth;
    u16 model;
    const char *name;
};

std::map<u16, XboxControllerInfo> g_controller_map {
    {0x02D1, {false, 1537, "Xbox One Controller"}},
    {0x02DD, {false, 1697, "Xbox One Controller"}},
    {0x02E3, {false, 1698, "Xbox One Elite Controller"}},
    {0x02EA, {true,  1708, "Xbox One X|S Controller"}},
    {0x0B00, {true,  1797, "Xbox One Elite Series 2 Controller"}},
    {0x0B12, {true,  1914, "Xbox Series X|S Controller"}}
};

Result GetXboxControllerFirmwareVersion(UsbHsInterface *interface, XboxControllerFirmwareVersion *version) {
    UsbHsClientIfSession if_session;
    Result rc = usbHsAcquireUsbIf(&if_session, interface);

    if (R_SUCCEEDED(rc)) {
        usb_endpoint_descriptor *ep_desc;
        for (int i = 0; i < 15; ++i) {
            ep_desc = &if_session.inf.inf.input_endpoint_descs[i];

            if (ep_desc->bEndpointAddress == 0x82) {
                UsbHsClientEpSession ep_session;
                rc = usbHsIfOpenUsbEp(&if_session, &ep_session, 1, ep_desc->wMaxPacketSize, ep_desc);
                if (R_SUCCEEDED(rc)) {
                    u32 tx_size = 0;
                    std::memset(g_usb_buffer, 0, UsbBufferSize);
                    rc = usbHsEpPostBuffer(&ep_session, g_usb_buffer, 0x20, &tx_size);
                    if (R_SUCCEEDED(rc)) {
                        *version = *reinterpret_cast<XboxControllerFirmwareVersion *>(&g_usb_buffer[0x10]);
                    }
                }

                usbHsEpClose(&ep_session);

                break;
            }
        }
    }

    usbHsIfClose(&if_session);

    return rc;
}

int main(int argc, char* argv[]) {
    appletLockExit();
    consoleInit(NULL);

    std::printf("Xbox Controller Bluetooth Compatibility Tool. 2023 ndeadly\n\n");
    std::printf("Connect an Xbox controller via USB to determine Bluetooth compatibility.\n");
    std::printf("Please disable sys-con if you have it installed, as it may interfere with USB.\n\n");
    std::printf("Press + to exit.\n\n");

    Result rc = usbHsCreateInterfaceAvailableEvent(&g_interface_available_event, true, g_event_index, &g_interface_filter);
    if (R_FAILED(rc)) {
        fatalThrow(rc);
    }

    padConfigureInput(8, HidNpadStyleSet_NpadStandard);

    PadState pad;
    padInitializeAny(&pad);

    u64 kDown;
    while (appletMainLoop()) {

        padUpdate(&pad);
        kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break;

        if (R_SUCCEEDED(eventWait(&g_interface_available_event, 0))) {
            std::printf(CONSOLE_YELLOW "[!] Microsoft device connected.\n");

            s32 total_entries;
            rc = usbHsQueryAvailableInterfaces(&g_interface_filter, g_interfaces, sizeof(g_interfaces), &total_entries);

            if (R_SUCCEEDED(rc)) {
                if (total_entries > 0) {
                    UsbHsInterface *interface = &g_interfaces[0];

                    std::printf(CONSOLE_WHITE " Vendor ID:              0x%04x\n", interface->device_desc.idVendor);
                    std::printf(CONSOLE_WHITE " Product ID:             0x%04x\n", interface->device_desc.idProduct);
                    if (g_controller_map.contains(interface->device_desc.idProduct)) {
                        XboxControllerInfo info = g_controller_map[interface->device_desc.idProduct];
                        bool supports_downgrade = (info.model >= 1708) && (info.model < 1914);

                        std::printf(CONSOLE_WHITE " Variant:                %s (Model %d)\n", info.name, info.model);
                        std::printf(CONSOLE_WHITE " Bluetooth support:      %s\n", info.supports_bluetooth ? CONSOLE_GREEN "Yes" : CONSOLE_RED "No");

                        XboxControllerFirmwareVersion fw_version = {};
                        if (info.supports_bluetooth) {
                            rc = GetXboxControllerFirmwareVersion(interface, &fw_version);
                            if (R_SUCCEEDED(rc)) {
                                std::printf(CONSOLE_WHITE " Firmware version:       %d.%d.%d.%d\n",
                                    fw_version.major,
                                    fw_version.minor,
                                    fw_version.micro,
                                    fw_version.rev
                                );
                            } else {
                                std::printf(CONSOLE_WHITE " Firmware version:       " CONSOLE_RED "Error retrieving firmware version (rc=0x%x)\n", rc);
                            }

                            if (R_SUCCEEDED(rc) && fw_version.major > 0) {
                                std::printf(CONSOLE_WHITE " Bluetooth LE firmware:  %s\n", fw_version.major < 5 ? CONSOLE_GREEN "No" : CONSOLE_YELLOW "Yes");
                            } else {
                                std::printf(CONSOLE_WHITE " Bluetooth LE firmware:  " CONSOLE_YELLOW "Unknown\n");
                            }

                            std::printf(CONSOLE_WHITE " Firmware downgradeable: %s\n", supports_downgrade ? CONSOLE_GREEN "Yes" : CONSOLE_RED "No");
                        }
 
                    } else {
                        std::printf(CONSOLE_WHITE " Variant:                Unknown\n");
                    }
                }
            }

            std::printf("\n");
        }

        consoleUpdate(NULL);
    }

    usbHsDestroyInterfaceAvailableEvent(&g_interface_available_event, g_event_index);
    g_controller_map.clear();

    consoleExit(NULL);
    appletUnlockExit();

    return 0;
}
