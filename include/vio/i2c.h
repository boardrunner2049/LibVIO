#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * I2C buses with attached slave models.
 *
 * An I2C controller model creates its bus once, from its device config, and
 * then forwards START, data and STOP to it. The slaves are separate models
 * (see <vio/slave.h>). 7-bit and 10-bit addresses are separate address
 * spaces.
 */

/** Bus events passed to a slave. */
enum i2c_event {
    I2C_START_RECV,
    I2C_START_SEND,
    I2C_FINISH,
    I2C_NACK
};

/* Slave model callbacks; see <vio/slave.h>. */
#ifndef VIO_SLAVE_FREE_FUNC_DEFINED
#define VIO_SLAVE_FREE_FUNC_DEFINED
typedef void *(*VioSlaveInitFunc)(const char *name, const char *config_json);
typedef void (*VioSlaveFreeFunc)(void *ctx);
#endif
typedef int     (*SlaveSendFunc)(void *ctx, uint8_t data);
typedef uint8_t (*SlaveRecvFunc)(void *ctx);
typedef int     (*SlaveEventFunc)(void *ctx, enum i2c_event event);

typedef struct {
    char* name;
    uint16_t address;      // 7-bit, or 10-bit when ten_bit is set
    bool ten_bit;          // answers 10-bit starts only (api_i2c_start_transfer_10bit)
    SlaveSendFunc send;
    SlaveRecvFunc recv;
    SlaveEventFunc event;
    void *ctx;              // instance state returned by the slave's init
    VioSlaveFreeFunc free;  // optional: releases ctx
    void *handle;           // the slave model's library handle
} SlaveDetails;

typedef struct {
    int num_slaves;
    SlaveDetails* slave;
} SlaveList;

/**
 * An I2C bus. Models keep the pointer returned by `vio_i2c_bus_init()` and
 * pass it to the functions below; they do not need its fields.
 */
typedef struct {
    SlaveList Slaves;
    SlaveDetails* current_dev;
    uint8_t saved_address;
} I2CBus;

/**
 * Create a bus from the device's JSON config. Its "slaves" array lists the
 * attached slaves, e.g.
 * `{"slaves": [{"device": "bme280", "address": "0x76", "device_scroll": "bme280.so"}]}`.
 * Each slave needs "device", "device_scroll" and "address" (an integer or a
 * string such as "0x76", up to 0x3FF; addresses above 0x7F are 10-bit).
 * Optional keys are "config" (an object passed to the slave's init),
 * "model", "speed" and "addr_width" (7 or 10). Other top-level keys are
 * ignored. Returns NULL, after logging the reason, if the config is invalid
 * or a slave cannot be loaded.
 */
I2CBus *vio_i2c_bus_init(const char *config_json);

/** Free the bus and its slaves. NULL is allowed. */
void vio_i2c_bus_free(I2CBus *bus);

/**
 * Send a START to the 7-bit slave at `address`, to read from it if
 * `is_recv` is true, to write to it otherwise. Returns 0 if the slave ACKed,
 * non-zero if no slave answered. A repeated START keeps talking to the
 * slave of the current transfer.
 */
int api_i2c_start_transfer(I2CBus *bus, uint8_t address, bool is_recv);

/** Like `api_i2c_start_transfer()`, for the 10-bit slave at `address`. */
int api_i2c_start_transfer_10bit(I2CBus *bus, uint16_t address, bool is_recv);

/** Send a STOP, ending the current transfer (the slave gets I2C_FINISH). */
void api_i2c_end_transfer(I2CBus *bus);

/**
 * Send one byte to the slave of the current transfer. Returns 0 if it
 * ACKed, -1 if it NACKed or there is no transfer.
 */
int api_i2c_send(I2CBus *bus, uint8_t data);

/**
 * Receive one byte from the slave of the current transfer. Returns 0xFF if
 * there is no transfer.
 */
uint8_t api_i2c_recv(I2CBus *bus);
