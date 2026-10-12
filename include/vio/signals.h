#ifndef VIO_SIGNALS_H
#define VIO_SIGNALS_H

#include <stdbool.h>

/**
 * Logical signal lines between peripheral models, e.g. a GPIO pin driving
 * an EXTI line. For STM32 GPIO/EXTI, use the EXTI line number as the signal
 * ID. Signals are level-based: the source publishes the line level, and the
 * sink (e.g. EXTI) owns edge detection and trigger policy.
 */

/** Number of signal lines; valid IDs are 0 to MAX_SIGNALS - 1. */
#define MAX_SIGNALS 128

/** Called with the new `level` (true = high) of line `signal_id`. */
typedef void (*signal_handler_t)(void *opaque, int signal_id, bool level);

/**
 * Register the handler for line `signal_id`; `opaque` is passed back to it.
 * Each line has one handler: a later call replaces it.
 */
void api_signal_register(int signal_id, signal_handler_t handler, void *opaque);

/**
 * Set the level of line `signal_id`. The handler is called immediately if
 * the level changed; setting the same level again does nothing.
 */
void api_signal_set(int signal_id, bool level);

#endif // VIO_SIGNALS_H
