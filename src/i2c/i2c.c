#include "i2c_internal.h"
#include "../config/bus_config.h"
#include "../internal.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

//These are the apis exposed to the user.

/**
 * @brief Starts an I2C transaction with a specific slave device.
 *
 * This function serves as the public API to initiate a transaction. It finds the
 * target slave on the bus using its 7-bit address and signals the start of
 * either a master-send (write) or master-receive (read) operation.
 *
 * @param bus       A pointer to the I2CBus instance being operated on.
 * @param address   The 7-bit address of the target slave device.
 * @param is_recv   The direction of the transfer: 'true' for a read operation
 * (master receives), 'false' for a write operation (master sends).
 * @return          0 on success (the slave acknowledged the address), or a non-zero
 * value on failure (slave not found or did not acknowledge).
 */
int api_i2c_start_transfer(I2CBus* bus, uint8_t address, bool is_recv) {
    return i2c_do_start_transfer(bus, address, false, is_recv
                                               ? I2C_START_RECV
                                               : I2C_START_SEND);
}
int api_i2c_start_transfer_10bit(I2CBus* bus, uint16_t address, bool is_recv) {
    return i2c_do_start_transfer(bus, address, true, is_recv
                                               ? I2C_START_RECV
                                               : I2C_START_SEND);
}
/**
 * @brief Ends the current I2C transaction and resets the bus's active device.
 *
 * This function should be called after a STOP condition. It notifies the
 * currently active slave that the transfer is finished and then sets the
 * bus's current_dev pointer to NULL, making the bus ready for a new transaction.
 *
 * @param bus A pointer to the I2CBus structure.
 */
void api_i2c_end_transfer(I2CBus* bus) {
    if (bus->current_dev) {
        (void)bus->current_dev->event(bus->current_dev->ctx, I2C_FINISH);
        bus->current_dev = NULL;
    }
}

/**
 * @brief Sends a single data byte to the currently active slave device.
 * @param bus  A pointer to the I2CBus instance.
 * @param data The byte of data to send.
 * @return 0 on success (slave ACKed), or -1 on failure (slave NACKed or no active device).
 */
int api_i2c_send(I2CBus *bus, uint8_t data)
{
    // 1. Check if a transaction is active by checking the pointer itself.
    if (!bus->current_dev) {
        return -1; // No active device to send to.
    }

    // 2. Call the slave's send function using the correct syntax ('->').
    //    Pass the ADDRESS of the data to match the uint8_t* signature.
    int ret = bus->current_dev->send(bus->current_dev->ctx, data);

    // 3. Return 0 for success (if ret is 0), or -1 for failure (if ret is non-zero).
    return (ret == 0) ? 0 : -1;
}

/**
 * @brief Receives a single data byte from the currently active slave device.
 * @param bus A pointer to the I2CBus instance.
 * @return The byte of data received from the slave. Returns 0xFF if no
 * transaction is active.
 */
uint8_t api_i2c_recv(I2CBus *bus)
{
    // Check if a transaction is active.
    if (bus->current_dev) {
        // Call the slave's recv function and return the byte.
        return bus->current_dev->recv(bus->current_dev->ctx);
    }

    // If no device is active, return a default "bus high" value.
    return 0xFF;
}

I2CBus *vio_i2c_bus_init(const char *config_json) {
    VioBusConfig config;
    char err[256];
    if (!vio_bus_config_parse(config_json, VIO_BUS_I2C, &config, err, sizeof(err))) {
        VIO_ERROR("i2c", "invalid bus config: %s", err);
        return NULL;
    }

    I2CBus *bus = calloc(1, sizeof(*bus));
    if (!bus) {
        VIO_ERROR("i2c", "out of memory");
        vio_bus_config_free(&config);
        return NULL;
    }
    for (int i = 0; i < config.count; i++) {
        const VioSlaveConfig *slave = &config.slaves[i];
        char where[48];
        snprintf(where, sizeof(where), "slaves[%d].device_scroll", i);
        if (!i2c_attach_slave(bus, slave->device, slave->address, slave->ten_bit, slave->device_scroll,
                              slave->config_json, where)) {
            vio_i2c_bus_free(bus);
            vio_bus_config_free(&config);
            return NULL;
        }
    }
    vio_bus_config_free(&config);
    return bus;
}

void vio_i2c_bus_free(I2CBus *bus) {
    if (!bus) {
        return;
    }
    for (int i = 0; i < bus->Slaves.num_slaves; i++) {
        i2c_release_slave(&bus->Slaves.slave[i]);
    }
    free(bus->Slaves.slave);
    free(bus);
}
