#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <drivers/sensor/hx7xx/hx7xx.h>
#include <zephyr/logging/log.h>

#include "hx711.h"

namespace HX711 {
    LOG_MODULE_REGISTER(hx711);

    const device *const dev = DEVICE_DT_GET(DT_NODELABEL(hx711));

    void init() {
        if (!device_is_ready(dev)) {
            LOG_ERR("HX711 device not ready");
            return;
        }

        constexpr sensor_value calibration_value {
            1
        };
        const int err = sensor_attr_set(dev, SENSOR_CHAN_MASS, SENSOR_ATTR_CALIBRATION, &calibration_value);
        if (err != 0) {
            LOG_ERR("Failed to set calibration attribute: %d", err);
            return;
        }

        LOG_INF("Found HX711 device");
    }

    int32_t read() {
        sensor_value value{};

        int err = sensor_sample_fetch(dev);
        if (err != 0) {
            LOG_ERR("Failed to read from sensor: %d", err);
            return err;
        }

        err = sensor_channel_get(dev, SENSOR_CHAN_MASS, &value);
        if (err != 0) {
            LOG_ERR("Failed to read from sensor: %d", err);
            return err;
        }

        return value.val1;
    }
}
