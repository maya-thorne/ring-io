/**
 * @file ring_io.h
 * @brief High-Throughput Asynchronous I/O Engine via Linux io_uring (C23)
 * @author Maya Thorne <zse4123jo@gmail.com>
 * @license MIT
 */

#ifndef RING_IO_H
#define RING_IO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RING_IO_DEFAULT_ENTRIES 2048
#define RING_IO_CACHE_LINE_SIZE 64

typedef enum {
    RING_IO_OP_READ = 0,
    RING_IO_OP_WRITE,
    RING_IO_OP_ACCEPT,
    RING_IO_OP_CONNECT,
    RING_IO_OP_POLL,
    RING_IO_OP_TIMEOUT,
    RING_IO_OP_NOP
} ring_io_opcode_t;

typedef enum {
    RING_IO_FLAG_NONE       = 0x00,
    RING_IO_FLAG_SQPOLL     = 0x01,  /* Kernel SQ polling thread enabled */
    RING_IO_FLAG_FIXED_FILE = 0x02,  /* Registered buffer / direct descriptor */
    RING_IO_FLAG_DRAIN      = 0x04   /* Complete previous before dispatching */
} ring_io_flags_t;

typedef struct ring_io_event {
    uint64_t user_data;
    int32_t  res;
    uint32_t flags;
} ring_io_event_t;

typedef struct ring_io_stats {
    uint64_t submissions;
    uint64_t completions;
    uint64_t sq_drops;
    uint64_t cq_overflows;
    uint64_t ring_reaped_batches;
} ring_io_stats_t;

/* Opaque context handle */
typedef struct ring_io_ctx ring_io_ctx_t;

/* Callback signature */
typedef void (*ring_io_callback_fn)(ring_io_ctx_t *ctx, ring_io_event_t *event, void *arg);

/**
 * @brief Initializes a ring_io engine context with the specified queue depth and flags.
 */
ring_io_ctx_t *ring_io_create(uint32_t entries, ring_io_flags_t flags);

/**
 * @brief Destroys the context and unmaps submission/completion rings.
 */
void ring_io_destroy(ring_io_ctx_t *ctx);

/**
 * @brief Submits a read request to the SQ ring.
 */
int ring_io_prep_read(ring_io_ctx_t *ctx, int fd, void *buf, size_t count, uint64_t offset, uint64_t user_data);

/**
 * @brief Submits a write request to the SQ ring.
 */
int ring_io_prep_write(ring_io_ctx_t *ctx, int fd, const void *buf, size_t count, uint64_t offset, uint64_t user_data);

/**
 * @brief Flushes pending submissions to the kernel io_uring.
 */
int ring_io_submit(ring_io_ctx_t *ctx);

/**
 * @brief Waits for at least min_complete events and drains CQ ring.
 */
int ring_io_wait(ring_io_ctx_t *ctx, uint32_t min_complete, ring_io_event_t *events, uint32_t max_events, uint32_t timeout_ms);

/**
 * @brief Non-blocking event poll and dispatch via callback.
 */
int ring_io_dispatch(ring_io_ctx_t *ctx, ring_io_callback_fn callback, void *arg);

/**
 * @brief Extracts real-time runtime statistics.
 */
void ring_io_get_stats(const ring_io_ctx_t *ctx, ring_io_stats_t *out_stats);

#ifdef __cplusplus
}
#endif

#endif /* RING_IO_H */
