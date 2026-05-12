/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "../../../zephyr/include/zephyr/kernel.h"
#include "zephyr/usb/class/hid.h"

LOG_MODULE_REGISTER(main);

#include "periph.h"

const uint8_t hid_report_desc[] = {
    HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
    HID_USAGE(HID_USAGE_GEN_DESKTOP_GAMEPAD),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),
        HID_REPORT_ID(1),

        // pedals
        HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
        HID_USAGE(0x30),  // x
        HID_USAGE(0x31),  // y
        HID_USAGE(0x32),  // z
        HID_LOGICAL_MIN8(0),
        HID_LOGICAL_MAX8(255),
        HID_REPORT_SIZE(8),
        HID_REPORT_COUNT(3),
        HID_INPUT(0x02),

        // buttons
        HID_USAGE_PAGE(HID_USAGE_GEN_BUTTON),
        HID_USAGE_MIN8(1),
        HID_USAGE_MAX8(32),
        HID_LOGICAL_MIN8(0),
        HID_LOGICAL_MAX8(1),
        HID_REPORT_SIZE(1),
        HID_REPORT_COUNT(32),
        HID_INPUT(0x02),

        HID_REPORT_ID(2),

        // vendor commands
        HID_USAGE_PAGE16(0xFF00),
        HID_USAGE(1),
        HID_LOGICAL_MIN8(0),
        HID_LOGICAL_MAX8(255),
        HID_REPORT_SIZE(8),
        HID_REPORT_COUNT(32),
        HID_OUTPUT(0x02),
    HID_END_COLLECTION,
};

struct [[gnu::packed]] hid_report_t {
    uint8_t x;
    uint8_t y;
    uint8_t z;
    uint16_t buttons;
    uint16_t shifter;
};

hid_report_t hid_report {};
K_SEM_DEFINE(hid_report_sem, 0, 1);

bool gzll_rx_callback(const uint8_t *data_payload, const uint32_t len, uint8_t *ack_buf) {
    k_sem_take(&hid_report_sem, K_FOREVER);
    switch (data_payload[0]) {
    case 'P':  // Pedal report
        if (len == 13) {
            const auto *axes = reinterpret_cast<const int32_t *>(data_payload + 1);
            LOG_INF("Received pedal report: x=%d, y=%d, z=%d", axes[0], axes[1], axes[2]);
            hid_report.x = axes[0] & 0xFF;
            hid_report.y = axes[1] & 0xFF;
            hid_report.z = axes[2] & 0xFF;
        } else {
            LOG_WRN("Invalid pedal report length: %u", len);
        }
        break;
    case 'W':  // Wheel report
        if (len == 3) {
            const uint16_t buttons = *reinterpret_cast<const uint16_t *>(data_payload + 1);
            LOG_INF("Received wheel report: buttons=%04X", buttons);
            hid_report.buttons = buttons;
        } else {
            LOG_WRN("Invalid wheel report length: %u", len);
        }
        break;
    case 'S': // Shifter report
        if (len == 2) {
            const uint8_t value = data_payload[1];
            LOG_INF("Received shifter report: shifter=%u", value);
            hid_report.shifter = 1 << value;
        } else {
            LOG_WRN("Invalid shifter report length: %u", len);
        }
        break;
    default:
        LOG_WRN("Unknown report ID: %02x", data_payload[0]);
    }

    k_sem_give(&hid_report_sem);

    return false;
}

void hid_thread() {
    while (true) {
        k_sem_take(&hid_report_sem, K_FOREVER);
        HID::send_input_report(reinterpret_cast<const uint8_t*>(&hid_report));
        k_sem_give(&hid_report_sem);
        k_sleep(K_MSEC(10));
    }
}

[[noreturn]] int main() {
    UART::init();
    HID::init(hid_report_desc, sizeof(hid_report_desc), 7);
    USBD::init();
    Gazell::init();
    Gazell::set_rx_callback(gzll_rx_callback);

    while (true) {
        LOG_INF("Hello from Zephyr! This is a placeholder for the main loop.");
        k_sleep(K_SECONDS(1));
    }
}
