#include "adc.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include "zephyr/logging/log.h"

namespace ADC {
    LOG_MODULE_REGISTER(adc);

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),
    constexpr adc_dt_spec adc_channels[] = {
        DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
    };

    int16_t bufs[DT_PROP_LEN(DT_PATH(zephyr_user), io_channels)];
#define ADC_SEQUENCE(_, __, idx) { \
    .buffer = &bufs[idx], \
    .buffer_size = sizeof(int16_t), \
}
    adc_sequence sequences[] = {
        DT_FOREACH_PROP_ELEM_SEP(DT_PATH(zephyr_user), io_channels, ADC_SEQUENCE, (,))
    };

    int16_t read(const size_t channel) {
        const int err = adc_read(adc_channels[channel].dev, &sequences[channel]);
        if (err != 0) {
            LOG_ERR("Failed to read ADC channel: %d", err);
            return 0;
        }

        return bufs[channel];
    }

    void init() {
        for (size_t channel=0; channel < ARRAY_SIZE(adc_channels); channel++) {
            if (!device_is_ready(adc_channels[channel].dev)) {
                LOG_ERR("ADC device not ready");
                continue;
            }

            int err = adc_channel_setup_dt(&adc_channels[channel]);
            if (err) {
                LOG_ERR("Failed to setup ADC channel: %d", err);
                continue;
            }

            err = adc_sequence_init_dt(&adc_channels[channel], &sequences[channel]);
            if (err) {
                LOG_ERR("Failed to initialize ADC sequence: %d", err);
                continue;
            }

            LOG_INF("ADC device %s initialized", adc_channels[channel].dev->name);
        }
    }
}
