#include "check.h"

#include <vio/vio.h>

#include <string.h>

static int calls;
static int last_level;
static char last_subsystem[32];
static char last_message[256];

static void capture(int level, const char *subsystem, const char *message, void *user) {
    CHECK(user == &calls);
    calls++;
    last_level = level;
    snprintf(last_subsystem, sizeof(last_subsystem), "%s", subsystem);
    snprintf(last_message, sizeof(last_message), "%s", message);
}

int main(void) {
    CHECK(vio_get_log_level() == VIO_LOG_INFO);

    vio_set_logger(capture, &calls);
    CHECK(api_file_open("/nonexistent-dir/vio-test", 0) < 0);
    CHECK(calls == 1);
    CHECK(last_level == VIO_LOG_ERROR);
    CHECK(strcmp(last_subsystem, "fileio") == 0);
    CHECK(strncmp(last_message, "open failed: ", 13) == 0);
    CHECK(strchr(last_message, '\n') == NULL);

    vio_set_log_level(VIO_LOG_ERROR + 1);
    CHECK(api_file_open("/nonexistent-dir/vio-test", 0) < 0);
    CHECK(calls == 1);

    vio_set_log_level(VIO_LOG_INFO);
    vio_set_logger(NULL, NULL);  /* back to stderr */
    CHECK(api_file_open("/nonexistent-dir/vio-test", 0) < 0);
    CHECK(calls == 1);

    printf("test_log passed\n");
    return 0;
}
