#include "slave_loader.h"
#include "../internal.h"

#include <vio/slave.h>

#include <dlfcn.h>
#include <stddef.h>

void *vio_slave_open(const char *subsystem, const char *path, const char *where) {
    void *handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        VIO_ERROR(subsystem, "%s: unable to open slave model '%s': %s", where, path, dlerror());
        return NULL;
    }
    const int *abi = dlsym(handle, "vio_slave_abi");
    if (!abi) {
        VIO_ERROR(subsystem,
                  "%s: '%s' is not a LibVIO slave (it does not export vio_slave_abi); "
                  "see <vio/slave.h>",
                  where, path);
        dlclose(handle);
        return NULL;
    }
    if (*abi != VIO_SLAVE_ABI_VERSION) {
        VIO_ERROR(subsystem, "%s: '%s' was built for slave ABI %d, but this LibVIO expects %d",
                  where, path, *abi, VIO_SLAVE_ABI_VERSION);
        dlclose(handle);
        return NULL;
    }
    return handle;
}

void *vio_slave_symbol(void *handle, const char *subsystem, const char *symbol,
                       const char *path, const char *where) {
    void *address = dlsym(handle, symbol);
    if (!address) {
        VIO_ERROR(subsystem, "%s: slave model '%s' does not export %s", where, path, symbol);
    }
    return address;
}
