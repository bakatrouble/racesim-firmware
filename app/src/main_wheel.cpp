/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include "zephyr/usb/class/hid.h"

LOG_MODULE_REGISTER(main);

#include "periph.h"

struct [[gnu::packed]] packet_t {
    char id;
    uint16_t buttons;
};

const i2c_dt_spec i2c_dev = I2C_DT_SPEC_GET(DT_NODELABEL(exp1));

[[noreturn]] int main() {
    UART::init();
    USBD::init();
    Gazell::init();
    GPIO::init();

    while (true) {
        const auto buttons = GPIO::read();
        LOG_INF("Buttons state: %04X", buttons);
        const packet_t packet {
            'W',
            buttons,
        };
        Gazell::send_packet(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
        k_sleep(K_MSEC(10));
    }
}
