#ifndef __TIMER_H_QS__
#define __TIMER_H_QS__

/**
 * @file timer.h
 * @brief Hashed and Hierarchical Timing Wheels, should use this timer in a single thread
 * @note paper: "Hashed and Hierarchical Timing Wheels:
 * 				Efficient DataStructures for Implementing a Timer Facility"
 *
 * @author X
 * @date 2012-7-27
 */

#include "os_types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct g_timer_t g_timer_t;
typedef void* g_timer_handle_t;

typedef void (* g_timer_proc)(
		g_timer_handle_t handle, void* user_data);

typedef void (* g_timer_free)(void* user_data);

// equal to g_timer_create2(0)
g_timer_t* g_timer_create();

// for testing
g_timer_t* g_timer_create2(uint32_t now);

// the free_func is used for releasing user_data of timers, can be NULL
void g_timer_destroy(g_timer_t* t, g_timer_free free_func);

// NOTICE: the returning handle become invalid after the timer fired
g_timer_handle_t g_timer_start(g_timer_t* t, uint32_t delay_ticks,
		g_timer_proc func, void* user_data);

int32_t g_timer_cancel(g_timer_t* t, g_timer_handle_t handle, void** user_data);

void g_timer_poll(g_timer_t* t, uint32_t lapsed_ticks);

uint32_t g_timer_count(g_timer_t* t);

void g_timer_shrink_mempool(g_timer_t* t, double keep);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif//__TIMER_H_QS__
