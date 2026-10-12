/* Loading slave model libraries (see <vio/slave.h>). */
#pragma once

#include <stdbool.h>

/* dlopen `path` and check that it exports vio_slave_abi with the expected
 * version. Logs under `subsystem`, prefixed with `where`, and returns NULL
 * on failure. */
void *vio_slave_open(const char *subsystem, const char *path, const char *where);

/* Look up a required symbol; logs and returns NULL if it is missing. */
void *vio_slave_symbol(void *handle, const char *subsystem, const char *symbol,
                       const char *path, const char *where);
