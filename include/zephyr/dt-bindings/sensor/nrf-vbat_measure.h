/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_NRF_VBAT_MEASURE_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_NRF_VBAT_MEASURE_H_

#include <zephyr/dt-bindings/dt-mdk-checker.h>

/* Any values added here should be checked in 
 * zephyr/drivers/sensor/nordic/vbat/vbat_measure.c
 * against the corresponding values in the MDK using
 * CHECK_DTS_BINDING_VS_MDK()
 */

#define VBAT_MEASURE_NRFX_ANALOG_TEST_AVSS       66
#define VBAT_MEASURE_NRFX_ANALOG_INTERNAL_VDD    128

#endif /* #define ZEPHYR_INCLUDE_DT_BINDINGS_NRF_VBAT_MEASURE_H_ */
