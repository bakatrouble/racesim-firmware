#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/logging/log.h>

#include "hid.h"


namespace HID {
    LOG_MODULE_REGISTER(hid);

    const device *const hid_dev DEVICE_DT_GET_ONE(zephyr_hid_device);

    k_msgq hid_msgq;
    k_work hid_work;

    bool hid_ready = false;
    set_report_callback_t hid_set_report_callback = nullptr;
    uint16_t hid_report_len = 0;

    void iface_ready(const device* dev, const bool ready) {
        LOG_INF("HID device %s interface is %s",
            dev->name, ready ? "ready" : "not ready");
        hid_ready = ready;
    }

    int get_report(const device*,
                 const uint8_t type, const uint8_t id, const uint16_t,
                 uint8_t *const) {
        LOG_WRN("Get Report not implemented, Type %u ID %u", type, id);

        return 0;
    }

    int set_report(const device *,
                 const uint8_t type, const uint8_t, const uint16_t len,
                 const uint8_t *const buf)
    {
        if (type != HID_REPORT_TYPE_OUTPUT) {
            LOG_WRN("Unsupported report type");
            return -ENOTSUP;
        }

        if (hid_set_report_callback != nullptr) {
            hid_set_report_callback(buf, len);
        }

        return 0;
    }

    void output_report(const device *dev, const uint16_t len,
                       const uint8_t *const buf) {
        LOG_HEXDUMP_DBG(buf, len, "o.r.");
        set_report(dev, HID_REPORT_TYPE_OUTPUT, 0U, len, buf);
    }

    void set_set_report_callback(const set_report_callback_t callback) {
        hid_set_report_callback = callback;
    }

    void send_input_report(const uint8_t *buf) {
        if (!hid_ready) {
            LOG_ERR("HID device is not ready");
            return;
        }

        const int err = k_msgq_put(&hid_msgq, buf, K_NO_WAIT);
        if (err == 0) {
            k_work_submit(&hid_work);
        }
    }

    void hid_work_handler(k_work*) {
        uint8_t buf[hid_report_len];
        while (k_msgq_get(&hid_msgq, buf, K_NO_WAIT) == 0) {
            const int ret = hid_device_submit_report(hid_dev, hid_report_len, buf);
            if (ret != 0) {
                LOG_ERR("Failed to submit report to HID device: %d", ret);
            }
        }
    }

    hid_device_ops ops = {
        .iface_ready = iface_ready,
        .get_report = get_report,
        .set_report = set_report,
        .output_report = output_report,
    };

    int init(const uint8_t *hid_report_desc, const size_t hid_report_desc_len, const uint16_t _hid_report_len) {
        if (!device_is_ready(hid_dev)) {
            LOG_ERR("HID Device is not ready");
            return -EIO;
        }

        const int ret = hid_device_register(hid_dev, hid_report_desc, hid_report_desc_len, &ops);
        if (ret != 0) {
            LOG_ERR("Failed to register HID device: %d", ret);
            return ret;
        }

        k_msgq_init(&hid_msgq, static_cast<char*>(k_malloc(_hid_report_len * 10)), hid_report_len, 10);
        k_work_init(&hid_work, hid_work_handler);
        hid_report_len = _hid_report_len;

        return 0;
    }
}
