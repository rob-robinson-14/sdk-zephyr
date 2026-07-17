/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Manual NVM low-latency mode for nrf_sys_event.
 *
 */

#ifndef NRF_SYS_EVENT_TFM_
#define NRF_SYS_EVENT_TFM_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enter manual NVM low-latency mode.
 *
 * @retval NRF_SYS_EVENT_MANUAL_HANDLE on success.
 * @retval -EAGAIN if the reference count would overflow.
 */
void nrf_sys_event_tfm_low_latency_on(void);

/**
 * @brief Leave manual NVM low-latency mode.
 *
 * @retval 0 on success.
 * @retval -EINVAL if there is no active registration.
 */
void nrf_sys_event_tfm_low_latency_off(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF_SYS_EVENT_TFM_ */