#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include "battery_voltage_monitor.h"

LOG_MODULE_REGISTER(vtf_monitoring, CONFIG_VTF_MONITOR_LOG_LEVEL);

typedef struct {
	atomic_t *g_vbatt_value;
} vtf_data_t;

static vtf_data_t vtf_data = {};

int vtf_monitoring_setup(void)
{
	int err;
	err = battery_monitoring_setup();

	if (err != 0) {
		LOG_ERR("Battery Monitoring Setup Failed");
		return err;
	}
	vtf_data.g_vbatt_value = get_battery_voltage_pointer();

//Here for now to demonstrate functionality
	for (;;) {
		k_sleep(K_SECONDS(2));
		// uint32_t voltage = atomic_get(g_vbatt_value);
		printk("Global ADC: %ld mV\n", *vtf_data.g_vbatt_value);
	}

	return 0;
}
