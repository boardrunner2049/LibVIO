# Using LibVIO

LibVIO connects emulated peripherals (device models) to the outside world
and to each other. This guide explains how the pieces fit together; for
every function's exact signature and behaviour, see
[`contract/api_reference.md`](../contract/api_reference.md).

## Who uses what

* A **device model** emulates one peripheral (a UART, an I2C controller, a
  DMA engine). It is a shared library the emulator loads. It calls LibVIO
  (`#include <vio/vio.h>`, link with `-lvio`) and the emulator's
  [host functions](../contract/host_api.md) (guest memory, interrupts,
  timers).
* A **slave model** emulates a chip on an SPI or I2C bus (a sensor, a flash
  chip). It is a separate shared library that implements
  [`<vio/slave.h>`](../include/vio/slave.h); the bus model's LibVIO bus
  loads it.
* The **host side** (scripts, terminals, the network) talks to models
  through pseudo-terminals, named pipes, files and TAP interfaces.

## Talking to the host

| Need | API |
|---|---|
| A serial line (UART TX/RX) | `api_pty_fd_gen`, `api_pty_write_req`, `api_pty_read_nonblock` |
| A byte stream from or to a script | `api_fifo_*` |
| Persistent storage (flash, EEPROM) | `api_file_*` |
| Ethernet frames | `api_tap_*` |
| Analog inputs | `api_adc_*` |

All reads are non-blocking, so models can poll them from MMIO callbacks or
timers without stalling the guest.

## Connecting models to each other

Models never call each other's code. They meet through LibVIO:

* **Signals**: a source sets a line's level with `api_signal_set`, the sink
  registered with `api_signal_register` gets a callback when it changes
  (e.g. GPIO pin → EXTI line).
* **DMA**: a DMA model registers per-stream handlers; a peripheral triggers
  them with `api_dma_request` or, with up to 8 bytes of data,
  `api_dma_request_data`.

Handlers are called immediately, in the caller's context, with the `opaque`
pointer given at registration. Their signatures must match exactly; never
cast a function pointer to make it fit.

## SPI and I2C buses

A bus controller model creates its bus from the JSON config of its device
and keeps the pointer:

```c
typedef struct {
    I2CBus *bus;
    /* register state ... */
} I2cState;

void *my_i2c_init(const char *name, const char *config_json) {
    I2cState *s = calloc(1, sizeof(*s));
    s->bus = vio_i2c_bus_init(config_json);
    if (!s->bus) {          /* the reason has been logged */
        free(s);
        return NULL;
    }
    return s;
}
```

The config lists the slaves:

```json
{"slaves": [
    {"device": "bme280", "address": "0x76", "device_scroll": "build/bme280.so",
     "config": {"chip_id": 96}}
]}
```

Every slave needs `device` (its name), `device_scroll` (the slave model's
`.so`), and `cs_id` (SPI) or `address` (I2C; addresses above `0x7F` are
10-bit). `config` is passed to the slave. Mistakes are reported with their
location, e.g. `slaves[1].cs_id: duplicate chip select 0 (also used by
slaves[0])`, and the bus is not created.

The model then drives the bus from its register callbacks:
`api_i2c_start_transfer` / `_10bit`, `api_i2c_send`, `api_i2c_recv` and
`api_i2c_end_transfer` for I2C; `api_spi_set_cs` and `api_spi_transfer` for
SPI.

## Writing a slave model

```c
#include <vio/slave.h>
#include <stdlib.h>

const int vio_slave_abi = VIO_SLAVE_ABI_VERSION;

typedef struct { uint8_t reg, regs[256]; int have_reg; } Bme280;

void *vio_i2c_slave_init(const char *name, const char *config_json) {
    (void)name; (void)config_json;          /* parse settings here if any */
    Bme280 *s = calloc(1, sizeof(*s));
    if (s) s->regs[0xD0] = 0x60;            /* chip ID */
    return s;
}

int vio_i2c_slave_event(void *ctx, enum i2c_event event) {
    if (event == I2C_START_SEND) ((Bme280 *)ctx)->have_reg = 0;
    return 0;                               /* ACK */
}

int vio_i2c_slave_send(void *ctx, uint8_t data) {
    Bme280 *s = ctx;
    if (!s->have_reg) { s->reg = data; s->have_reg = 1; }
    else s->regs[s->reg++] = data;
    return 0;
}

uint8_t vio_i2c_slave_recv(void *ctx) {
    Bme280 *s = ctx;
    return s->regs[s->reg++];
}

void vio_slave_free(void *ctx) { free(ctx); }
```

