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

#define VBAT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_vbat)
#define VBAT_TRIGGER_SUPPORT DT_NODE_HAS_PROP(VBAT_NODE, min_threshold_mv)

#define VBAT_THRESHOLD_MV DT_PROP_OR(DT_INST(0,nordic_nrf_vbat), min_threshold_mv, -1)

LOG_MODULE_REGISTER(battery_voltage_monitoring, CONFIG_VTF_MONITOR_LOG_LEVEL);

static const struct device *vbat_dev = DEVICE_DT_GET(VBAT_NODE);
static const enum sensor_channel chan_to_use = SENSOR_CHAN_VOLTAGE;
#if VBAT_TRIGGER_SUPPORT
static struct sensor_trigger vbat_threshold_trigger = {
	.type = SENSOR_TRIG_THRESHOLD,
	.chan = SENSOR_CHAN_VOLTAGE,
};
#endif /* VBAT_TRIGGER_SUPPORT */

/** Millivolts at last successful sample */
static atomic_t battery_voltage_mv;

static void battery_voltage_work_handler(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(get_battery_voltage, battery_voltage_work_handler);

#if VBAT_TRIGGER_SUPPORT
static void vbat_threshold_trigger_handler(const struct device *dev,
					   const struct sensor_trigger *trig);

/**
 * @brief Function in the event battery voltage is below minimum threshold
 *
 */
__weak void battery_below_threshold(void)
{
	LOG_WRN("IRQ LIMITL triggered on battery voltage");
	LOG_WRN("Placeholder -  customer should implement their own requirements");
}
#endif /* VBAT_TRIGGER_SUPPORT */

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
#if VBAT_TRIGGER_SUPPORT
	if (VBAT_THRESHOLD_MV >= 0) {
		err = sensor_trigger_set(vbat_dev, &vbat_threshold_trigger,
					 vbat_threshold_trigger_handler);
		if (err < 0) {
			LOG_ERR("sensor_trigger_set() returned: %d", err);
			return err;
		}
	}
#endif /* VBAT_TRIGGER_SUPPORT */
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

#if VBAT_TRIGGER_SUPPORT
static void vbat_threshold_trigger_handler(const struct device *dev,
					   const struct sensor_trigger *trig)
{
	ARG_UNUSED(trig);
	ARG_UNUSED(dev);

	LOG_WRN("IRQ12 LIMITL triggered on battery voltage");
	battery_below_threshold();
}
	#endif /* VBAT_TRIGGER_SUPPORT */
