/*
 * ADC measurement sample with work queue.
 *
 * - Performs an ADC measurement in a work queue item scheduled every 5 seconds.
 * - Updates a global variable with the latest ADC value (raw and millivolts).
 * - If the ADC value (mV) is below a configured threshold, triggers a
 *   low-threshold callback (e.g. for alert/interrupt handling).
 * - After each measurement, schedules the same work again to repeat.
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};

/* Delay between measurements */
#define MEASUREMENT_INTERVAL_SEC 5

/* Threshold in millivolts: trigger callback when ADC (mV) is below this */
#define ADC_LOW_THRESHOLD_MV 1500

/** Global variable: latest ADC value in millivolts (updated by work item). */
volatile int32_t g_adc_value_mv;

/** Global variable: latest raw ADC value (updated by work item). */
volatile int32_t g_adc_value_raw;

/** Set when ADC value is below threshold (can be used like an interrupt flag). */
volatile bool g_adc_below_threshold;

/** Optional: count how many times low threshold was triggered. */
volatile uint32_t g_adc_low_trigger_count;

static int32_t adc_buffer;
static struct adc_sequence sequence = {
	.buffer = &adc_buffer,
	.buffer_size = sizeof(adc_buffer),
#if CONFIG_SAMPLE_ADC_CALIBRATE_REQUIRED
    .calibrate = true,
#endif
};

static void adc_work_handler(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(adc_work, adc_work_handler);

/**
 * Called when ADC value is below threshold (treat as software "interrupt" handler).
 * Can be extended to set a GPIO, wake a thread, or raise a poll signal.
 */
static void adc_low_threshold_trigger(void)
{
	g_adc_below_threshold = true;
	g_adc_low_trigger_count++;
	printk("ADC below threshold: %d mV (raw %d)\n",
	       (int)g_adc_value_mv, (int)g_adc_value_raw);
}

static int adc_setup()
{
    int err = 0;
    /* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		printk("Is ready %s \n", adc_channels[i].dev->name);
		if (!adc_is_ready_dt(&adc_channels[i])) {
			printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return 0;
		}
		printk("Configure %s \n", adc_channels[i].dev->name);
		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel #%d (%d)\n", i, err);
			return 0;
		}
	}
    return 0;
}

static void adc_work_handler(struct k_work *work)
{
	int err;

	(void)work;

    for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
        int32_t val_mv;

        printk("- %s, channel %d: ",
                adc_channels[i].dev->name,
                adc_channels[i].channel_id);

        (void)adc_sequence_init_dt(&adc_channels[i], &sequence);

        err = adc_read_dt(&adc_channels[i], &sequence);
        if (err < 0) {
            printk("Could not read (%d)\n", err);
            continue;
        }

        val_mv = adc_buffer;

        printk("%"PRId32, val_mv);
        err = adc_raw_to_millivolts_dt(&adc_channels[i],
                            &val_mv);
        /* conversion to mV may not be supported, skip if not */
        if (err < 0) {
            printk(" (value in mV not available)\n");
        } else {
            printk(" = %"PRId32" mV\n", val_mv);
        }
    }
    k_work_schedule(&adc_work, K_MSEC(CONFIG_VBAT_MONITOR_INTERVAL_MS));

// 	/* Update global variable (raw) */
// 	g_adc_value_raw = adc_buffer;

// 	/* Convert to millivolts and update global */
// 	ret = adc_raw_to_millivolts_dt(&adc_channel, &adc_buffer);
// 	if (ret == 0) {
// 		g_adc_value_mv = adc_buffer;
// 	} else {
// 		g_adc_value_mv = 0;
// 	}

// 	/* If below threshold, trigger "interrupt" (callback) */
// 	if (g_adc_value_mv < ADC_LOW_THRESHOLD_MV) {
// 		adc_low_threshold_trigger();
// 	} else {
// 		g_adc_below_threshold = false;
// 	}

// 	printk("ADC: %d mV (raw %d)%s\n",
// 	       (int)g_adc_value_mv, (int)g_adc_value_raw,
// 	       g_adc_below_threshold ? " [LOW]" : "");

// reschedule:
// 	/* Schedule this work again in 5 seconds to repeat the process */
// 	k_work_schedule(&adc_work, K_SECONDS(MEASUREMENT_INTERVAL_SEC));
}



int health_monitoring_setup(void)
{

	printk("ADC work queue sample: measure every %d s, low threshold %d mV\n",
	       MEASUREMENT_INTERVAL_SEC, ADC_LOW_THRESHOLD_MV);

	adc_setup();

	/* Start the cycle: first run in 5 seconds */
	k_work_schedule(&adc_work, K_MSEC(CONFIG_VBAT_MONITOR_INTERVAL_MS));

	// for (;;) {
	// 	k_sleep(K_SECONDS(10));
	// 	/* Optional: print global state from main loop */
	// 	printk("Global ADC: %d mV, low_trigger_count %u\n",
	// 	       (int)g_adc_value_mv, (unsigned)g_adc_low_trigger_count);
	// }

	return 0;
}
