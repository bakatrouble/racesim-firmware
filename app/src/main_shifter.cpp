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
    uint8_t value;
};

[[noreturn]] int main() {
    UART::init();
    USBD::init();
    Gazell::init();
    GPIO::init();
    ADC::init();

    k_sleep(K_SECONDS(5));

    constexpr int16_t adc_values[] = { 830, 680, 530, 380, 230 };
    constexpr uint8_t gears[][2] = {
        { 9, 0 },
        { 1, 2 },
        { 3, 4 },
        { 5, 6 },
        { 7, 8 },
    };
    const uint16_t error =  40;

    while (true) {
        const auto gpio = GPIO::read();
        const auto adc = ADC::read(0);
        LOG_INF("GPIO state: %04X, ADC value: %d mV", gpio, adc);
        uint8_t gear = 0;
        if (gpio != 0) {
            for (size_t i=0; i < ARRAY_SIZE(adc_values); i++) {
                if (adc > adc_values[i] - error && adc < adc_values[i] + error) {
                    gear = gears[i][gpio - 1];
                    break;
                }
            }
        }
        const packet_t packet {
            'S',
            gear,
        };
        LOG_INF("Detected gear: %u", gear);
        Gazell::send_packet(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
        k_sleep(K_MSEC(10));
    }
}
