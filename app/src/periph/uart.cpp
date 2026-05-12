#include "uart.h"

#include "zephyr/device.h"
#include "zephyr/drivers/uart.h"
#include "zephyr/logging/log.h"

namespace UART {
    LOG_MODULE_REGISTER(uart);

    const device *const uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

    void interrupt_handler(const device *dev, void *) {
        while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
            uint8_t buf;
            auto recv_len = uart_fifo_read(dev, &buf, 1);
            if (recv_len < 0) {
                LOG_ERR("Error reading from UART");
                return;
            }

            if (buf == 'b') {
                NRF_POWER->GPREGRET = 0x57;
                NVIC_SystemReset();
            }
        }
    }

    void init() {
        if (!device_is_ready(uart_dev)) {
            LOG_ERR("UART device not ready");
            return;
        }

        uart_irq_callback_set(uart_dev, interrupt_handler);
        uart_irq_rx_enable(uart_dev);

        LOG_INF("UART initialized");
    }
}
