/* Built against an installed LibVIO by tests/install_consumer.sh. */
#include <vio/vio.h>

#include <stdio.h>

int main(void) {
    vio_set_log_level(VIO_LOG_WARNING);
    api_signal_set(1, true);
    printf("consumer ok (log level %d)\n", vio_get_log_level());
    return 0;
}
