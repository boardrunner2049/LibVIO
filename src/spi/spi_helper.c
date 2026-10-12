#include "spi_internal.h"
#include "../internal.h"
#include "../slave/slave_loader.h"

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

bool spi_attach_slave(SPIBus *bus, const char *name, int cs_id, const char *path,
                      const char *config_json, const char *where) {
    where = where ? where : "spi";
    if (!bus || !name || !path) {
        VIO_ERROR("spi", "%s: invalid arguments passed to spi_attach_slave", where);
        return false;
    }

    void *handle = vio_slave_open("spi", path, where);
    if (!handle) {
        return false;
    }
    /* Stop at the first missing symbol, so only one error is logged. */
    VioSlaveInitFunc init = NULL;
    SlaveTransferFunc transfer = NULL;
    SlaveSetcsFunc set_cs = NULL;
    if (!(init = (VioSlaveInitFunc)vio_slave_symbol(handle, "spi", "vio_spi_slave_init", path, where)) ||
        !(transfer = (SlaveTransferFunc)vio_slave_symbol(handle, "spi", "vio_spi_slave_transfer", path, where)) ||
        !(set_cs = (SlaveSetcsFunc)vio_slave_symbol(handle, "spi", "vio_spi_slave_set_cs", path, where))) {
        dlclose(handle);
        return false;
    }
    VioSlaveFreeFunc free_ctx = (VioSlaveFreeFunc)dlsym(handle, "vio_slave_free");

    char *slave_name = strdup(name);
    SpiSlaveDetails *slaves = realloc(bus->Slaves.slave,
                                      (bus->Slaves.num_slaves + 1) * sizeof(SpiSlaveDetails));
    if (slaves) {
        bus->Slaves.slave = slaves;
    }
    if (!slave_name || !slaves) {
        VIO_ERROR("spi", "%s: out of memory while attaching slave '%s'", where, name);
        free(slave_name);
        dlclose(handle);
        return false;
    }

    void *ctx = init(name, config_json ? config_json : "{}");
    if (!ctx) {
        VIO_ERROR("spi", "%s: slave '%s' refused its configuration (init returned NULL)", where, name);
        free(slave_name);
        dlclose(handle);
        return false;
    }

    SpiSlaveDetails *new_slave = &slaves[bus->Slaves.num_slaves++];
    memset(new_slave, 0, sizeof(*new_slave));
    new_slave->name = slave_name;
    new_slave->cs_id = cs_id;
    new_slave->cs_enable = 0;
    new_slave->transfer = transfer;
    new_slave->set_cs = set_cs;
    new_slave->ctx = ctx;
    new_slave->free = free_ctx;
    new_slave->handle = handle;
    return true;
}

void spi_release_slave(SpiSlaveDetails *slave) {
    if (slave->free) {
        slave->free(slave->ctx);
    }
    if (slave->handle) {
        dlclose(slave->handle);
    }
    free(slave->name);
}
