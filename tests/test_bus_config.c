/* Validation of the "slaves" bus config (src/config/bus_config.c). */
#include "check.h"

#include "../src/config/bus_config.h"

#include <string.h>

typedef struct {
    VioBusKind kind;
    const char *json;
    const char *error; /* NULL: must be accepted */
} Case;

#define SPI_SLAVE(extra) "{\"device\": \"flash\", \"device_scroll\": \"f.so\", \"cs_id\": 0" extra "}"
#define I2C_SLAVE(extra) "{\"device\": \"bme280\", \"device_scroll\": \"b.so\", \"address\": \"0x76\"" extra "}"

static const Case cases[] = {
    /* accepted */
    {VIO_BUS_SPI, "{}", NULL},
    {VIO_BUS_SPI, "{\"slaves\": [], \"baud\": 9600, \"anything\": {\"x\": 1}}", NULL},
    {VIO_BUS_SPI, "{\"slaves\": [" SPI_SLAVE(", \"model\": [\"elder\"], \"mode\": 3, \"max_hz\": 1e6") "]}", NULL},
    {VIO_BUS_I2C, "{\"slaves\": [" I2C_SLAVE(", \"model\": \"elder\", \"speed\": 400000") "]}", NULL},
    {VIO_BUS_I2C, "{\"slaves\": [{\"device\": \"d\", \"device_scroll\": \"d.so\", \"address\": 783, \"addr_width\": 10}]}", NULL},
    {VIO_BUS_I2C, "{\"slaves\": [{\"device\": \"bme-280 #2\", \"device_scroll\": \"b.so\", \"address\": 1, \"config\": {}}]}", NULL},

    /* JSON shape */
    {VIO_BUS_SPI, "{\"slaves\": [", "config: invalid JSON at offset"},
    {VIO_BUS_SPI, "[]", "config: must be a JSON object"},
    {VIO_BUS_SPI, "{\"slaves\": {}}", "slaves: must be an array"},
    {VIO_BUS_SPI, "{\"slaves\": [], \"slaves\": []}", "slaves: duplicate key"},
    {VIO_BUS_SPI, "{\"slaves\": [1]}", "slaves[0]: must be an object"},

    /* unknown and duplicate keys */
    {VIO_BUS_SPI, "{\"slaves\": [" SPI_SLAVE(", \"cs\": 1") "]}", "slaves[0].cs: unknown key for an spi slave"},
    {VIO_BUS_SPI, "{\"slaves\": [" SPI_SLAVE(", \"address\": 1") "]}", "slaves[0].address: unknown key for an spi slave"},
    {VIO_BUS_I2C, "{\"slaves\": [" I2C_SLAVE(", \"cs_id\": 1") "]}", "slaves[0].cs_id: unknown key for an i2c slave"},
    {VIO_BUS_SPI, "{\"slaves\": [" SPI_SLAVE(", \"cs_id\": 1") "]}", "slaves[0].cs_id: duplicate key"},

    /* missing fields */
    {VIO_BUS_SPI, "{\"slaves\": [{\"device_scroll\": \"f.so\", \"cs_id\": 0}]}", "slaves[0]: missing required key 'device'"},
    {VIO_BUS_SPI, "{\"slaves\": [{\"device\": \"f\", \"cs_id\": 0}]}", "slaves[0]: missing required key 'device_scroll'"},
    {VIO_BUS_SPI, "{\"slaves\": [{\"device\": \"f\", \"device_scroll\": \"f.so\"}]}", "slaves[0]: missing required key 'cs_id'"},
    {VIO_BUS_I2C, "{\"slaves\": [{\"device\": \"b\", \"device_scroll\": \"b.so\"}]}", "slaves[0]: missing required key 'address'"},

    /* wrong types and values */
    {VIO_BUS_SPI, "{\"slaves\": [{\"device\": \"\", \"device_scroll\": \"f.so\", \"cs_id\": 0}]}", "slaves[0].device: must be a non-empty string"},
    {VIO_BUS_SPI, "{\"slaves\": [{\"device\": \"f\", \"device_scroll\": 5, \"cs_id\": 0}]}", "slaves[0].device_scroll: must be a non-empty string"},
    {VIO_BUS_SPI, "{\"slaves\": [{\"device\": \"f\", \"device_scroll\": \"f.so\", \"cs_id\": \"0\"}]}", "slaves[0].cs_id: must be an integer"},
    {VIO_BUS_SPI, "{\"slaves\": [{\"device\": \"f\", \"device_scroll\": \"f.so\", \"cs_id\": 1.5}]}", "slaves[0].cs_id: must be an integer"},
    {VIO_BUS_SPI, "{\"slaves\": [{\"device\": \"f\", \"device_scroll\": \"f.so\", \"cs_id\": -1}]}", "slaves[0].cs_id: must be an integer"},
    {VIO_BUS_SPI, "{\"slaves\": [" SPI_SLAVE(", \"mode\": 4") "]}", "slaves[0].mode: must be an integer from 0 to 3"},
    {VIO_BUS_SPI, "{\"slaves\": [" SPI_SLAVE(", \"model\": [1]") "]}", "slaves[0].model: must be a string or an array of strings"},
    {VIO_BUS_I2C, "{\"slaves\": [{\"device\": \"b\", \"device_scroll\": \"b.so\", \"address\": \"0x76z\"}]}", "slaves[0].address: must be an integer or a string"},
    {VIO_BUS_I2C, "{\"slaves\": [{\"device\": \"b\", \"device_scroll\": \"b.so\", \"address\": 1024}]}", "slaves[0].address: must be an integer or a string"},
    {VIO_BUS_I2C, "{\"slaves\": [" I2C_SLAVE(", \"addr_width\": 8") "]}", "slaves[0].addr_width: must be 7 or 10"},
    {VIO_BUS_I2C, "{\"slaves\": [{\"device\": \"b\", \"device_scroll\": \"b.so\", \"address\": \"0x80\", \"addr_width\": 7}]}", "slaves[0].address: 0x80 does not fit in 7 bits"},
    {VIO_BUS_SPI, "{\"slaves\": [" SPI_SLAVE(", \"config\": [1]") "]}", "slaves[0].config: must be an object"},

    /* duplicates across slaves */
    {VIO_BUS_SPI, "{\"slaves\": [" SPI_SLAVE("") ", {\"device\": \"other\", \"device_scroll\": \"o.so\", \"cs_id\": 0}]}",
     "slaves[1].cs_id: duplicate chip select 0 (also used by slaves[0])"},
    {VIO_BUS_I2C, "{\"slaves\": [" I2C_SLAVE("") ", {\"device\": \"other\", \"device_scroll\": \"o.so\", \"address\": 118}]}",
     "slaves[1].address: duplicate 7-bit address 0x76 (also used by slaves[0])"},
    {VIO_BUS_I2C, "{\"slaves\": [{\"device\": \"a\", \"device_scroll\": \"a.so\", \"address\": \"0x30F\"}, "
                  "{\"device\": \"b\", \"device_scroll\": \"b.so\", \"address\": 783, \"addr_width\": 10}]}",
     "slaves[1].address: duplicate 10-bit address 0x30F (also used by slaves[0])"},

    /* 7- and 10-bit addresses are separate address spaces. */
    {VIO_BUS_I2C, "{\"slaves\": [{\"device\": \"a\", \"device_scroll\": \"a.so\", \"address\": \"0x0F\"}, "
                  "{\"device\": \"b\", \"device_scroll\": \"b.so\", \"address\": \"0x30F\"}, "
                  "{\"device\": \"c\", \"device_scroll\": \"c.so\", \"address\": \"0x0F\", \"addr_width\": 10}]}",
     NULL},
};

