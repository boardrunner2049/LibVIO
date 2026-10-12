/* Slave loading (src/spi/spi_helper.c, src/i2c/i2c_helper.c). */
#include "check.h"

#include "../src/i2c/i2c_internal.h"
#include "../src/spi/spi_internal.h"

int main(int argc, char **argv) {
    CHECK(argc == 3);
    const char *spi_slave = argv[1];
    const char *i2c_slave = argv[2];

    SPIBus spi = {0};
    CHECK(spi_attach_slave(&spi, "flash", 1, spi_slave, "{}", "test"));
    CHECK(spi.Slaves.num_slaves == 1);
    CHECK(spi.Slaves.slave[0].ctx && spi.Slaves.slave[0].free && spi.Slaves.slave[0].handle);
    CHECK(api_spi_transfer(&spi, 0x0F) == 0xFFFFFFFFu);  /* nothing selected */
    api_spi_set_cs(&spi, 1, 0);
    CHECK(api_spi_transfer(&spi, 0x0F) == ~0x0Fu);
    api_spi_set_cs(&spi, 1, 1);
    CHECK(api_spi_transfer(&spi, 0x0F) == 0xFFFFFFFFu);

    CHECK(!spi_attach_slave(&spi, "missing", 2, "/nonexistent/slave.so", "{}", "test"));
    CHECK(!spi_attach_slave(&spi, "wrong", 2, i2c_slave, "{}", "test"));  /* an I2C slave */
    CHECK(spi.Slaves.num_slaves == 1);

    I2CBus i2c = {0};
    CHECK(i2c_attach_slave(&i2c, "echo", 0x42, false, i2c_slave, NULL, "test"));  /* NULL config: "{}" */
    CHECK(api_i2c_start_transfer(&i2c, 0x42, false) == 0);
    CHECK(api_i2c_send(&i2c, 7) == 0);
    api_i2c_end_transfer(&i2c);
    CHECK(api_i2c_start_transfer(&i2c, 0x42, true) == 0);
    CHECK(api_i2c_recv(&i2c) == 8);
    api_i2c_end_transfer(&i2c);
    CHECK(api_i2c_start_transfer(&i2c, 0x10, false) != 0);  /* no such address */

    CHECK(!i2c_attach_slave(&i2c, "refused", 0x43, false, i2c_slave, "{\"refuse\":true}", "test"));
    CHECK(!i2c_attach_slave(&i2c, "wrong", 0x43, false, spi_slave, "{}", "test"));  /* an SPI slave */
    CHECK(i2c.Slaves.num_slaves == 1);

    for (int i = 0; i < spi.Slaves.num_slaves; i++) {
        spi_release_slave(&spi.Slaves.slave[i]);
    }
    free(spi.Slaves.slave);
    for (int i = 0; i < i2c.Slaves.num_slaves; i++) {
        i2c_release_slave(&i2c.Slaves.slave[i]);
    }
    free(i2c.Slaves.slave);

    printf("test_slaves passed\n");
    return 0;
}
