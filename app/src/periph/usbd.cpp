#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/logging/log.h>

#include "usbd.h"

namespace USBD {
    LOG_MODULE_REGISTER(usbd_init);

    const char *const usbd_interface_blocklist[] = {
        "dfu_dfu",
        nullptr,
    };

    USBD_DEVICE_DEFINE(usbd,
               DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
               0xBACA, 0x0002);

    USBD_DESC_LANG_DEFINE(lang);
    USBD_DESC_MANUFACTURER_DEFINE(mfr, "mandarinka inc");
    USBD_DESC_PRODUCT_DEFINE(product, CONFIG_USBD_NAME);
    USBD_DESC_SERIAL_NUMBER_DEFINE(serial_number);

    USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");

    // Not self-powered, no remote wakeup = 0
    USBD_CONFIGURATION_DEFINE(fs_config, 0, 125, &fs_cfg_desc);

    void fix_code_triple(usbd_context *uds_ctx, const usbd_speed speed) {
        /* Always use class code information from Interface Descriptors */
        if (IS_ENABLED(CONFIG_USBD_CDC_ACM_CLASS) ||
            IS_ENABLED(CONFIG_USBD_CDC_ECM_CLASS) ||
            IS_ENABLED(CONFIG_USBD_CDC_NCM_CLASS) ||
            IS_ENABLED(CONFIG_USBD_MIDI2_CLASS) ||
            IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS) ||
            IS_ENABLED(CONFIG_USBD_VIDEO_CLASS)) {
            /*
             * Class with multiple interfaces have an Interface
             * Association Descriptor available, use an appropriate triple
             * to indicate it.
             */
            usbd_device_set_code_triple(uds_ctx, speed,
                            USB_BCC_MISCELLANEOUS, 0x02, 0x01);
        } else {
            usbd_device_set_code_triple(uds_ctx, speed, 0, 0, 0);
        }
    }

    void msg_cb(usbd_context *const usbd_ctx, const usbd_msg *const msg) {
        LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

        if (msg->type == USBD_MSG_CONFIGURATION) {
            LOG_INF("\tConfiguration value %d", msg->status);
        }

        if (usbd_can_detect_vbus(usbd_ctx)) {
            if (msg->type == USBD_MSG_VBUS_READY) {
                if (usbd_enable(usbd_ctx)) {
                    LOG_ERR("Failed to enable device support");
                }
            }

            if (msg->type == USBD_MSG_VBUS_REMOVED) {
                if (usbd_disable(usbd_ctx)) {
                    LOG_ERR("Failed to disable device support");
                }
            }
        }
    }

    usbd_context *setup_device() {
        int err;


        err = usbd_add_descriptor(&usbd, &lang);
        if (err) {
            LOG_ERR("Failed to initialize language descriptor (%d)", err);
            return nullptr;
        }

        err = usbd_add_descriptor(&usbd, &mfr);
        if (err) {
            LOG_ERR("Failed to initialize manufacturer descriptor (%d)", err);
            return nullptr;
        }

        err = usbd_add_descriptor(&usbd, &product);
        if (err) {
            LOG_ERR("Failed to initialize product descriptor (%d)", err);
            return nullptr;
        }

        err = usbd_add_descriptor(&usbd, &serial_number);
        if (err) {
            LOG_ERR("Failed to initialize serial number descriptor (%d)", err);
            return nullptr;
        }


        err = usbd_add_configuration(&usbd, USBD_SPEED_FS, &fs_config);
        if (err) {
            LOG_ERR("Failed to add Full-Speed configuration");
            return nullptr;
        }

        err = usbd_register_all_classes(&usbd, USBD_SPEED_FS, 1, usbd_interface_blocklist);
        if (err) {
            LOG_ERR("Failed to add register classes");
            return nullptr;
        }

        fix_code_triple(&usbd, USBD_SPEED_FS);
        usbd_self_powered(&usbd, false);

        err = usbd_msg_register_cb(&usbd, msg_cb);
        if (err) {
            LOG_ERR("Failed to register message callback");
            return nullptr;
        }

        return &usbd;
    }

    usbd_context *init() {
        int err;

        if (setup_device() == nullptr) {
            return nullptr;
        }

        err = usbd_init(&usbd);
        if (err) {
            LOG_ERR("Failed to initialize device support: %d", err);
            return nullptr;
        }

        if (!usbd_can_detect_vbus(&usbd)) {
            err = usbd_enable(&usbd);
            if (err) {
                LOG_ERR("Failed to enable device support: %d", err);
                return nullptr;
            }
        }

        return &usbd;
    }
}
