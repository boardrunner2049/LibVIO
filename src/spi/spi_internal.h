#pragma once
#include <vio/spi.h>

//Warn: Do we need the following function? Not defined anywhere
// uint32_t spi_transfer_raw_default(SlaveDetails curr_slave, uint32_t value); //For passed slave, check cs_enable and based on that sends the value to slave.

/* Load the slave model at `path` (see <vio/slave.h>), create an instance
 * with `config_json` and attach it to `bus` on chip select `cs_id`. Returns
 * false, with the bus unchanged, if the model cannot be used; error messages
 * start with `where` (the config entry). */
bool spi_attach_slave(SPIBus *bus, const char *name, int cs_id, const char *path,
                      const char *config_json, const char *where);

/* Free the instance, unload the model and free the name. */
void spi_release_slave(SpiSlaveDetails *slave);
