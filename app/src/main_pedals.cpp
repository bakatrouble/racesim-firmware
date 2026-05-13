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
    int32_t value1;
    int32_t value2;
    int32_t value3;
};

[[noreturn]] int main() {
    UART::init();
    USBD::init();
    Gazell::init();
    ADC::init();
    HX711::init();

    while (true) {
        const int32_t adc1 = ADC::read(0);
        const int32_t adc2 = ADC::read(1);
        const int32_t adc3 = HX711::read();
        const packet_t packet {
            'P',
            adc1,
            adc2,
            adc3,
        };
        LOG_INF("Pedal values: x=%d, y=%d, z=%d", adc1, adc2, adc3);
        Gazell::send_packet(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
        k_sleep(K_MSEC(10));
    }
}
