#pragma once

namespace Gazell {
    typedef bool (*rx_callback_t)(const uint8_t *payload, uint32_t len, uint8_t *ack_buf);
    typedef void (*ack_callback_t)(const uint8_t *ack, uint32_t len);

#ifdef GAZELL_HOST
    void set_rx_callback(rx_callback_t rx_callback);
#else
    void set_ack_callback(ack_callback_t ack_callback);
    void send_packet(const uint8_t *payload, size_t len);
#endif
    int init();
}
