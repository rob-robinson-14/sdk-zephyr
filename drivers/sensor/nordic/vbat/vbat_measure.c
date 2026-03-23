/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_vbat

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/nrf-vbat_measure.h>
#include <zephyr/logging/log.h>
#include <nrfx_saadc.h>

LOG_MODULE_REGISTER(nrf_vbat, CONFIG_SENSOR_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "multiple instances not supported");

CHECK_DTS_BINDING_VS_MDK(VBAT_MEASURE_NRFX_ANALOG_TEST_AVSS,	 NRFX_ANALOG_TEST_AVSS);
CHECK_DTS_BINDING_VS_MDK(VBAT_MEASURE_NRFX_ANALOG_INTERNAL_VDD, NRFX_ANALOG_INTERNAL_VDD);

#define VBAT_TRIGGER_SUPPORT DT_ANY_INST_HAS_PROP_STATUS_OKAY(min_threshold_mv)

#if defined(NRF7120_ENGA_XXAA) || defined(NRF54L15_XXAA)
#define VDD_VOLTAGE 		900
#define CHANNEL_GAIN		NRF_SAADC_GAIN1_4
#else
#error "Unsupported SoC series for vbat monitoring"
#endif

struct vbat_config {
	const nrfx_analog_input_t pselp;
	const nrfx_analog_input_t pseln;
	int32_t min_threshold_mv;
	bool invert_voltage;
};

struct gain_desc {
	uint8_t mul;
	uint8_t div;
};

static const struct gain_desc gains[] = {
#if NRF_SAADC_HAS_GAIN_1_6
	[NRF_SAADC_GAIN1_6] = {.mul = 6, .div = 1},
#endif
#if NRF_SAADC_HAS_GAIN_1_5
	[NRF_SAADC_GAIN1_5] = {.mul = 5, .div = 1},
#endif
#if NRF_SAADC_HAS_GAIN_1_4
	[NRF_SAADC_GAIN1_4] = {.mul = 4, .div = 1},
#endif
#if NRF_SAADC_HAS_GAIN_2_7
	[NRF_SAADC_GAIN2_7] = {.mul = 7, .div = 2},
#endif
#if NRF_SAADC_HAS_GAIN_1_3
	[NRF_SAADC_GAIN1_3] = {.mul = 3, .div = 1},
#endif
#if NRF_SAADC_HAS_GAIN_2_5
	[NRF_SAADC_GAIN2_5] = {.mul = 5, .div = 2},
#endif
#if NRF_SAADC_HAS_GAIN_1_2
	[NRF_SAADC_GAIN1_2] = {.mul = 2, .div = 1},
#endif
#if NRF_SAADC_HAS_GAIN_2_3
	[NRF_SAADC_GAIN2_3] = {.mul = 3, .div = 2},
#endif
	[NRF_SAADC_GAIN1] = {.mul = 1, .div = 1},
	[NRF_SAADC_GAIN2] = {.mul = 1, .div = 2},
#if NRF_SAADC_HAS_GAIN_4	
	[NRF_SAADC_GAIN4] = {.mul = 1, .div = 4},
#endif
};

static int get_number_of_resolution_bits(uint8_t *number_of_resolution_bits, nrf_saadc_resolution_t resolution)
{
	switch (resolution) {
	case NRF_SAADC_RESOLUTION_8BIT:
		*number_of_resolution_bits = 8;
		break;
	case NRF_SAADC_RESOLUTION_10BIT:
		*number_of_resolution_bits = 10;
		break;
	case NRF_SAADC_RESOLUTION_12BIT:
		*number_of_resolution_bits = 12;
		break;
	case NRF_SAADC_RESOLUTION_14BIT:
		*number_of_resolution_bits = 14;
		break;
	default:
		LOG_ERR("ADC resolution value %d is not valid register value", resolution);
		return -EINVAL;
	}
	return 0;
}

static inline int adc_raw_to_millivolts(int32_t ref_mv, nrf_saadc_gain_t gain, uint8_t resolution,
					int32_t *valp)
{
	int err = 0;
	int64_t adc_mv = (int64_t)*valp * (int64_t)ref_mv;

	if ((uint8_t)gain < ARRAY_SIZE(gains)) {
		const struct gain_desc *gdp = &gains[gain];

		__ASSERT_NO_MSG(gdp->mul != 0);
		__ASSERT_NO_MSG(gdp->div != 0);
		adc_mv = (gdp->mul * adc_mv) / gdp->div;
		err = 0;
	}

	if (err == 0) {
		adc_mv = adc_mv >> resolution;
		if (adc_mv > INT32_MAX || adc_mv < INT32_MIN) {
			__ASSERT_MSG_INFO("conversion result is out of range");
		}

		*valp = (int32_t)adc_mv;
	}
	return err;
}

struct vbat_data {
	int16_t raw;
#if VBAT_TRIGGER_SUPPORT
	const struct sensor_trigger *trigger;
	sensor_trigger_handler_t trigger_handler;
#endif /* VBAT_TRIGGER_SUPPORT */
};

static void saadc_handler(nrfx_saadc_evt_t const * p_event)
{
    int status;
    (void)status;

    switch (p_event->type)
    {
        case NRFX_SAADC_EVT_DONE:
            LOG_INF("SAADC event: DONE");

            // samples_number = p_event->data.done.size;
            // for (uint16_t i = 0; i < samples_number; i++)
            // {
            //     LOG_INF("[Sample %d] value == %d",
            //                   i, NRFX_SAADC_SAMPLE_GET(p_event->data.done.p_buffer, i));
            // }

            // m_saadc_ready = true;
            break;

        case NRFX_SAADC_EVT_CALIBRATEDONE:
            LOG_INF("SAADC event: CALIBRATEDONE");
            // status = nrfx_saadc_mode_trigger();
            // NRFX_ASSERT(status == 0);
            break;

        default:
            break;
    }
}

