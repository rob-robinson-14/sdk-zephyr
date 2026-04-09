/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DT_MDK_HELPER_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DT_MDK_HELPER_H_

/* Check dt-bindings match MDK frequency division definitions*/
#define CHECK_DTS_BINDING_VS_MDK(dt, mdk) \
	BUILD_ASSERT((mdk) == (dt), \
		"Different " #mdk " definition in MDK and devicetree binding")

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DT_MDK_HELPER_H_ */