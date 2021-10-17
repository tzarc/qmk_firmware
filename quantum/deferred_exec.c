// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <deferred_exec.h>

#ifndef MAX_DEFERRED_EXECUTORS
#    define MAX_DEFERRED_EXECUTORS 8
#endif

typedef struct deferred_executor_t {
    uint32_t               trigger_time;
    deferred_exec_callback callback;
    void *                 cb_arg;
} deferred_executor_t;

static deferred_executor_t executors[MAX_DEFERRED_EXECUTORS] = {0};

deferred_token enqueue_deferred_exec(uint32_t delay_ms, deferred_exec_callback callback, void *cb_arg) {
    // Ignore queueing if it's a zero-time delay
    if (delay_ms == 0 || !callback) {
        return INVALID_DEFERRED_TOKEN;
    }

    // Find an unused slot and claim it
    for (int i = 0; i < MAX_DEFERRED_EXECUTORS; ++i) {
        if (executors[i].trigger_time == 0) {
            executors[i].trigger_time = timer_read32() + delay_ms;
            executors[i].callback     = callback;
            executors[i].cb_arg       = cb_arg;
            return (deferred_token)(i + 1);
        }
    }

    // None available
    return INVALID_DEFERRED_TOKEN;
}

void cancel_deferred_exec(deferred_token token) {
    int idx = ((int)token) - 1;
    if (idx >= 0 && idx < MAX_DEFERRED_EXECUTORS) {
        executors[idx].trigger_time = 0;
        executors[idx].callback     = NULL;
        executors[idx].cb_arg       = NULL;
    }
}

void deferred_exec_task(void) {
    static uint32_t last_check = 0;
    uint32_t        now        = timer_read32();

    // Throttle only once per millisecond
    if (TIMER_DIFF_32(now, last_check) >= 1) {
        last_check = now;

        // Run through each of the executors
        for (int i = 0; i < MAX_DEFERRED_EXECUTORS; ++i) {
            deferred_executor_t *entry = &executors[i];

            // Check if we're supposed to execute this entry
            if (entry->trigger_time > 0 && TIMER_DIFF_32(now, entry->trigger_time) <= 0) {
                // Invoke the callback and work work out if we should be requeued
                uint32_t delay_ms = entry->callback(entry->cb_arg);

                // Update the trigger time if we have to repeat, otherwise clear it out
                if (delay_ms > 0) {
                    // Intentionally add just the delay to the existing trigger time -- this ensures the next
                    // invocation is with respect to the previous trigger, rather than when it got to execution. Under
                    // normal circumstances this won't cause issue, but if another executor is invoked that takes a
                    // considerable length of time, then this
                    entry->trigger_time += delay_ms;
                } else {
                    // If it was zero, then the callback is cancelling repeated execution. Free up the slot.
                    entry->trigger_time = 0;
                    entry->callback     = NULL;
                    entry->cb_arg       = NULL;
                }
            }
        }
    }
}