static int vbat_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int err = 0;

	if ((chan != SENSOR_CHAN_VOLTAGE) && (chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	err = nrfx_saadc_offset_calibrate(NULL);;
	if (err != 0) {
		LOG_ERR("Calibration failed: %d", err);
		return err;
	}

	err = nrfx_saadc_mode_trigger();
	if (err != 0) {
		LOG_ERR("adc read failed: %d", err);
		return err;
	}

	return err;
}

static int vbat_channel_get(const struct device *dev, enum sensor_channel chan,
			    struct sensor_value *val)
{
	const struct vbat_config *const cfg = dev->config;
	struct vbat_data *data = dev->data;
	int32_t val_mv;
	int err = 0;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	val_mv = data->raw;

	uint8_t resolution = 0;
	err = get_number_of_resolution_bits(&resolution, nrf_saadc_resolution_get(NRF_SAADC));
	if (err != 0) {
		LOG_ERR("get_resolution failed: %d", err);
	}

	/*
	 * For differential channels, one bit less needs to be specified
	 * for resolution to achieve correct conversion.
	 */
	if(cfg->pseln != NRFX_ANALOG_INPUT_DISABLED) {
		resolution -= 1U;
	}

	err = adc_raw_to_millivolts(VDD_VOLTAGE, CHANNEL_GAIN,resolution, &val_mv);
	if (err != 0) {
		LOG_ERR("adc_raw_to_millivolts_dt failed: %d", err);
		return err;
	}
	if(cfg->invert_voltage) {
		val_mv = -val_mv;
	}
#if VBAT_TRIGGER_SUPPORT
	if ((cfg->min_threshold_mv >= 0) && (data->trigger_handler != NULL)) {
		if (val_mv < cfg->min_threshold_mv) {
				data->trigger_handler(dev, data->trigger);
		}
	}
#endif /* VBAT_TRIGGER_SUPPORT */

	LOG_DBG("raw %" PRIu16 ", %" PRIi32 " uV", data->raw, val_mv);
	return sensor_value_from_milli(val, val_mv);
}

#if VBAT_TRIGGER_SUPPORT
static int vbat_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			    sensor_trigger_handler_t handler)
{
	const struct vbat_config *const cfg = dev->config;
	struct vbat_data *data = dev->data;

	__ASSERT_NO_MSG(trig != NULL);

	if (cfg->min_threshold_mv < 0) {
		return -ENOTSUP;
	}

	if ((handler != NULL) && (trig == NULL)) {
		return -EINVAL;
	}

	if (trig->type == SENSOR_TRIG_THRESHOLD && \
		trig->chan == SENSOR_CHAN_VOLTAGE) {
		data->trigger_handler = handler;
		data->trigger = trig;
		return 0;
	}

	return -ENOTSUP;
}
#endif
static DEVICE_API(sensor, vbat_driver_api) = {
	.sample_fetch = vbat_sample_fetch,
	.channel_get = vbat_channel_get,
#if VBAT_TRIGGER_SUPPORT
	.trigger_set = vbat_trigger_set,
#endif /* VBAT_TRIGGER_SUPPORT */
};

static int vbat_init(const struct device *dev)
{
	const struct vbat_config *const cfg = dev->config;
	struct vbat_data *data = dev->data;
	int err;

	// Connect ADC interrupt to nrfx interrupt handler
    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(adc)),
                DT_IRQ(DT_NODELABEL(adc), priority),
                nrfx_saadc_irq_handler, 0, 0);

	err = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);
	if (err != 0) {
        printk("nrfx_saadc_init error: %08x", err);
        return err;
    }

	nrfx_saadc_channel_t vbat_channel = NRFX_SAADC_DEFAULT_CHANNEL_DIFFERENTIAL(cfg->pselp, cfg->pseln, 0);

	vbat_channel.channel_config.gain = CHANNEL_GAIN;
    err = nrfx_saadc_channels_config(&vbat_channel, 1);
    if (err != 0) {
        printk("nrfx_saadc_channels_config error: %08x", err);
        return err;
    }

	err = nrfx_saadc_simple_mode_set(BIT(0),
                                     NRF_SAADC_RESOLUTION_10BIT,
                                     NRF_SAADC_OVERSAMPLE_DISABLED,
                                     saadc_handler); //SHOULD BE (data->trigger_handler) pass handler for threshold IRQs (data->trigger_handler)
    if (err != 0) {
        printk("nrfx_saadc_simple_mode_set error: %08x", err);
        return err;
    }

    err = nrfx_saadc_buffer_set(&data->raw, 1);
    if (err != 0) {
        printk("nrfx_saadc_buffer_set error: %08x", err);
        return err;
    }

	// TODO: configure threshold IRQ

	return 0;
}

#define NRF_VBAT_INIT(inst)                                                \
    static struct vbat_data vbat_data_##inst;                              \
                                                                           \
    static const struct vbat_config vbat_cfg_##inst = {                    \
        .pselp = DT_INST_PROP(inst, pselp),   \
		.pseln = DT_INST_PROP_OR(inst, pseln,NRFX_ANALOG_INPUT_DISABLED),   \
		.min_threshold_mv = DT_INST_PROP_OR(inst, min_threshold_mv, -1),   \
		.invert_voltage = DT_INST_PROP_OR(inst, invert_voltage, false),	   \
    };                                                                     \
                                                                           \
    SENSOR_DEVICE_DT_INST_DEFINE(inst, vbat_init, NULL,                           \
                          &vbat_data_##inst, &vbat_cfg_##inst,             \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,        \
                          &vbat_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NRF_VBAT_INIT)
