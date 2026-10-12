#include "internal.h"

#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>

static vio_log_fn log_fn;
static void *log_user;
static atomic_int log_level = VIO_LOG_INFO;

void vio_set_logger(vio_log_fn fn, void *user) {
    log_fn = fn;
    log_user = user;
}

void vio_set_log_level(int level) {
    atomic_store(&log_level, level);
}

int vio_get_log_level(void) {
    return atomic_load(&log_level);
}

static const char *level_name(int level) {
    if (level >= VIO_LOG_ERROR) {
        return "ERROR";
    }
    if (level >= VIO_LOG_WARNING) {
        return "WARNING";
    }
    if (level >= VIO_LOG_INFO) {
        return "INFO";
    }
    return "DEBUG";
}

void vio_log(int level, const char *subsystem, const char *format, ...) {
    if (level < atomic_load(&log_level)) {
        return;
    }
    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    if (log_fn) {
        log_fn(level, subsystem, message, log_user);
        return;
    }
    fprintf(stderr, "vio.%s|%s|  %s\n", subsystem, level_name(level), message);
}
