/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf_sys_event.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <stdio.h>

#ifdef CONFIG_NRF_SYS_EVENT_IRQ_LATENCY
#define ALARM_CH 0
#if IS_ENABLED(CONFIG_HAS_HW_NRF_MRAMC)
#define TIMEOUT_US 1000 /* 0x6400 autopowerdown cycles at 64 MHz + margin */
#else
#define TIMEOUT_US 100
#endif

enum flash_controller_mode {
	/* Default mode where FLASH_CONTROLLER goes to low power state and had approx.15 us wake up time. */
	FLASH_CONTROLLER_DEFAULT,
	/* Using nrf_sys_event API to schedule a PPI wake up before expected interrupt. */
	FLASH_CONTROLLER_PPI_WAKEUP,
	/* Using nrf_sys_event API to change the power mode of FLASH_CONTROLLER to 0 us wake up time. */
	FLASH_CONTROLLER_POWER_MODE,
};

static void counter_handler(const struct device *counter_dev, uint8_t ch_id,
			    uint32_t ticks, void *user_data)
{
	k_sem_give((struct k_sem *)user_data);
}

static uint32_t counter_alarm_execute(const struct device *counter_dev,
				      struct counter_alarm_cfg *alarm_cfg, k_timeout_t timeout)
{
	struct k_sem sem;
	uint32_t now;
	int err;

	k_sem_init(&sem, 0, 1);
	alarm_cfg->user_data = &sem;

#if IS_ENABLED(CONFIG_HAS_HW_NRF_MRAMC)
	k_sched_lock();
#endif
	now = k_cycle_get_32();
	err = counter_set_channel_alarm(counter_dev, ALARM_CH, alarm_cfg);
	if (err < 0) {
		printf("Failed to set the counter alarm.\n");
#if IS_ENABLED(CONFIG_HAS_HW_NRF_MRAMC)
		k_sched_unlock();
#endif
		return 0;
	}

#if IS_ENABLED(CONFIG_HAS_HW_NRF_MRAMC)
	while (k_sem_take(&sem, K_NO_WAIT) != 0) {
		__WFI();
	}
	k_sched_unlock();
#else
	err = k_sem_take(&sem, timeout);
	if (err < 0) {
		printf("Failed waiting for counter alarm.\n");
		return 0;
	}
#endif

	return k_cycle_get_32() - now;
}

static void sys_event_irq_latency_run(enum flash_controller_mode mode)
{
	const struct device *counter = DEVICE_DT_GET(DT_NODELABEL(sample_counter));
	struct counter_alarm_cfg alarm_cfg;
	uint32_t delay = TIMEOUT_US;
	uint32_t delay_adj = 4;
	uint32_t rpt = 100;
	uint32_t cyc;
	int event_handle;
	const char *mode_str = (mode == FLASH_CONTROLLER_DEFAULT) ? "default FLASH_CONTROLLER mode" :
		(mode == FLASH_CONTROLLER_PPI_WAKEUP) ? "FLASH_CONTROLLER waken by PPI" : "FLASH_CONTROLLER Standby mode";

	counter_start(counter);
	alarm_cfg.flags = 0;
	alarm_cfg.ticks = counter_us_to_ticks(counter, delay);
	alarm_cfg.callback = counter_handler;

	cyc = 0;
	for (int i = 0; i < rpt; i++) {
		sys_cache_instr_invd_all();
		if (mode != FLASH_CONTROLLER_DEFAULT) {
			uint32_t t = (mode == FLASH_CONTROLLER_PPI_WAKEUP) ? (delay + delay_adj) : 0;

			event_handle = nrf_sys_event_register(t, true);
			if (mode == FLASH_CONTROLLER_PPI_WAKEUP && event_handle == 32) {
				printk("err\n");
			}
		}

		cyc += counter_alarm_execute(counter, &alarm_cfg, K_USEC(delay + 100));
		if (mode != FLASH_CONTROLLER_DEFAULT) {
			(void)nrf_sys_event_unregister(event_handle, false);
		}
	}

	cyc /= rpt;
	printf("Alarm set for %d us, execution took:%d (%s)\n", delay, cyc, mode_str);

	counter_stop(counter);
}

static void sys_event_irq_latency(void)
{
#if IS_ENABLED(CONFIG_HAS_HW_NRF_MRAMC)
	printf("NVM controller: MRAMC\n");
#else
	printf("NVM controller: RRAMC\n");
#endif

	sys_event_irq_latency_run(FLASH_CONTROLLER_DEFAULT);
	sys_event_irq_latency_run(FLASH_CONTROLLER_POWER_MODE);
#ifdef CONFIG_NRF_SYS_EVENT_USE_GPPI
	sys_event_irq_latency_run(FLASH_CONTROLLER_PPI_WAKEUP);
#endif
}
#endif /* CONFIG_NRF_SYS_EVENT_IRQ_LATENCY */

int main(void)
{
	printf("request global constant latency mode\n");
	if (nrf_sys_event_request_global_constlat()) {
		printf("failed to request global constant latency mode\n");
		return 0;
	}
	printf("constant latency mode enabled\n");

	printf("request global constant latency mode again\n");
	if (nrf_sys_event_request_global_constlat()) {
		printf("failed to request global constant latency mode\n");
		return 0;
	}

	printf("release global constant latency mode\n");
	printf("constant latency mode will remain enabled\n");
	if (nrf_sys_event_release_global_constlat()) {
		printf("failed to release global constant latency mode\n");
		return 0;
	}

	printf("release global constant latency mode again\n");
	printf("constant latency mode will be disabled\n");
	if (nrf_sys_event_release_global_constlat()) {
		printf("failed to release global constant latency mode\n");
		return 0;
	}

	printf("constant latency mode disabled\n");

#ifdef CONFIG_NRF_SYS_EVENT_IRQ_LATENCY
	sys_event_irq_latency();

	printf("All done\n");
#endif
	return 0;
}
