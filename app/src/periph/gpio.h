#pragma once

#include <zephyr/kernel.h>

namespace GPIO {
    uint16_t read();
    void init();
}
