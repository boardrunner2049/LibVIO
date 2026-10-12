#include "check.h"

#include <vio/fifo.h>

int main(void) {
    char path[64];
    snprintf(path, sizeof(path), "/tmp/vio_test_fifo_%d", (int)getpid());

    int fd = api_fifo_open(path);
    CHECK(fd >= 0);

    uint8_t byte = 0;
    CHECK(api_fifo_read_nonblock(fd, &byte) == 0);  /* empty */
    CHECK(api_fifo_write(fd, "hi", 2) == 2);
    CHECK(api_fifo_read_nonblock(fd, &byte) == 1 && byte == 'h');
    CHECK(api_fifo_read_nonblock(fd, &byte) == 1 && byte == 'i');
    CHECK(api_fifo_read_nonblock(fd, &byte) == 0);

    api_fifo_close(fd, path);
    CHECK(access(path, F_OK) != 0);
    printf("test_fifo passed\n");
    return 0;
}
