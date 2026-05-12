#include <zephyr/kernel.h>
#include <nrf_gzll.h>
#include <gzll_glue.h>
#include <zephyr/logging/log.h>

#include "gazell.h"

#define ACK_PAYLOAD_LENGTH 8

namespace Gazell {
    LOG_MODULE_REGISTER(gazell);

#ifdef GAZELL_HOST
    struct gzll_rx_result {
        uint32_t pipe;
        nrf_gzll_host_rx_info_t info;
    };
#else
    struct gzll_tx_result {
        bool success;
        uint32_t pipe;
        nrf_gzll_device_tx_info_t info;
    };
#endif

#ifdef GAZELL_HOST
    K_MSGQ_DEFINE(gzll_rx_msgq,
              sizeof(gzll_rx_result),
              1,
              sizeof(uint32_t));
#else
    K_MSGQ_DEFINE(gzll_tx_msgq,
              sizeof(gzll_tx_result),
              1,
              sizeof(uint32_t));
#endif

    uint8_t data_payload[NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH];
    uint8_t ack_payload[ACK_PAYLOAD_LENGTH];

#ifdef GAZELL_HOST
    k_work gzll_rx_work;
#else
    k_work gzll_tx_work;
#endif

#ifdef GAZELL_HOST
    rx_callback_t gzll_rx_callback = nullptr;
#else
    ack_callback_t gzll_ack_callback = nullptr;
#endif

#ifndef GAZELL_HOST
    void gzll_device_report_tx(const bool success,
                               const uint32_t pipe,
                               const nrf_gzll_device_tx_info_t* tx_info) {
        const gzll_tx_result tx_result {
            success,
            pipe,
            *tx_info,
        };

        const int err = k_msgq_put(&gzll_tx_msgq, &tx_result, K_NO_WAIT);
        if (!err) {
            /* Get work handler to run */
            k_work_submit(&gzll_tx_work);
        } else {
            LOG_ERR("Cannot put TX result to message queue");
        }
    }
#endif

    extern "C" {
    void nrf_gzll_device_tx_success(const uint32_t pipe, const nrf_gzll_device_tx_info_t tx_info) {
#ifndef GAZELL_HOST
        gzll_device_report_tx(true, pipe, &tx_info);
#endif
    }

    void nrf_gzll_device_tx_failed(const uint32_t pipe, const nrf_gzll_device_tx_info_t tx_info) {
#ifndef GAZELL_HOST
        gzll_device_report_tx(false, pipe, &tx_info);
#endif
    }

    void nrf_gzll_disabled() {}

    void nrf_gzll_host_rx_data_ready(const uint32_t pipe, const nrf_gzll_host_rx_info_t info) {
#ifdef GAZELL_HOST
        const gzll_rx_result rx_result {
            .pipe = pipe,
            .info = info,
        };

        if (k_msgq_put(&gzll_rx_msgq, &rx_result, K_NO_WAIT) != 0) {
            LOG_ERR("Failed to put GZLL RX result in message queue");
        } else {
            k_work_submit(&gzll_rx_work);
        }
#endif
    }
    }

#ifdef GAZELL_HOST
    void gzll_rx_result_handler(const gzll_rx_result *rx_result) {
        uint32_t data_payload_length = NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH;

        const bool result_value = nrf_gzll_fetch_packet_from_rx_fifo(
            rx_result->pipe,
            data_payload,
            &data_payload_length
        );

        if (!result_value) {
            LOG_ERR("RX fifo error");
        } else if (data_payload_length > 0) {
            LOG_INF("Received data on pipe %u, length %u", rx_result->pipe, data_payload_length);
            LOG_HEXDUMP_DBG(data_payload, data_payload_length, "Data payload");
            if (gzll_rx_callback != nullptr) {
                const bool need_ack = gzll_rx_callback(data_payload, data_payload_length, ack_payload);
                if (need_ack) {
                    if (!nrf_gzll_add_packet_to_tx_fifo(rx_result->pipe, ack_payload, ACK_PAYLOAD_LENGTH)) {
                        LOG_ERR("Failed to add ACK payload to FIFO");
                    } else {
                        LOG_INF("ACK payload added to FIFO");
                        LOG_HEXDUMP_DBG(ack_payload, ACK_PAYLOAD_LENGTH, "ACK payload");
                    }
                }
            }
        }
    }
#else
    void gzll_tx_result_handler(const gzll_tx_result *tx_result) {
        uint32_t ack_payload_length = NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH;

        if (tx_result->success) {
            if (tx_result->info.payload_received_in_ack) {
                const int res = nrf_gzll_fetch_packet_from_rx_fifo(tx_result->pipe, ack_payload, &ack_payload_length);
                if (!res) {
                    LOG_ERR("Failed to fetch ACK payload from FIFO");
                } else {
                    LOG_INF("Received ACK payload on pipe %u, length %u", tx_result->pipe, ack_payload_length);
                    LOG_HEXDUMP_DBG(ack_payload, ack_payload_length, "ACK payload");
                    if (gzll_ack_callback != nullptr) {
                        gzll_ack_callback(ack_payload, ack_payload_length);
                    }
                }
            }
        }
    }
#endif

#ifdef GAZELL_HOST
    void gzll_rx_work_handler(k_work*) {
        gzll_rx_result rx_result{};

        while (k_msgq_get(&gzll_rx_msgq, &rx_result, K_NO_WAIT) == 0) {
            gzll_rx_result_handler(&rx_result);
        }
    }
#else
    void gzll_tx_work_handler(k_work*) {
        gzll_tx_result tx_result{};

        while (k_msgq_get(&gzll_tx_msgq, &tx_result, K_NO_WAIT) == 0) {
            gzll_tx_result_handler(&tx_result);
        }
    }
#endif


#ifdef GAZELL_HOST
    void set_rx_callback(const rx_callback_t rx_callback) {
        gzll_rx_callback = rx_callback;
    }
#else
    void set_ack_callback(const ack_callback_t ack_callback) {
        gzll_ack_callback = ack_callback;
    }

    void send_packet(const uint8_t* payload, const size_t len) {
        const bool res = nrf_gzll_add_packet_to_tx_fifo(0, payload, len);
        if (!res) {
            LOG_WRN("Failed to add TX payload to FIFO");
        }
    }
#endif

    int init() {
#ifdef GAZELL_HOST
        k_work_init(&gzll_rx_work, gzll_rx_work_handler);
#else
        k_work_init(&gzll_tx_work, gzll_tx_work_handler);
#endif

        nrf_gzll_set_base_address_0(0xB0A0C0A0);

        int res = gzll_glue_init();
        if (!res) {
            LOG_ERR("Cannot initialize GZLL glue code");
            return -1;
        }

        res = nrf_gzll_init(
#ifdef GAZELL_HOST
            NRF_GZLL_MODE_HOST
#else
            NRF_GZLL_MODE_DEVICE
#endif
        );
        if (!res) {
            LOG_ERR("Cannot initialize GZLL");
            return -1;
        }

        res = nrf_gzll_enable();
        if (!res) {
            LOG_ERR("Cannot enable GZLL");
            return -1;
        }

        LOG_INF("Gazell initialized");

        return 0;
    }
}
