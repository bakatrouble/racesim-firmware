#ifndef APP_ADC_H
#define APP_ADC_H

#include <zephyr/kernel.h>

namespace ADC {
    int16_t read(size_t channel);
    void init();
}

#endif //APP_ADC_H
