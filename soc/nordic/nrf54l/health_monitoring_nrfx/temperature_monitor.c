// nrf/tests/benchmarks/peripheral_load/src/temp_thread.c
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include "temperature_monitor.h"

LOG_MODULE_REGISTER(die_temp_monitoring, CONFIG_HEALTH_MONITOR_LOG_LEVEL);

static const struct device *temp_dev = DEVICE_DT_GET(DT_NODELABEL(temp));
static enum sensor_channel chan_to_use = SENSOR_CHAN_DIE_TEMP;

/** @brief global variable to store battery voltage */
static atomic_t die_temperature_raw;

static void die_temperature_work_handler(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(get_die_temperature, die_temperature_work_handler);

/**
 * @brief Function for getting pointer to die temperature.
 *
 * @return Pointer to atomic variable holding die temperature
 */
atomic_t *get_die_temperature_pointer(void)
{
	return &die_temperature_raw;
}

int die_temperature_monitoring_setup(void)
{
	int err = 0;

	if (!device_is_ready(temp_dev)) {
		LOG_ERR("Device %s is not ready.", temp_dev->name);
		return err = -ENODEV;
	}

	k_work_schedule(&get_die_temperature, K_MSEC(CONFIG_DIE_TEMP_MONITOR_INTERVAL_MS));

	return err;
}

static void die_temperature_work_handler(struct k_work *work)
{
	int err;
	struct sensor_value val;
	int32_t temp_val;

	(void)work;

	err = sensor_sample_fetch(temp_dev);
	if (err < 0) {
		LOG_ERR("sensor_sample_fetch_chan() returned: %d", err);
		return;
	}

	err = sensor_channel_get(temp_dev, chan_to_use, &val);
	if (err < 0) {
		LOG_ERR("sensor_channel_get() returned: %d", err);
		return;
	}

	temp_val = (val.val1 * 100) + (val.val2 / 10000);
	atomic_set(&die_temperature_raw,temp_val);
	LOG_INF("DIE_TEMP is %d.%02u", temp_val / 100, abs(temp_val) % 100);
	k_work_schedule(&get_die_temperature, K_MSEC(CONFIG_DIE_TEMP_MONITOR_INTERVAL_MS));

	return;
}
