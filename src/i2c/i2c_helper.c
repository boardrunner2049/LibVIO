#include "i2c_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "../internal.h"
#include "../slave/slave_loader.h"


// Internal helpers (not exposed to model authors)
// Helper functions
/**
 * @brief Scans the bus for a slave with the given address.
 * @param bus A pointer to the I2CBus.
 * @param address The slave address (without the R/W bit).
 * @param ten_bit Whether this is a 10-bit address; 7- and 10-bit slaves never match each other.
 * @param found_dev A pointer-to-a-pointer, which will be updated to point to the found slave.
 * @return true if the slave was found, false otherwise.
 */
bool i2c_scan_bus(I2CBus *bus, uint16_t address, bool ten_bit, SlaveDetails **found_dev) {
    for (int i = 0; i < bus->Slaves.num_slaves; i++) {
        SlaveDetails *curr_slave = &bus->Slaves.slave[i];
        if (curr_slave->ten_bit == ten_bit && curr_slave->address == address) {
            *found_dev = curr_slave;
            return true;
        }
    }

    *found_dev = NULL; // Ensure the output pointer is NULL if not found
    return false; // Failure
}

/**
 * @brief (Internal) Starts a transaction with a slave device on the bus.
 * @param bus A pointer to the I2CBus.
 * @param address The slave address (without the R/W bit).
 * @param ten_bit Whether `address` is a 10-bit address.
 * @param event The event to send (I2C_START_SEND or I2C_START_RECV).
 * @return 0 on success (ACK), non-zero on failure (NACK).
 */
int i2c_do_start_transfer(I2CBus *bus, uint16_t address, bool ten_bit, enum i2c_event event) {
    // If no device is currently active, scan for the requested one.
    // A repeated START keeps the active device, even for another address.
    if (!bus->current_dev) {
        bool found = i2c_scan_bus(bus, address, ten_bit, &bus->current_dev);
        if (!found) {
            return 1; // Device not found on bus, return NACK/failure
        }
    }

    int event_status = bus->current_dev->event(bus->current_dev->ctx, event);

    return event_status;
}

bool i2c_attach_slave(I2CBus *bus, const char *name, uint16_t address, bool ten_bit,
                      const char *path, const char *config_json, const char *where) {
    where = where ? where : "i2c";
    if (!bus || !name || !path) {
        VIO_ERROR("i2c", "%s: invalid arguments passed to i2c_attach_slave", where);
        return false;
    }

    void *handle = vio_slave_open("i2c", path, where);
    if (!handle) {
        return false;
    }
    /* Stop at the first missing symbol, so only one error is logged. */
    VioSlaveInitFunc init = NULL;
    SlaveSendFunc send = NULL;
    SlaveRecvFunc recv = NULL;
    SlaveEventFunc event = NULL;
    if (!(init = (VioSlaveInitFunc)vio_slave_symbol(handle, "i2c", "vio_i2c_slave_init", path, where)) ||
        !(send = (SlaveSendFunc)vio_slave_symbol(handle, "i2c", "vio_i2c_slave_send", path, where)) ||
        !(recv = (SlaveRecvFunc)vio_slave_symbol(handle, "i2c", "vio_i2c_slave_recv", path, where)) ||
        !(event = (SlaveEventFunc)vio_slave_symbol(handle, "i2c", "vio_i2c_slave_event", path, where))) {
        dlclose(handle);
        return false;
    }
    VioSlaveFreeFunc free_ctx = (VioSlaveFreeFunc)dlsym(handle, "vio_slave_free");

    char *slave_name = strdup(name);
    SlaveDetails *slaves = realloc(bus->Slaves.slave,
                                   (bus->Slaves.num_slaves + 1) * sizeof(SlaveDetails));
    if (slaves) {
        bus->Slaves.slave = slaves;
    }
    if (!slave_name || !slaves) {
        VIO_ERROR("i2c", "%s: out of memory while attaching slave '%s'", where, name);
        free(slave_name);
        dlclose(handle);
        return false;
    }

    void *ctx = init(name, config_json ? config_json : "{}");
    if (!ctx) {
        VIO_ERROR("i2c", "%s: slave '%s' refused its configuration (init returned NULL)", where, name);
        free(slave_name);
        dlclose(handle);
        return false;
    }

    SlaveDetails *new_slave = &slaves[bus->Slaves.num_slaves++];
    memset(new_slave, 0, sizeof(*new_slave));
    new_slave->name = slave_name;
    new_slave->address = address;
    new_slave->ten_bit = ten_bit;
    new_slave->send = send;
    new_slave->recv = recv;
    new_slave->event = event;
    new_slave->ctx = ctx;
    new_slave->free = free_ctx;
    new_slave->handle = handle;
    return true;
}

void i2c_release_slave(SlaveDetails *slave) {
    if (slave->free) {
        slave->free(slave->ctx);
    }
    if (slave->handle) {
        dlclose(slave->handle);
    }
    free(slave->name);
}
