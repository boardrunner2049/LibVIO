#pragma once
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define _FILE_OFFSET_BITS 64

/**
 * Host files as persistent storage, e.g. the contents of an emulated flash
 * or EEPROM chip.
 */

/**
 * Open `filename`: mode 0 opens it read-only, mode 1 for reading and writing
 * (created if missing). Returns the file descriptor, or -1 on error.
 */
int api_file_open(const char *filename, int mode);

/**
 * Read `len` bytes at `offset` into `buf`. Bytes past the end of the file
 * read as 0x00. Returns `len`, or -1 on error (the rest of `buf` is then
 * zero-filled).
 */
int api_file_pread(int fd, void *buf, int len, uint64_t offset);

/**
 * Like `api_file_pread()`, but bytes past the end of the file read as
 * `fill_val` (e.g. 0xFF for erased flash).
 */
int api_file_pread_fill(int fd, void *buf, int len, uint64_t offset, uint8_t fill_val);

/** Write `len` bytes from `buf` at `offset`. Returns `len`, or -1 on error. */
int api_file_pwrite(int fd, const void *buf, int len, uint64_t offset);

/** Return the file's size in bytes (0 on error). */
uint64_t api_file_get_size(int fd);

/** Close `fd`. */
void api_file_close(int fd);
