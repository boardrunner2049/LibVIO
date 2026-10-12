#pragma once

#include <stdint.h>

#include <vio/i2c.h>

/**
 * The interface a slave model (a shared library) implements for LibVIO's SPI
 * and I2C buses.
 *
 * A slave exports `vio_slave_abi` and functions with fixed names. Every
 * function after init receives the `ctx` that init returned, so one library
 * can serve any number of slaves on any number of buses: keep all state in
 * ctx, never in globals. A slave links against nothing from the bus model.
 *
 * ```c
 * #include <vio/slave.h>
 *
 * const int vio_slave_abi = VIO_SLAVE_ABI_VERSION;
 *
 * void *vio_i2c_slave_init(const char *name, const char *config_json) {
 *     return calloc(1, sizeof(struct my_state));
 * }
 * ```
 */

/** The slave interface version described here. */
#define VIO_SLAVE_ABI_VERSION 1

/**
 * Every slave defines this as VIO_SLAVE_ABI_VERSION; LibVIO refuses slaves
 * without it or with another version.
 */
extern const int vio_slave_abi;

/**
 * Called to create an SPI slave instance. `name` is the slave's "device" and
 * `config_json` its "config" object from the bus config ("{}" if it has
 * none); both are valid only during the call. Returns the instance state,
 * or NULL to refuse the configuration (bus creation then fails).
 */
void *vio_spi_slave_init(const char *name, const char *config_json);

/**
 * Called to exchange one word while the slave is selected: `value` is what
 * the controller sends (MOSI). Returns the reply (MISO).
 */
uint32_t vio_spi_slave_transfer(void *ctx, uint32_t value);

/** Called when the chip-select line changes: `level` 0 selects, 1 deselects. */
void vio_spi_slave_set_cs(void *ctx, int level);

/** Like `vio_spi_slave_init()`, for an I2C slave instance. */
void *vio_i2c_slave_init(const char *name, const char *config_json);

/**
 * Called with one byte from the controller. Returns 0 to ACK, non-zero to
 * NACK.
 */
int vio_i2c_slave_send(void *ctx, uint8_t data);

/** Called when the controller reads a byte. Returns that byte. */
uint8_t vio_i2c_slave_recv(void *ctx);

/**
 * Called on a bus event for this slave: I2C_START_RECV or I2C_START_SEND
 * (return 0 to ACK the address), I2C_FINISH at STOP, or I2C_NACK.
 */
int vio_i2c_slave_event(void *ctx, enum i2c_event event);

/** Optional, SPI and I2C: called when the bus is freed, to release `ctx`. */
void vio_slave_free(void *ctx);
