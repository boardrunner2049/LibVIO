#pragma once

/**
 * LibVIO's diagnostics.
 *
 * By default LibVIO writes its messages to stderr as
 * `vio.<subsystem>|LEVEL|  message`. A host (for example the emulator that
 * loads the model) can send them to its own log instead. Levels use the
 * same numbers as Python's logging module.
 */

/** Message levels. */
#define VIO_LOG_DEBUG 10
#define VIO_LOG_INFO 20
#define VIO_LOG_WARNING 30
#define VIO_LOG_ERROR 40

/**
 * Called with one message; `message` is formatted and has no trailing
 * newline.
 */
typedef void (*vio_log_fn)(int level, const char *subsystem, const char *message, void *user);

/**
 * Send messages to `fn`, with `user` passed back (NULL restores the stderr
 * default). Call it before using the library; it is not synchronised with
 * concurrent log calls.
 */
void vio_set_logger(vio_log_fn fn, void *user);

/** Drop messages below `level` (default: VIO_LOG_INFO). */
void vio_set_log_level(int level);

/** Return the current level. */
int vio_get_log_level(void);
