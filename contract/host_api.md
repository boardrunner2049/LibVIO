# Host functions for device models

These functions are provided by the emulator<!-- vio -->, not by LibVIO<!-- /vio -->: FastDyn's QEMU
exports them to the device models it loads. `<fastdyn/device_model.h>`
declares them (it includes QEMU's `qemu-plugin.h`), so a model does not
include anything else for them. They act on the first (and only) CPU.

## Logging

- `host->log(FD_LOG_LEVEL_INFO, dev->name, "configured %d channels", n)`: Log a message through the host services the model receives in `init` (declared in `<fastdyn/device_model.h>`). Levels are `FD_LOG_LEVEL_TRACE`, `_DEBUG`, `_INFO`, `_WARNING` and `_ERROR`. Messages appear in FastDyn's log as `fastdyn.<device>|LEVEL|  message`, so FastDyn's `-q`, `-v` and `--log-level` options apply to them.

## Guest memory

- `int qemu_plugin_write_memory(unsigned long long addr, uint8_t *mem_buf, int len)`: Write `len` bytes from `mem_buf` to guest address `addr` (Cortex-M has no MMU, so this is the physical address). Returns **0 on success** and non-zero on error, not a byte count. RAM only: an access to an address inside the SoC's MMIO window is blocked by QEMU's re-entrancy guard (QEMU warns `Blocked re-entrant IO`).
- `int qemu_plugin_read_memory(unsigned long long addr, uint8_t *mem_buf, int len)`: Read `len` bytes at guest address `addr` into `mem_buf`. Same return value and RAM-only restriction as `qemu_plugin_write_memory`.

## CPU registers

- `void qemu_plugin_set_register(uint8_t *mem_buf, int reg)`: Write register number `reg` (the GDB numbering: on ARM, 0-15 are R0-R15) from `mem_buf`, in target byte order (4 bytes on 32-bit ARM).

## Interrupts

- `void qemu_plugin_raise_irq(int irq, int secure)`: Raise interrupt line `irq`. On Cortex-M, `irq` is the exception number, which is the device's IRQ number **plus 16** (e.g. IRQ 37 is `37 + 16`). Pass `secure = 0` (non-secure).

## Time and timers

- `int64_t qemu_plugin_get_virtual_timer(void)`: Return the guest's virtual clock in nanoseconds. It only advances at translation-block boundaries, so repeated calls during back-to-back MMIO accesses can return the same value.
- `uint64_t qemu_plugin_timer_new_ns(void (*cb)(void *), void *data)`: Create a one-shot timer on that clock and return its handle; it is not armed yet.
- `void qemu_plugin_timer_alarm(uint64_t timer_fd, uint64_t delay_ns)`: Arm the timer to call `cb(data)` once at the **absolute** virtual time `delay_ns` (despite the name), e.g. `qemu_plugin_get_virtual_timer() + 1000000` for 1 ms from now.
- `uint64_t qemu_plugin_timer_new_period_ns(void (*cb)(void *), void *data, uint64_t period)`: Create and arm a timer that calls `cb(data)` every `period` nanoseconds. At most 10 periodic timers can exist; QEMU exits if a model asks for more.

Timer callbacks run at translation-block boundaries, not during a run of
MMIO accesses. If firmware polls a register in a tight loop, virtual time
barely moves and an alarm may not fire before the loop ends. For
compare-match or timeout logic, also check the condition in the read
callback of the polled register and act immediately when it is met.
