# Changelog

## 1.0.1 (2026-10-14)

* Copyright holder in `NOTICE` and the README is PSecLab. No code changes.

## 1.0.0 (2026-10-12)

First version as a standalone library, from FastDyn's
`boardrunner/boardrunner_sdk` (commit `9c2e0b0`).

* Builds and installs on its own: `libvio.so`, headers under `include/vio/`
  (previously `include/boardrunner/`), `vio.pc`, the CMake package
  `vio::vio`, `scripts/build.sh`. FastDyn and QEMU are no longer needed to
  build it.
* SPI and I2C buses are created from the device's JSON config with
  `vio_spi_bus_init` / `vio_i2c_bus_init`, which validate it strictly and
  report errors with their location. This replaces `api_spi_init_bus` and
  `api_i2c_init_bus`, which read FastDyn's internal `ConfigSection`.
* New slave model interface (`<vio/slave.h>`): fixed symbol names, an
  instance pointer for every call, an ABI version marker, and a per-slave
  `config` object. One slave library can be attached any number of times.
  Slaves in the old style are refused.
* I2C: 7-bit and 10-bit addresses are matched exactly and kept apart;
  before, a 10-bit address matched any slave with the same low 7 bits.
* Logging through `vio_set_logger` / `vio_set_log_level`, with a stderr
  default, instead of `printf` / `perror`. The per-transfer DMA dump is a
  DEBUG message.
* Licensed under the Apache License 2.0 (`LICENSE`, `NOTICE`), installed to
  `share/doc/vio/`.
* Model contract under `contract/`: the functions a device model can call,
  for tools that generate models: a generated API reference and summary,
  the host functions (including logging), and its version (2.0.0),
  installed to `share/vio/contract/`. How to write a model is left to
  those tools.
