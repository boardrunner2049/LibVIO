#pragma once
#include <vio/i2c.h>

// Internal helpers (not exposed to model authors)
bool i2c_scan_bus(I2CBus *bus, uint16_t address, bool ten_bit, SlaveDetails **found_dev);
int  i2c_do_start_transfer(I2CBus *bus, uint16_t address, bool ten_bit, enum i2c_event event);
/* Load the slave model at `path` (see <vio/slave.h>), create an instance
 * with `config_json` and attach it to `bus` at `address` (a 10-bit address
 * when `ten_bit` is set). Returns false,
 * with the bus unchanged, if the model cannot be used; error messages start
 * with `where` (the config entry). */
bool i2c_attach_slave(I2CBus *bus, const char *name, uint16_t address, bool ten_bit,
                      const char *path, const char *config_json, const char *where);

/* Free the instance, unload the model and free the name. */
void i2c_release_slave(SlaveDetails *slave);
