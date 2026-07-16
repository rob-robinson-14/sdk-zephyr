/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Manual NVM low-latency mode for nrf_sys_event.
 *
 */

#ifndef SOC_NORDIC_COMMON_NRF_SYS_EVENT_MANUAL_H_
#define SOC_NORDIC_COMMON_NRF_SYS_EVENT_MANUAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enter manual NVM low-latency mode (refcounted).
 *
 * @retval NRF_SYS_EVENT_MANUAL_HANDLE on success.
 * @retval -EAGAIN if the reference count would overflow.
 */
void nrf_sys_event_manual_register(void);

/**
 * @brief Leave manual NVM low-latency mode (refcounted).
 *
 * @retval 0 on success.
 * @retval -EINVAL if there is no active registration.
 */
void nrf_sys_event_manual_unregister(void);

#ifdef __cplusplus
}
#endif

#endif /* SOC_NORDIC_COMMON_NRF_SYS_EVENT_MANUAL_H_ */
