#pragma once

namespace HID {
    typedef void (*set_report_callback_t)(const uint8_t *buf, uint16_t len);

    void set_set_report_callback(set_report_callback_t callback);
    void send_input_report(const uint8_t *buf);
    int init(const uint8_t *hid_report_desc, size_t hid_report_desc_len, uint16_t _hid_report_len);
};

