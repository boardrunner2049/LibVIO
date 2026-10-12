/* Helpers shared by LibVIO's sources; not installed. */
#pragma once

#include <vio/log.h>

void vio_log(int level, const char *subsystem, const char *format, ...)
    __attribute__((format(printf, 3, 4)));

#define VIO_DEBUG(subsystem, ...) vio_log(VIO_LOG_DEBUG, subsystem, __VA_ARGS__)
#define VIO_INFO(subsystem, ...) vio_log(VIO_LOG_INFO, subsystem, __VA_ARGS__)
#define VIO_WARN(subsystem, ...) vio_log(VIO_LOG_WARNING, subsystem, __VA_ARGS__)
#define VIO_ERROR(subsystem, ...) vio_log(VIO_LOG_ERROR, subsystem, __VA_ARGS__)
