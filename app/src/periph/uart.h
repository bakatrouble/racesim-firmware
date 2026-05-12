#pragma once

#include <zephyr/kernel.h>

namespace UART {
    int32_t read(size_t channel);
    void init();
}

