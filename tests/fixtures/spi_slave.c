/* SPI slave used by the tests: while selected, returns value ^ xor, where
 * "xor" comes from its config (default 0xFFFFFFFF). */
#include <vio/slave.h>

#include <stdlib.h>
#include <string.h>

#ifndef FIXTURE_ABI
#define FIXTURE_ABI VIO_SLAVE_ABI_VERSION
#endif

const int vio_slave_abi = FIXTURE_ABI;

typedef struct {
    int selected;
    uint32_t xor_value;
} State;

void *vio_spi_slave_init(const char *name, const char *config_json) {
    (void)name;
    State *s = calloc(1, sizeof(*s));
    if (!s) {
        return NULL;
    }
    const char *x = strstr(config_json, "\"xor\":");
    s->xor_value = x ? (uint32_t)strtoul(x + 6, NULL, 0) : 0xFFFFFFFFu;
    return s;
}

uint32_t vio_spi_slave_transfer(void *ctx, uint32_t value) {
    State *s = ctx;
    return s->selected ? (value ^ s->xor_value) : 0xFFFFFFFFu;
}

void vio_spi_slave_set_cs(void *ctx, int level) {
    ((State *)ctx)->selected = (level == 0);
}

void vio_slave_free(void *ctx) {
    free(ctx);
}
