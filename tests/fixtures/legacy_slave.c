/* A slave in the old, pre-ABI style (global state, no vio_slave_abi). */
#include <stdint.h>

uint32_t slave_spi_transfer(uint32_t value) {
    return ~value;
}

void slave_spi_set_cs(int level) {
    (void)level;
}
