#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * SPI buses with attached slave models.
 *
 * An SPI controller model creates its bus once, from its device config, and
 * then forwards chip-select changes and data words to it. The slaves are
 * separate models (see <vio/slave.h>).
 */

#define NUM_CS_LINES 4          //USER can change the max number based on the requirement

/* Slave model callbacks; see <vio/slave.h>. */
#ifndef VIO_SLAVE_FREE_FUNC_DEFINED
#define VIO_SLAVE_FREE_FUNC_DEFINED
typedef void *(*VioSlaveInitFunc)(const char *name, const char *config_json);
typedef void (*VioSlaveFreeFunc)(void *ctx);
#endif
typedef uint32_t (*SlaveTransferFunc)(void *ctx, uint32_t value);
typedef void (*SlaveSetcsFunc)(void *ctx, int level);

typedef struct {
    char* name;                 //Name of the slave
    int cs_id;                  //Chip select id for which the device will be registered
    int cs_enable;              //is the current chip enable
    SlaveTransferFunc transfer; //transfer function used to send and receive the data to/from slave
    SlaveSetcsFunc set_cs;      //function to tell the slave it is selected
    void *ctx;                  //instance state returned by the slave's init
    VioSlaveFreeFunc free;      //optional: releases ctx
    void *handle;               //the slave model's library handle
} SpiSlaveDetails;

typedef struct {
    int num_slaves;
    SpiSlaveDetails* slave; //Dynamic array of slaves
} SpiSlaveList;

/**
 * An SPI bus. Models keep the pointer returned by `vio_spi_bus_init()` and
 * pass it to the functions below; they do not need its fields.
 */
typedef struct {
    SpiSlaveList Slaves;  //Registered on spi_init_bus
} SPIBus;

/**
 * Create a bus from the device's JSON config. Its "slaves" array lists the
 * attached slaves, e.g.
 * `{"slaves": [{"device": "flash", "cs_id": 0, "device_scroll": "flash.so"}]}`.
 * Each slave needs "device", "device_scroll" and "cs_id"; optional keys are
 * "config" (an object passed to the slave's init), "model", "mode" (0-3) and
 * "max_hz". Other top-level keys are ignored. Returns NULL, after logging
 * the reason, if the config is invalid or a slave cannot be loaded.
 */
SPIBus *vio_spi_bus_init(const char *config_json);

/** Free the bus and its slaves. NULL is allowed. */
void vio_spi_bus_free(SPIBus *bus);

/**
 * Send `val` (MOSI) to the selected slave and return its reply (MISO).
 * Returns 0xFFFFFFFF, an idle line, if no slave is selected.
 */
uint32_t api_spi_transfer(SPIBus *bus, uint32_t val);

/**
 * Drive chip-select line `cs_id` to `level`. Chip selects are active-low:
 * level 0 selects the slaves on that line, 1 deselects them. Each affected
 * slave is notified.
 */
void api_spi_set_cs(SPIBus *bus, int cs_id, int level);
