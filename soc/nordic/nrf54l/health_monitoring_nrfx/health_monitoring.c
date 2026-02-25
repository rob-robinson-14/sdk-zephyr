#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include "battery_monitor.h"
#include "temperature_monitor.h"

LOG_MODULE_REGISTER(health_monitoring, CONFIG_HEALTH_MONITOR_LOG_LEVEL);

typedef struct {
	atomic_t *g_vbatt_value;
	atomic_t *g_die_temp_value;
} health_metrics_t;

static health_metrics_t health_metrics = {};

int health_monitoring_setup(void)
{
	int err;
	err = battery_monitoring_setup();

	if (err != 0) {
		LOG_ERR("Battery Monitoring Setup Failed");
		return err;
	}
	health_metrics.g_vbatt_value = get_battery_voltage_pointer();

	err = die_temperature_monitoring_setup();
	if (err != 0) {
		LOG_ERR("Die Temperature Monitoring Setup Failed");
		return err;
	}
	health_metrics.g_die_temp_value = get_die_temperature_pointer();

//Here for now to demonstrate functionality
	for (;;) {
		k_sleep(K_SECONDS(2));
		// uint32_t voltage = atomic_get(g_vbatt_value);
		printk("Global ADC: %ld mV\n", *health_metrics.g_vbatt_value);
		printk("Global die temp: %ld.%02u\n", *health_metrics.g_die_temp_value / 100,
		       abs(*health_metrics.g_die_temp_value) % 100);
	}

	return 0;
}
