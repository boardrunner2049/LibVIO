#pragma once
#include <stdint.h>

/**
 * ADC inputs.
 *
 * An ADC model asks LibVIO for the value at the analog front-end of a given
 * ADC unit and channel instead of inventing samples. Samples are 12-bit
 * values, right-aligned in a uint16_t, so they can go straight to
 * `api_dma_request_data()` with len = 2. Units 0-3 and channels 0-19 are
 * supported; other values always give the default sample.
 */

/** Called to get the next sample of one channel; returns a 12-bit sample. */
typedef uint16_t (*adc_sample_fn)(void *opaque);

/**
 * Bind a host-side sample source to (adc_id, channel). `cb` is called with
 * `opaque` for every sample. Passing cb = NULL removes the binding.
 */
void api_adc_bind_input(int adc_id, int channel, adc_sample_fn cb, void *opaque);

/**
 * Return the current sample for (adc_id, channel): from the bound source if
 * there is one, otherwise the default sample. Never blocks.
 */
uint16_t api_adc_get_sample(int adc_id, int channel);

/**
 * Return the default sample for (adc_id, channel), ignoring any bound
 * source: about 1489 counts for channel 17 (VREFINT, 1.21 V on a 3.3 V
 * reference), about 1000 for channel 18 (temperature sensor), 0 otherwise.
 */
uint16_t api_adc_default_sample(int adc_id, int channel);
