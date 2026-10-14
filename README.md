# LibVIO

LibVIO is the C library that device models use to connect emulated
peripherals to virtual I/O: PTYs, files, network (TAP) devices, signals,
FIFOs, DMA, ADC, and SPI / I2C buses with pluggable slave devices.

The code was imported from FastDyn (`boardrunner/boardrunner_sdk`, commit
`9c2e0b0`) and builds on its own; it does not need FastDyn or QEMU.

Documentation: [docs/API.md](docs/API.md) explains how to use it;
[contract/api_reference.md](contract/api_reference.md) lists every function;
[CHANGELOG.md](CHANGELOG.md) lists the changes since the FastDyn copy.

## Build

Requirements: a C11 compiler, CMake 3.16 or newer, and pkg-config for
the install check.

```bash
scripts/build.sh                     # configure, build and test in ./build
scripts/build.sh --prefix ~/.local   # ... and install
```

or with CMake directly:

```bash
cmake -S . -B build && cmake --build build && ctest --test-dir build
cmake --install build --prefix <dir>
```

The install contains `libvio.so`, the headers under `include/vio/`,
`vio.pc`, the CMake package `vio` (target `vio::vio`) and the model
contract under `share/vio/contract/`.

## Use

```c
#include <vio/vio.h>
```

```bash
cc model.c $(pkg-config --cflags --libs vio)
```

```cmake
find_package(vio 1.0 REQUIRED)
target_link_libraries(model PRIVATE vio::vio)
```

## SPI and I2C buses

A bus model creates its bus from its device's JSON config, which lists the
attached slave models:

```c
SPIBus *bus = vio_spi_bus_init(config_json);
/* {"slaves": [{"device": "flash", "cs_id": 0, "device_scroll": "flash.so"}]} */

I2CBus *bus = vio_i2c_bus_init(config_json);
/* {"slaves": [{"device": "bme280", "address": "0x76", "device_scroll": "bme280.so"}]} */
```

Every slave needs `device` and `device_scroll`, plus `cs_id` (SPI) or
`address` (I2C, an integer or a string such as `"0x76"`, up to `0x3FF`).
Optional keys: `config`, an object passed to the slave; `model`; `mode`
and `max_hz` (SPI); `speed` and `addr_width` (I2C). Unknown or duplicate keys, a repeated chip select or
address, and a slave model that cannot be loaded are errors: the function
returns `NULL` and logs which entry and key are wrong. Other top-level keys
belong to the model and are ignored.

### Slave models

A slave model is a shared library that implements `include/vio/slave.h`:
it exports `vio_slave_abi` and fixed functions such as
`vio_i2c_slave_init(name, config_json)`, which returns the instance state,
and `vio_i2c_slave_send(ctx, data)`. All state lives in that `ctx`, so one
library can serve several slaves, on one bus or on several.

## Model contract

`contract/` describes the functions a device model can call, for tools
that generate models: the LibVIO API (`api_reference.md`, and
`api_summary.md` with one line per function for prompts, both generated
from the headers, each section tagged with who uses it; run
`scripts/gen_api_reference.py` after changing a header comment, and see
"Documenting the API" in `docs/API.md` for the comment format), the
emulator's host functions (`host_api.md`) and the contract `VERSION`. How
to write a model (rules, examples, prompts) is up to those tools. They find
the contract with `pkg-config --variable=contractdir vio`.

## Logging

LibVIO writes diagnostics to stderr as `vio.<subsystem>|LEVEL|  message`.
A host can take them over with `vio_set_logger()` and filter them with
`vio_set_log_level()` (`include/vio/log.h`).

## License

LibVIO is licensed under the [Apache License 2.0](LICENSE). Copyright 2026
PSecLab. It includes cJSON under the MIT License; see [NOTICE](NOTICE).
