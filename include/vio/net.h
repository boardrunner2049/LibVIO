#pragma once
#include <stdint.h>

/**
 * Ethernet frames through a Linux TAP interface, so an emulated network
 * peripheral can talk to the host's network stack. Creating a TAP interface
 * needs permission for /dev/net/tun (usually CAP_NET_ADMIN).
 */

/**
 * Open the TAP interface `dev_name` (e.g. "tap0"; "" lets the kernel choose
 * a name) in non-blocking mode. `dev_name` must not be NULL. Returns the
 * file descriptor, or -1 on error.
 */
int api_tap_init(const char *dev_name);

/**
 * Send one raw Ethernet frame of `len` bytes to the host. Returns the number
 * of bytes written, or -1 on error.
 */
int api_tap_send(int fd, const uint8_t *buf, int len);

/**
 * Receive one frame from the host into `buf` (at most `max_len` bytes)
 * without blocking. Returns its length, or -1 if no frame is waiting or on
 * error.
 */
int api_tap_recv_nonblock(int fd, uint8_t *buf, int max_len);
