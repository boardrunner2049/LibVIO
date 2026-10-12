#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

/**
 * Named pipes (FIFOs) for byte streams to and from host programs, e.g. a
 * button press or sensor values written by a script.
 */

/**
 * Create the named pipe at `path` if needed (e.g. "/tmp/gpio_btn") and open
 * it for reading and writing without blocking. Because LibVIO keeps it open
 * for writing too, reads do not see end-of-file when an outside writer
 * disconnects. Returns the file descriptor, or -1 on error.
 */
int api_fifo_open(const char *path);

/**
 * Write `len` bytes of `data` to the pipe. Returns the number of bytes
 * written, or -1 (e.g. when the pipe is full).
 */
int api_fifo_write(int fd, const void *data, int len);

/**
 * Read one byte into `*out_byte` without blocking. Returns 1 if a byte was
 * read, 0 if the pipe is empty, -1 on error.
 */
int api_fifo_read_nonblock(int fd, uint8_t *out_byte);

/** Close `fd` and delete the pipe at `path` (if path is not NULL). */
void api_fifo_close(int fd, const char *path);
