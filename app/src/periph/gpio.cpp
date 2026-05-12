//
// Created by bakatrouble on 11.05.2026.
//

#include "gpio.h"
#include "zephyr/drivers/gpio.h"

#include "zephyr/logging/log.h"

namespace GPIO {
    LOG_MODULE_REGISTER(gpio);

    constexpr gpio_dt_spec buttons[] = {
        DT_FOREACH_PROP_ELEM_SEP(DT_PATH(zephyr_user), gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))
    };

    uint16_t read() {
        uint16_t result = 0;
        size_t counter = 0;
        for (const auto &button : buttons) {
            result |= gpio_pin_get_dt(&button) << (counter++);
        }
        return result;
    }

    void init() {
        for (const auto &button : buttons) {
            const int err = gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
            if (err != 0) {
                LOG_ERR("Failed to configure GPIO pin: %d", err);
            }
        }
    }
}
