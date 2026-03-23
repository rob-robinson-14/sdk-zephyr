/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _VBAT_MEASURE_H_
#define _VBAT_MEASURE_H_

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>

#define VBAT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_vbat)
#define VBAT_TRIGGER_SUPPORT DT_NODE_HAS_PROP(VBAT_NODE, min_threshold_mv)

struct vbat_config {
	struct adc_dt_spec adc;
	int32_t min_threshold_mv;
};

struct vbat_data {
	struct adc_sequence sequence;
	int16_t raw;
#if VBAT_TRIGGER_SUPPORT
	const struct sensor_trigger *trigger;
	sensor_trigger_handler_t trigger_handler;
#endif /* VBAT_TRIGGER_SUPPORT */
};

#endif /* _VBAT_MEASURE_H_ */