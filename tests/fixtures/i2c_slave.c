/* I2C slave used by the tests: returns the last byte sent plus "offset"
 * from its config (default 1). Refuses a config containing "refuse". */
#include <vio/slave.h>

#include <stdlib.h>
#include <string.h>

const int vio_slave_abi = VIO_SLAVE_ABI_VERSION;

typedef struct {
    uint8_t last;
    int offset;
} State;

void *vio_i2c_slave_init(const char *name, const char *config_json) {
    (void)name;
    if (strstr(config_json, "refuse")) {
        return NULL;
    }
    State *s = calloc(1, sizeof(*s));
    if (!s) {
        return NULL;
    }
    const char *o = strstr(config_json, "\"offset\":");
    s->offset = o ? atoi(o + 9) : 1;
    return s;
}

int vio_i2c_slave_send(void *ctx, uint8_t data) {
    ((State *)ctx)->last = data;
    return 0;
}

uint8_t vio_i2c_slave_recv(void *ctx) {
    State *s = ctx;
    return (uint8_t)(s->last + s->offset);
}

int vio_i2c_slave_event(void *ctx, enum i2c_event event) {
    (void)ctx;
    (void)event;
    return 0;
}

void vio_slave_free(void *ctx) {
    free(ctx);
}