Because all state lives in `ctx`, the same `.so` can be attached several
times (two sensors at different addresses, or on two buses).

## Logging

LibVIO reports problems on stderr as `vio.<subsystem>|LEVEL|  message`. An
emulator can route them into its own log with `vio_set_logger` and filter
them with `vio_set_log_level`.

## The model contract

`contract/` describes the functions a device model can call, for tools that
write device models, for example to put into an LLM prompt. How to write a
model (rules, examples, prompts) is up to those tools.

* `api_reference.md`: every public LibVIO function, generated from the
  header comments by `scripts/gen_api_reference.py` (a test fails when it
  is out of date). Each header's section starts with `<!-- for: model -->`,
  `<!-- for: slave -->` or `<!-- for: host -->`: whether device models,
  slave models or the emulator loading them use it, so a tool can give a
  prompt only the sections it needs. The text before the first section
  applies to all; a new header needs its audience in the script
* `api_summary.md`: the same text with one line per function, the form
  tools put into prompts (same sections and audience tags)
* `host_api.md`: the emulator's host functions models may call, one line
  per function; text between the HTML comments `<!-- vio -->` and
  `<!-- /vio -->` applies only when LibVIO is available, so a tool that
  writes models without it (e.g. an ablation) drops that text, markers
  included
* `VERSION`: the contract version, to record with each generated model

It is installed to `share/vio/contract/`; find it with
`pkg-config --variable=contractdir vio`, or `VIO_CONTRACT_DIR` after
`find_package(vio)` in CMake.

## Documenting the API

The header comments are the API documentation: `scripts/gen_api_reference.py`
turns them into `contract/api_reference.md` and `contract/api_summary.md`,
which tools put into LLM prompts. Keep them in this form, so the generated
files stay correct and read the same everywhere.

**Structure** (the generator relies on it):

* The first `/** ... */` comment in a header, followed by a blank line,
  introduces the header: one or two sentences on what it is for, with an
  example use ("Serial ports as host pseudo-terminals, e.g. for a UART's TX
  and RX lines.").
* Every public declaration has a `/** ... */` comment directly above it,
  with no blank line in between. A declaration without one is internal and
  is left out of the contract.
* A macro that callers use like a function adds `@signature <declaration>`
  as the last line of its comment; the contract shows that declaration
  instead of the macro.
* A new header needs a section title and an audience (`model`, `slave` or
  `host`) in `HEADERS` in the script, which refuses to run without them.
* Run `scripts/gen_api_reference.py` after changing a comment; the
  `api_reference_up_to_date` test fails until you do.

**Wording:**

* A function you call starts with what it does, in the imperative: "Write
  one byte to the pseudo-terminal.", "Return the file's size in bytes."
* A function or callback type that a model *implements* (handlers, the
  slave functions) starts with "Called": "Called with the new `level` of
  line `signal_id`.", "Called when the bus is freed, to release `ctx`."
* A type or constant is described with a noun phrase: "An SPI bus.",
  "Number of signal lines".
* Name parameters in backticks when their meaning is not obvious from their
  name and type; `fd`, `bus` and `ctx` need no explanation.
* Always state the return value and how errors are reported: "Returns the
  file descriptor, or -1 on error." A variant may say "Like `x()`, but ..."
  and give only the difference.
* Refer to other functions as `` `name()` ``, in backticks.
* Describe only what a caller needs: blocking or not, limits, who owns what.
  Leave out how it is implemented.
* Wrap at 80 columns, with ` * ` at the start of each line (an example that
  cannot be broken, such as a JSON config, may be longer).

```c
/**
 * Read one byte into `*buff` without blocking. Returns 1 if a byte was read,
 * 0 if none is available, -1 on error.
 */
int api_pty_read_nonblock(int fd, uint8_t *buff);

/** Called with the new `level` (true = high) of line `signal_id`. */
typedef void (*signal_handler_t)(void *opaque, int signal_id, bool level);
```
