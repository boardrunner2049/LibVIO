/* vio_spi_bus_init / vio_i2c_bus_init with real slave models. */
#include "check.h"

#include <vio/vio.h>

#include <string.h>

static char last_error[512];

static void capture(int level, const char *subsystem, const char *message, void *user) {
    (void)user;
    if (level >= VIO_LOG_ERROR) {
        snprintf(last_error, sizeof(last_error), "%s: %s", subsystem, message);
    }
}

static uint8_t i2c_round_trip(I2CBus *bus, uint8_t address, uint8_t value) {
    CHECK(api_i2c_start_transfer(bus, address, false) == 0);
    CHECK(api_i2c_send(bus, value) == 0);
    api_i2c_end_transfer(bus);
    CHECK(api_i2c_start_transfer(bus, address, true) == 0);
    uint8_t result = api_i2c_recv(bus);
    api_i2c_end_transfer(bus);
    return result;
}

int main(int argc, char **argv) {
    CHECK(argc == 5);
    const char *spi_slave = argv[1];
    const char *i2c_slave = argv[2];
    const char *legacy_slave = argv[3];
    const char *abi2_slave = argv[4];
    char json[2048];
    vio_set_logger(capture, NULL);

    /* One SPI slave model as two instances with their own config. */
    snprintf(json, sizeof(json),
             "{\"slaves\": ["
             "{\"model\": [\"elder\"], \"device\": \"a\", \"cs_id\": 0, \"device_scroll\": \"%s\", "
             "\"config\": {\"xor\": 255}},"
             "{\"device\": \"b\", \"cs_id\": 1, \"device_scroll\": \"%s\", \"config\": {\"xor\": 15}}"
             "], \"baud\": 9600}",
             spi_slave, spi_slave);
    SPIBus *spi = vio_spi_bus_init(json);
    CHECK(spi && spi->Slaves.num_slaves == 2);
    CHECK(spi->Slaves.slave[0].ctx != spi->Slaves.slave[1].ctx);
    api_spi_set_cs(spi, 0, 0);
    CHECK(api_spi_transfer(spi, 0xF0) == (0xF0u ^ 255u));
    api_spi_set_cs(spi, 0, 1);
    api_spi_set_cs(spi, 1, 0);
    CHECK(api_spi_transfer(spi, 0xF0) == (0xF0u ^ 15u));
    vio_spi_bus_free(spi);

    /* One I2C slave model as two instances: independent state and config. */
    snprintf(json, sizeof(json),
             "{\"slaves\": ["
             "{\"device\": \"low\", \"address\": \"0x42\", \"device_scroll\": \"%s\"},"
             "{\"device\": \"high\", \"address\": \"0x43\", \"device_scroll\": \"%s\", "
             "\"config\": {\"offset\": 10}}"
             "]}",
             i2c_slave, i2c_slave);
    I2CBus *i2c = vio_i2c_bus_init(json);
    CHECK(i2c && i2c->Slaves.num_slaves == 2);
    CHECK(i2c_round_trip(i2c, 0x42, 7) == 8);
    CHECK(i2c_round_trip(i2c, 0x43, 20) == 30);
    CHECK(api_i2c_start_transfer(i2c, 0x42, true) == 0);
    CHECK(api_i2c_recv(i2c) == 8);  /* still 7 + 1: not overwritten by the other instance */
    api_i2c_end_transfer(i2c);
    vio_i2c_bus_free(i2c);

    /* Two buses sharing one slave model. */
    snprintf(json, sizeof(json),
             "{\"slaves\": [{\"device\": \"s\", \"address\": 1, \"device_scroll\": \"%s\"}]}", i2c_slave);
    I2CBus *bus1 = vio_i2c_bus_init(json);
    I2CBus *bus2 = vio_i2c_bus_init(json);
    CHECK(bus1 && bus2);
    CHECK(i2c_round_trip(bus1, 1, 40) == 41);
    CHECK(i2c_round_trip(bus2, 1, 50) == 51);
    CHECK(api_i2c_start_transfer(bus1, 1, true) == 0);
    CHECK(api_i2c_recv(bus1) == 41);
    api_i2c_end_transfer(bus1);
    vio_i2c_bus_free(bus1);
    CHECK(i2c_round_trip(bus2, 1, 60) == 61);  /* still loaded for bus2 */
    vio_i2c_bus_free(bus2);

    /* A bus without slaves. */
    spi = vio_spi_bus_init("{}");
    CHECK(spi && spi->Slaves.num_slaves == 0);
    CHECK(api_spi_transfer(spi, 1) == 0xFFFFFFFFu);
    vio_spi_bus_free(spi);

    /* Invalid config: NULL, and the reason is logged. */
    CHECK(vio_spi_bus_init("{\"slaves\": [{\"device\": \"f\", \"cs_id\": 0}]}") == NULL);
    CHECK(strcmp(last_error, "spi: invalid bus config: slaves[0]: missing required key 'device_scroll'") == 0);

    /* Slave models that cannot be used. */
    CHECK(vio_i2c_bus_init("{\"slaves\": [{\"device\": \"x\", \"address\": 1, "
                           "\"device_scroll\": \"/nonexistent/x.so\"}]}") == NULL);
    CHECK(strstr(last_error, "i2c: slaves[0].device_scroll: unable to open slave model") == last_error);

    snprintf(json, sizeof(json),
             "{\"slaves\": [{\"device\": \"old\", \"cs_id\": 0, \"device_scroll\": \"%s\"}]}", legacy_slave);
    CHECK(vio_spi_bus_init(json) == NULL);
    CHECK(strstr(last_error, "is not a LibVIO slave (it does not export vio_slave_abi)") != NULL);

    snprintf(json, sizeof(json),
             "{\"slaves\": [{\"device\": \"new\", \"cs_id\": 0, \"device_scroll\": \"%s\"}]}", abi2_slave);
    CHECK(vio_spi_bus_init(json) == NULL);
    CHECK(strstr(last_error, "was built for slave ABI 2, but this LibVIO expects 1") != NULL);

    snprintf(json, sizeof(json),
             "{\"slaves\": [{\"device\": \"r\", \"address\": 1, \"device_scroll\": \"%s\", "
             "\"config\": {\"refuse\": true}}]}",
             i2c_slave);
    CHECK(vio_i2c_bus_init(json) == NULL);
    CHECK(strcmp(last_error, "i2c: slaves[0].device_scroll: slave 'r' refused its configuration "
                             "(init returned NULL)") == 0);

    /* The second slave fails: the first is released and the bus is NULL. */
    snprintf(json, sizeof(json),
             "{\"slaves\": [{\"device\": \"a\", \"cs_id\": 0, \"device_scroll\": \"%s\"}, "
             "{\"device\": \"b\", \"cs_id\": 1, \"device_scroll\": \"%s\"}]}",
             spi_slave, i2c_slave);
    CHECK(vio_spi_bus_init(json) == NULL);
    CHECK(strstr(last_error, "slaves[1].device_scroll: slave model") != NULL);
    CHECK(strstr(last_error, "does not export vio_spi_slave_init") != NULL);

    vio_set_logger(NULL, NULL);
    printf("test_bus_init passed\n");
    return 0;
}
