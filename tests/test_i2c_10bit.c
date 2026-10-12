/* 7-bit and 10-bit I2C addressing (i2c_scan_bus). */
#include "check.h"

#include <vio/vio.h>

/* Write `value`, then read back: the fixture returns value + its "offset". */
static int round_trip(I2CBus *bus, uint16_t address, bool ten_bit, uint8_t value) {
    int rc = ten_bit ? api_i2c_start_transfer_10bit(bus, address, false)
                     : api_i2c_start_transfer(bus, (uint8_t)address, false);
    if (rc != 0) {
        return -1;  /* NACK */
    }
    CHECK(api_i2c_send(bus, value) == 0);
    api_i2c_end_transfer(bus);
    rc = ten_bit ? api_i2c_start_transfer_10bit(bus, address, true)
                 : api_i2c_start_transfer(bus, (uint8_t)address, true);
    CHECK(rc == 0);
    int result = api_i2c_recv(bus);
    api_i2c_end_transfer(bus);
    return result;
}

int main(int argc, char **argv) {
    CHECK(argc == 2);
    char json[1024];
    snprintf(json, sizeof(json),
             "{\"slaves\": ["
             "{\"device\": \"seven\", \"address\": \"0x0F\", \"device_scroll\": \"%s\", \"config\": {\"offset\": 1}},"
             "{\"device\": \"ten\", \"address\": \"0x30F\", \"device_scroll\": \"%s\", \"config\": {\"offset\": 10}},"
             "{\"device\": \"ten_low\", \"address\": 15, \"addr_width\": 10, \"device_scroll\": \"%s\", "
             "\"config\": {\"offset\": 20}}"
             "]}",
             argv[1], argv[1], argv[1]);
    I2CBus *bus = vio_i2c_bus_init(json);
    CHECK(bus && bus->Slaves.num_slaves == 3);

    CHECK(round_trip(bus, 0x0F, false, 5) == 6);    /* the 7-bit slave */
    CHECK(round_trip(bus, 0x30F, true, 5) == 15);   /* the 10-bit slave, not the 7-bit 0x0F */
    CHECK(round_trip(bus, 0x00F, true, 5) == 25);   /* 10-bit 0x00F is not the 7-bit 0x0F */

    CHECK(round_trip(bus, 0x10F, true, 5) == -1);   /* same low bits, no such 10-bit slave */
    CHECK(round_trip(bus, 0x8F, false, 5) == -1);   /* 7-bit 0x0F must not match 0x8F */
    CHECK(round_trip(bus, 0x70F, true, 5) == -1);   /* beyond 10 bits */

    vio_i2c_bus_free(bus);

    /* Before this fix, a 7-bit 0x0F alone also answered the 10-bit 0x30F. */
    snprintf(json, sizeof(json),
             "{\"slaves\": [{\"device\": \"seven\", \"address\": 15, \"device_scroll\": \"%s\"}]}", argv[1]);
    bus = vio_i2c_bus_init(json);
    CHECK(bus);
    CHECK(round_trip(bus, 0x30F, true, 5) == -1);
    vio_i2c_bus_free(bus);

    printf("test_i2c_10bit passed\n");
    return 0;
}
