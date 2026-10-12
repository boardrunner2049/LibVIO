#pragma once
#include <stdint.h>

/**
 * Serial ports as host pseudo-terminals, e.g. for a UART's TX and RX lines.
 */

int api_pty_fd_gen_impl(const char* dev_name);

/**
 * Open the host pseudo-terminal for the device `dev_name`, at
 * `/tmp/<dev_name>_pty` (the endpoint the host side creates), in
 * non-blocking mode. Called with no argument it opens `/tmp/usart1_pty`.
 * Returns the file descriptor, or -1 if the endpoint does not exist yet, so
 * models can call it again later. Example: `int fd = api_pty_fd_gen("usart2");`
 * @signature int api_pty_fd_gen(const char *dev_name);
 */
#define api_pty_fd_gen(...) \
    api_pty_fd_gen_impl(_PTY_FIRST_ARG(_, ##__VA_ARGS__, NULL))
#define _PTY_FIRST_ARG(_0, arg, ...) arg

/** Write one byte to the pseudo-terminal. Errors are logged. */
void api_pty_write_req(int fd, uint8_t value);

/**
 * Read one byte into `*buff` without blocking. Returns 1 if a byte was read,
 * 0 if none is available, -1 on error.
 */
int api_pty_read_nonblock(int fd, uint8_t *buff);
