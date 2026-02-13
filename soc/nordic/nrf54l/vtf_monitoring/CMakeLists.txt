# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

zephyr_library_sources(vtf_monitoring.c)
zephyr_library_sources_ifdef(CONFIG_VBAT_MONITOR_ENABLE battery_voltage_monitor.c)

zephyr_include_directories(.)