int main(void) {
    int failures = 0;
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const Case *c = &cases[i];
        VioBusConfig config;
        char err[256] = "";
        bool ok = vio_bus_config_parse(c->json, c->kind, &config, err, sizeof(err));
        bool passed = c->error ? (!ok && strncmp(err, c->error, strlen(c->error)) == 0) : ok;
        if (!passed) {
            fprintf(stderr, "case %zu: %s\n  expected: %s\n  got:      %s\n", i, c->json,
                    c->error ? c->error : "accepted", ok ? "accepted" : err);
            failures++;
        }
        if (ok) {
            vio_bus_config_free(&config);
        } else {
            CHECK(config.slaves == NULL && config.count == 0);
        }
    }
    CHECK(failures == 0);

    /* Parsed values. */
    VioBusConfig config;
    char err[256];
    CHECK(vio_bus_config_parse("{\"slaves\": [" I2C_SLAVE("") ", {\"device\": \"t\", \"device_scroll\": \"t.so\", \"address\": 72}]}",
                               VIO_BUS_I2C, &config, err, sizeof(err)));
    CHECK(config.count == 2);
    CHECK(strcmp(config.slaves[0].device, "bme280") == 0);
    CHECK(strcmp(config.slaves[0].device_scroll, "b.so") == 0);
    CHECK(config.slaves[0].address == 0x76);
    CHECK(config.slaves[1].address == 72);
    CHECK(!config.slaves[0].ten_bit && !config.slaves[1].ten_bit);
    CHECK(strcmp(config.slaves[0].config_json, "{}") == 0);
    vio_bus_config_free(&config);

    CHECK(vio_bus_config_parse("{\"slaves\": [" SPI_SLAVE(", \"config\": {\"rate\": 2, \"tags\": [\"a\"]}") "]}",
                               VIO_BUS_SPI, &config, err, sizeof(err)));
    CHECK(strcmp(config.slaves[0].config_json, "{\"rate\":2,\"tags\":[\"a\"]}") == 0);
    vio_bus_config_free(&config);

    CHECK(vio_bus_config_parse("{\"slaves\": [{\"device\": \"a\", \"device_scroll\": \"a.so\", \"address\": \"0x30F\"}, "
                               "{\"device\": \"b\", \"device_scroll\": \"b.so\", \"address\": 15, \"addr_width\": 10}, "
                               "{\"device\": \"c\", \"device_scroll\": \"c.so\", \"address\": 15, \"addr_width\": 7}]}",
                               VIO_BUS_I2C, &config, err, sizeof(err)));
    CHECK(config.slaves[0].ten_bit && config.slaves[0].address == 0x30F);
    CHECK(config.slaves[1].ten_bit && config.slaves[1].address == 0x0F);
    CHECK(!config.slaves[2].ten_bit && config.slaves[2].address == 0x0F);
    vio_bus_config_free(&config);

    CHECK(!vio_bus_config_parse(NULL, VIO_BUS_SPI, &config, err, sizeof(err)));

    printf("test_bus_config passed (%zu cases)\n", sizeof(cases) / sizeof(cases[0]));
    return 0;
}
