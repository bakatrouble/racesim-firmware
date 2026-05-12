#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

list(APPEND SOURCES
        src/periph/usbd.cpp
        src/periph/gazell.cpp
        src/periph/uart.cpp
)
list(APPEND CONF_FILE confs/common.conf)

#set(COMPONENT "$ENV{COMPONENT}")
if("${COMPONENT}" STREQUAL "DONGLE")
    add_compile_definitions(-DGAZELL_HOST)
    list(APPEND DTC_OVERLAY_FILE overlays/dongle.overlay)
    list(APPEND CONF_FILE confs/dongle.conf)
    list(APPEND SOURCES
            src/periph/hid.cpp
            src/main_dongle.cpp
    )
elseif("${COMPONENT}" STREQUAL "WHEEL")
    list(APPEND DTC_OVERLAY_FILE overlays/wheel.overlay)
    list(APPEND CONF_FILE confs/wheel.conf)
    list(APPEND SOURCES
            src/periph/gpio.cpp
            src/main_wheel.cpp
    )
elseif("${COMPONENT}" STREQUAL "PEDALS")
    list(APPEND DTC_OVERLAY_FILE overlays/pedals.overlay)
    list(APPEND CONF_FILE confs/pedals.conf)
    list(APPEND SOURCES
            src/periph/adc.cpp
            src/periph/hx711.cpp
            src/main_pedals.cpp
    )
elseif("${COMPONENT}" STREQUAL "SHIFTER")
    list(APPEND DTC_OVERLAY_FILE overlays/shifter.overlay)
    list(APPEND CONF_FILE confs/shifter.conf)
    list(APPEND SOURCES
            src/periph/adc.cpp
            src/periph/gpio.cpp
            src/main_shifter.cpp
    )
else()
    message(FATAL_ERROR "Unknown component specified: `${COMPONENT}`")
endif()
add_compile_definitions(-DCOMPONENT_${COMPONENT})
message("COMPONENT selected: ${COMPONENT}")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C CXX)


target_sources(app PRIVATE ${SOURCES})
