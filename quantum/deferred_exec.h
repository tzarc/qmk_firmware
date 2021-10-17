// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>

// A token that can be used to cancel an existing deferred execution.
typedef uint8_t deferred_token;
#define INVALID_DEFERRED_TOKEN 0

// Callback to execute.
//  -- Parameter cb_arg: the callback argument specified when enqueueing the deferred executor
//  -- Return value: Non-zero re-queues the callback to execute after the returned number of milliseconds. Zero cancels repeated execution.
typedef uint32_t (*deferred_exec_callback)(void *cb_arg);

// Configures the supplied deferred executor to be executed after the required number of milliseconds.
//  -- Parameter delay_ms: the number of milliseconds before executing the callback
//  --           callback: the executor to invoke
//  --           cb_arg: the argument to pass to the executor, may be NULL if unused by the executor
//  -- Return value: a token
deferred_token enqueue_deferred_exec(uint32_t delay_ms, deferred_exec_callback callback, void *cb_arg);

// Allows for cancellation of an existing deferred execution.
//  -- Parameter token: the returned value from enqueue_deferred_exec for the deferred execution you wish to cancel.
void cancel_deferred_exec(deferred_token token);

// Forward declaration for the main loop in order to execute any deferred executors. Should not be invoked by keyboard/user code.
void deferred_exec_task(void);
