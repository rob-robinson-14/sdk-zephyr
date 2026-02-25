#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <nrfx_saadc.h>
#include "battery_monitor.h"

LOG_MODULE_REGISTER(battery_monitoring, CONFIG_HEALTH_MONITOR_LOG_LEVEL);

#define BATTERY_CHANNEL_ID 	0
#define VDD_VOLTAGE 		900
/* TOOD: Map Raw value from mv and gain / resolution */
#define VBAT_MONITOR_LOWER_THRESHOLD_RAW CONFIG_VBAT_MONITOR_LOWER_THRESHOLD_MV

/** @brief global variable to store battery voltage */
static atomic_t battery_voltage_raw;


const nrf_saadc_channel_config_t channel_config = {.gain = NRF_SAADC_GAIN1_4,
						   .reference = NRF_SAADC_REFERENCE_INTERNAL,
						   .acq_time = NRFX_SAADC_DEFAULT_ACQTIME,
						   .mode = NRF_SAADC_MODE_SINGLE_ENDED,
						   .burst = NRF_SAADC_BURST_DISABLED};

/** @brief SAADC channel to measure battery level */
const nrfx_saadc_channel_t m_multiple_channels[] = {
	{/* TODO: If def around for measure battery and temp ADC
	  * Need to update to correct pins.
	  * Also need to set Channel to measure batt (instead of external pin),
	  * gain and divider must also be set
	  */
		.channel_index 	= BATTERY_CHANNEL_ID,
		.pin_n = NRFX_ANALOG_INPUT_DISABLED,
		.pin_p = NRFX_ANALOG_EXTERNAL_AIN4,
		.channel_config = channel_config },
};
#define CHANNEL_COUNT NRFX_ARRAY_SIZE(m_multiple_channels)

/** @brief Samples buffer defined with the size of @ref CHANNEL_COUNT
 * symbol to store values from each channel ( @ref m_multiple_channels).
 */
static nrf_saadc_value_t m_samples_buffer[CHANNEL_COUNT];

static void battery_voltage_work_handler(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(get_battery_voltage, battery_voltage_work_handler);

/**
 * @brief Function for getting pointer to vbatt.
 *
 * @return Pointer to atomic variable holding battery voltage
 */
atomic_t *get_battery_voltage_pointer(void)
{
	return &battery_voltage_raw;
}

/**
 * @brief Function in the event battery voltage is below minimum threshold
 *
 */
__weak void battery_below_threshold(void)
{
	LOG_WRN("IRQ LIMITL triggered on battery voltage");
	LOG_WRN("Placeholder -  customer should implement their own requirements");
}

/**
 * @brief Function for handling SAADC driver events.
 *
 * @param[in] p_event Pointer to an SAADC driver event.
 */
static void saadc_handler(nrfx_saadc_evt_t const *p_event)
{
int status;
	uint16_t samples_number;
	/* Temp value allows observability of functionality for reading data */
	static int32_t voltage_temp;

	switch (p_event->type) {
	case NRFX_SAADC_EVT_DONE:
		LOG_INF("SAADC event: DONE");
		samples_number = p_event->data.done.size;
		for (uint16_t i = 0; i < samples_number; i++) {
			atomic_set(&battery_voltage_raw, 
				NRFX_SAADC_SAMPLE_GET(p_event->data.done.p_buffer, i));
			LOG_INF("[Raw %d] value == %ld", i, atomic_get(&battery_voltage_raw));
			/* TODO: Map values using resolution and gain not magic numbers */
			int battery_voltage_mv = ((VDD_VOLTAGE*4) * 
			atomic_get(&battery_voltage_raw)) / ((1<<12));

			LOG_INF("[Voltage %d] value == %d mV", i, battery_voltage_mv);
		}
		atomic_set(&battery_voltage_raw, voltage_temp);
		voltage_temp++;
		k_work_schedule(&get_battery_voltage,
			K_MSEC(CONFIG_VBAT_MONITOR_INTERVAL_MS));
		break;

	case NRFX_SAADC_EVT_CALIBRATEDONE:
		LOG_INF("SAADC event: CALIBRATEDONE");
		status = nrfx_saadc_mode_trigger();
		NRFX_ASSERT(status == 0);
		break;

	case NRFX_SAADC_EVT_LIMIT:
		if (p_event->data.limit.limit_type == NRF_SAADC_LIMIT_LOW) {
			if (p_event->data.limit.channel == BATTERY_CHANNEL_ID) {
				battery_below_threshold();
			}
		} else {
			LOG_WRN("Undefined Threshold event triggered");
		}
		break;

	default:
		break;
	}
}

int battery_monitoring_setup(void)
{
	int err = 0;

	// TODO: This should cause error from CMakeLists.txt
	IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_SAADC), IRQ_PRIO_LOWEST, nrfx_saadc_irq_handler, 0, 0);

	err = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);
	if (err != 0) {
		LOG_ERR("SAADC init failed");
		return err;
	}

	err = nrfx_saadc_channels_config(m_multiple_channels, CHANNEL_COUNT);
	if (err != 0) {
		LOG_ERR("SAADC channel config failed");
		return err;
	}

	err = nrfx_saadc_simple_mode_set(nrfx_saadc_channels_configured_get(),
					 NRF_SAADC_RESOLUTION_12BIT, NRF_SAADC_OVERSAMPLE_DISABLED,
					 saadc_handler);
	if (err != 0) {
		LOG_ERR("SAADC mode setting failed");
		return err;
	}

	err = nrfx_saadc_buffer_set(m_samples_buffer, CHANNEL_COUNT);
	if (err != 0) {
		LOG_ERR("Setting SAADC buffer failed");
		return err;
	}

	err = nrfx_saadc_limits_set(BATTERY_CHANNEL_ID, VBAT_MONITOR_LOWER_THRESHOLD_RAW,
				    INT16_MAX);
	if (err != 0) {
		LOG_ERR("Setting voltage limits failed failed");
		return err;
	}

	LOG_INF("SAADC Configured");
	k_work_schedule(&get_battery_voltage, K_MSEC(CONFIG_VBAT_MONITOR_INTERVAL_MS));
	return err;
}

static void battery_voltage_work_handler(struct k_work *work)
{
	int err;

	(void)work;

	err = nrfx_saadc_offset_calibrate(saadc_handler);
	if (err != 0) {
		LOG_ERR("Offset calibration failed");
	}
	LOG_INF("Calibration Requested");
}
