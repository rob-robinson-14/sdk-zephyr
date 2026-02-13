# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

zephyr_library_sources(health_monitoring.c
                      battery_monitor.c)

zephyr_include_directories(.)

# TODO Currently this does not cause an error when enabled from Kconfig
if (CONFIG_ADC)
  message(FATAL_ERROR
          "Health Monitoring feautre can not be used in conjunction with other ADC measurements,
          see sample XXXX to implement an alternative, disabling this feature may have
          adverse effects on performance over temperature")
endif()
