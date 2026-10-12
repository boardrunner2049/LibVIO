#pragma once
#include <stdint.h>

/**
 * DMA requests between peripheral models.
 *
 * A DMA model registers a handler per (controller, stream); a peripheral
 * model triggers it. Requests are delivered by calling the handler
 * immediately, in the caller's context. Controllers 0-3 and streams 0-15 are
 * supported; requests outside that range, or for a stream with no handler,
 * are ignored.
 */

/** Called for a request without payload. */
typedef void (*dma_request_handler_t)(void *opaque);

/** Called for a request carrying `len` payload bytes. */
typedef void (*dma_request_data_handler_t)(void *opaque,
                                           const uint8_t *data,
                                           int len);

/**
 * Register the handler that receives payload-free requests for
 * (controller_id, stream_id), e.g. (1, 4) for DMA1 stream 4. `opaque` is
 * passed back to the handler. A later call replaces the handler.
 */
void api_dma_register_stream(int controller_id, int stream_id,
                             dma_request_handler_t handler,
                             void *opaque);

/** Trigger a payload-free request on (controller_id, stream_id). */
void api_dma_request(int controller_id, int stream_id);

/**
 * Register the handler that receives payload-carrying requests for
 * (controller_id, stream_id). `opaque` is passed back to the handler.
 */
void api_dma_register_stream_data(int controller_id, int stream_id,
                                  dma_request_data_handler_t handler,
                                  void *opaque);

/**
 * Trigger a request carrying `len` bytes (at most 8) of `data` on
 * (controller_id, stream_id). `target_addr` is the memory-mapped register
 * the data belongs to (e.g. the SPI data register); it is only used for the
 * transfer log, which is appended to `memory.log` in the current directory.
 * Returns 0 on success, -1 if the stream has no payload handler or the
 * arguments are out of range.
 */
int api_dma_request_data(int controller_id, int stream_id, uint32_t target_addr, const uint8_t *data, int len);
