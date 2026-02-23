/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEMPERATURE_MONITOR_H_
#define _TEMPERATURE_MONITOR_H_

#include <zephyr/sys/atomic.h>

atomic_t* get_die_temperature_pointer(void);
int die_temperature_monitoring_setup(void);

#endif //_TEMPERATURE_MONITOR_H_
