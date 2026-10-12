/* Parsing and validation of a bus device's "slaves" configuration.
 * Pure checks (no slave is loaded), so tools can reuse them. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    VIO_BUS_SPI,
    VIO_BUS_I2C,
} VioBusKind;

typedef struct {
    char *device;        /* slave name */
    char *device_scroll; /* path of the slave model .so */
    char *config_json;   /* the slave's "config" object, "{}" if absent */
    int cs_id;           /* SPI */
    uint16_t address;    /* I2C */
    bool ten_bit;        /* I2C: addr_width 10, or an address above 0x7F */
} VioSlaveConfig;

typedef struct {
    VioSlaveConfig *slaves;
    int count;
} VioBusConfig;

/* Parse `config_json`, the device's config object. Only its "slaves" array
 * is read; other top-level keys belong to the model. On failure returns
 * false and writes a message naming the offending entry and key, e.g.
 * "slaves[1].cs_id: duplicate chip select 0 (also used by slaves[0])". */
bool vio_bus_config_parse(const char *config_json, VioBusKind kind, VioBusConfig *out,
                          char *err, size_t err_len);
void vio_bus_config_free(VioBusConfig *config);
