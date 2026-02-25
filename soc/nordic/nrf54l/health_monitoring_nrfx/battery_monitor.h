/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BATTERY_MONITOR_H_
#define _BATTERY_MONITOR_H_

#include <zephyr/sys/atomic.h>

int battery_monitoring_setup(void);
atomic_t *get_battery_voltage_pointer(void);
void battery_below_threshold(void);

#endif /* _BATTERY_MONITOR_H_ */
