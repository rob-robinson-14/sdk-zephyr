/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "battery_voltage_monitor.h"

LOG_MODULE_REGISTER(battery_voltage_monitoring, CONFIG_VTF_MONITOR_LOG_LEVEL);

static const struct device *vbat_dev = DEVICE_DT_GET(DT_NODELABEL(vbat_monitor));
static const enum sensor_channel chan_to_use = SENSOR_CHAN_VOLTAGE;

/** Millivolts at last successful sample */
static atomic_t battery_voltage_mv;

static void battery_voltage_work_handler(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(get_battery_voltage, battery_voltage_work_handler);

/**
 * @brief Function in the event battery voltage is below minimum threshold
 *
 */
__weak void battery_below_threshold(void)
{
	LOG_WRN("IRQ LIMITL triggered on battery voltage");
	LOG_WRN("Placeholder -  customer should implement their own requirements");
}

atomic_t *get_battery_voltage_pointer(void)
{
	return &battery_voltage_mv;
}

int battery_monitoring_setup(void)
{
	int err = 0;

	if (!device_is_ready(vbat_dev)) {
		LOG_ERR("Device %s is not ready.", vbat_dev->name);
		return -ENODEV;
	}

	k_work_schedule(&get_battery_voltage, K_MSEC(CONFIG_VBAT_MONITOR_INTERVAL_MS));

	return err;
}

static void battery_voltage_work_handler(struct k_work *work)
{
	int err;
	struct sensor_value val;
	int32_t battery_mv;

	(void)work;

	err = sensor_sample_fetch(vbat_dev);
	if (err < 0) {
		LOG_ERR("sensor_sample_fetch() returned: %d", err);
		return;
	}

	err = sensor_channel_get(vbat_dev, chan_to_use, &val);
	if (err < 0) {
		LOG_ERR("sensor_channel_get() returned: %d", err);
		return;
	}

	battery_mv = (int32_t)sensor_value_to_milli(&val);
	atomic_set(&battery_voltage_mv, battery_mv);
	LOG_INF("VBAT is %d mV", (int)battery_mv);

	k_work_schedule(&get_battery_voltage, K_MSEC(CONFIG_VBAT_MONITOR_INTERVAL_MS));
}
