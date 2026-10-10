/**
 * @file test_submission_queue.c
 * @brief Unit tests and invariant assertions for ring_io submission/completion queues
 * @author Maya Thorne <zse4123jo@gmail.com>
 * @license MIT
 */

#include "ring_io.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    printf("[test_submission_queue] Running assertions...\n");

    ring_io_ctx_t *ctx = ring_io_create(128, RING_IO_FLAG_NONE);
    assert(ctx != NULL);

    char buf[64] = "ring_io_test_payload";

    /* Test 1: Submit single event */
    int rc = ring_io_prep_read(ctx, 3, buf, sizeof(buf), 0, 0xCAFE);
    assert(rc == 0);

    rc = ring_io_submit(ctx);
    assert(rc == 1);

    ring_io_event_t ev;
    rc = ring_io_wait(ctx, 1, &ev, 1, 0);
    assert(rc == 1);
    assert(ev.user_data == 0xCAFE);
    assert(ev.res == (int32_t)sizeof(buf));

    /* Test 2: Batch submission */
    for (int i = 0; i < 32; i++) {
        rc = ring_io_prep_write(ctx, 4, buf, sizeof(buf), (uint64_t)i * 64, (uint64_t)i);
        assert(rc == 0);
    }

    rc = ring_io_submit(ctx);
    assert(rc == 32);

    ring_io_event_t events[32];
    rc = ring_io_wait(ctx, 32, events, 32, 0);
    assert(rc == 32);
    for (int i = 0; i < 32; i++) {
        assert(events[i].user_data == (uint64_t)i);
    }

    /* Test 3: Stats verification */
    ring_io_stats_t stats;
    ring_io_get_stats(ctx, &stats);
    assert(stats.submissions == 33);
    assert(stats.completions == 33);
    assert(stats.sq_drops == 0);

    ring_io_destroy(ctx);

    printf("[test_submission_queue] All 3 assertions passed successfully.\n");
    return 0;
}
