# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

zephyr_library_sources(health_monitoring.c)
zephyr_include_directories(.)

# # if(CONFIG_VBAT_MONITOR_ENABLE)
# set(EXTRA_DTC_OVERLAY_FILE "vbatt_monitor.overlay")
# # endif()

# if(CONFIG_VINTERNAL_MONITOR_ENABLE)
#   set(EXTRA_DTC_OVERLAY_FILE "v_internal_monitor.overlay")
# endif()
