#pragma once

struct [[gnu::packed]] hid_report_t {
    uint8_t x;
    uint8_t y;
    uint8_t z;
    uint16_t buttons;
    uint16_t shifter;
};

namespace HID {
    typedef void (*set_report_callback_t)(const uint8_t *buf, uint16_t len);

    void set_set_report_callback(set_report_callback_t callback);
    void send_input_report(const uint8_t *buf);
    int init(const uint8_t *hid_report_desc, size_t hid_report_desc_len);
};

