#include "check.h"

#include <vio/fileio.h>

#include <string.h>

int main(void) {
    char path[] = "/tmp/vio_test_fileio_XXXXXX";
    int tmp = mkstemp(path);
    CHECK(tmp >= 0);
    close(tmp);

    int fd = api_file_open(path, 1);
    CHECK(fd >= 0);
    CHECK(api_file_pwrite(fd, "abcdef", 6, 2) == 6);
    CHECK(api_file_get_size(fd) == 8);

    char buf[8];
    CHECK(api_file_pread(fd, buf, 4, 2) == 4);
    CHECK(memcmp(buf, "abcd", 4) == 0);

    /* Reading past the end fills the rest with zeros (or the fill byte). */
    memset(buf, 0x55, sizeof(buf));
    CHECK(api_file_pread(fd, buf, 4, 6) == 4);
    CHECK(memcmp(buf, "ef\0\0", 4) == 0);
    CHECK(api_file_pread_fill(fd, buf, 4, 6, 0xFF) == 4);
    CHECK(memcmp(buf, "ef\xff\xff", 4) == 0);

    api_file_close(fd);
    unlink(path);
    printf("test_fileio passed\n");
    return 0;
}
