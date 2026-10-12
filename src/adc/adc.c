#include <vio/adc.h>

#define MAX_ADC_UNITS    4
#define MAX_ADC_CHANNELS 20   /* covers external ch0..15 and internal 17, 18 */

typedef struct {
    adc_sample_fn cb;
    void         *opaque;
} AdcBinding;

static AdcBinding bindings[MAX_ADC_UNITS][MAX_ADC_CHANNELS];

static int in_range(int adc_id, int channel) {
    return adc_id >= 0 && adc_id < MAX_ADC_UNITS
        && channel >= 0 && channel < MAX_ADC_CHANNELS;
}

void api_adc_bind_input(int adc_id, int channel, adc_sample_fn cb, void *opaque) {
    if (!in_range(adc_id, channel)) return;
    bindings[adc_id][channel].cb     = cb;
    bindings[adc_id][channel].opaque = opaque;
}

uint16_t api_adc_default_sample(int adc_id, int channel) {
    (void)adc_id;
    switch (channel) {
        case 17: return 1489;   /* VREFINT: 1.21 V on 3.3 V ref, 12-bit */
        case 18: return 1000;   /* Temperature sensor mid-range */
        default: return 0;
    }
}

uint16_t api_adc_get_sample(int adc_id, int channel) {
    if (!in_range(adc_id, channel)) {
        return api_adc_default_sample(adc_id, channel);
    }
    AdcBinding *b = &bindings[adc_id][channel];
    if (b->cb) return b->cb(b->opaque);
    return api_adc_default_sample(adc_id, channel);
}
