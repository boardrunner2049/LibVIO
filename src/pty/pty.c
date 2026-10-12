#include <vio/pty.h>
#include "../internal.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>


//TODO: Currently, the path is hardcoded- update the apis

//Returns the pseudo terminal handler
#undef api_pty_fd_gen
int api_pty_fd_gen_impl(const char* dev_name) {
    char pty_path[256];
    if (dev_name && dev_name[0] != '\0') {
        snprintf(pty_path, sizeof(pty_path), "/tmp/%s_pty", dev_name);
    } else {
        snprintf(pty_path, sizeof(pty_path), "/tmp/usart1_pty");
    }

    int fd = open(pty_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        // Suppress noise when polling for an endpoint that is not ready yet.
    }
    return fd;
}

// Sends a single byte to the pseudo-terminal fd (blocking write)
void api_pty_write_req(int fd, uint8_t value) {
        if (write(fd, &value, 1) < 0) {
            VIO_ERROR("pty", "write() to PTY failed: %s", strerror(errno));
        }
}

/**
 * @brief Attempts to read a single byte from the PTY file descriptor
 * in a non-blocking manner.
 * @param fd The file descriptor for the PTY.
 * @param buff A pointer to a uint8_t where the read byte will be stored.
 * @return 1 on success (a byte was read and stored in out_byte).
 * 0 if no data was available to read.
 * -1 on a critical error.
 */
int api_pty_read_nonblock(int fd, uint8_t *buff) {
    ssize_t n = read(fd, buff, 1);

    if (n == 1) {
        // Success: We read exactly one byte.
        return 1;
    } else if (n == -1) {
        // An error occurred. Check if it was because the buffer was empty.
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        } else {
            VIO_ERROR("pty", "critical PTY read error: %s", strerror(errno));
            return -1;
        }
    }
    return -1;
}
