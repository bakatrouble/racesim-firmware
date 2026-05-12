#pragma once

#include <zephyr/devicetree.h>

#include "periph/usbd.h"
#include "periph/gazell.h"
#include "periph/uart.h"

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_hid_device)
#include "periph/hid.h"
#endif

#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), gpios)
#include "periph/gpio.h"
#endif

#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#include "periph/adc.h"
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(avia_hx711)
#include "periph/hx711.h"
#endif
