#pragma once

#include <zephyr/usb/usbd.h>

namespace USBD {
    usbd_context *init();
}
