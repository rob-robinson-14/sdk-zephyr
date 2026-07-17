/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <nrf_sys_event_tfm_low_latency.h>
#ifdef RRAMC_PRESENT
#include <hal/nrf_rramc.h>
#elif defined(MRAMC_PRESENT)
#include <hal/nrf_mramc.h>
#endif

static void irq_low_latency_on(bool enable)
{
#ifdef RRAMC_POWER_LOWPOWERCONFIG_MODE_Standby
	nrf_rramc_lp_mode_set(NRF_RRAMC, enable ? NRF_RRAMC_LP_STANDBY : NRF_RRAMC_LP_POWER_OFF);
#elif defined(MRAMC_POWER_AUTOPOWERDOWN_ENABLE_Enable)
	nrf_mramc_power_autopowerdown_t cfg;
	nrf_mramc_power_autopowerdown_get(NRF_MRAMC, &cfg);
	if (enable) {
		cfg.enable = MRAMC_POWER_AUTOPOWERDOWN_ENABLE_Disable;
	} else {
		cfg.enable = MRAMC_POWER_AUTOPOWERDOWN_ENABLE_Enable;
	}
	nrf_mramc_power_autopowerdown_set(NRF_MRAMC, &cfg);
#endif
}

void nrf_sys_event_tfm_low_latency_on(void)
{
	irq_low_latency_on(true);
}

void nrf_sys_event_tfm_low_latency_off(void)
{
	irq_low_latency_on(false);
}