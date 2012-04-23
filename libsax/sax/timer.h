#ifndef __TIMER_H_QS__
#define __TIMER_H_QS__

/// @file timer.h
/// @author Qingshan
/// @date 2010.3.1
/// @brief Single Timer Wheel Timer @n
/// The Single Timer Wheel Timer facility is optimized to support embedded
/// timer structures, i.e. where the timer structure is integrated into the
/// structure it is associated with.
///  -# Spoke - A queue of timers set to expire.
///  -# Granularity - milliseconds between the processing of each spoke.
///  -# Rotation - one complete turn around the wheel
///
/// @see http://drdobbs.com/embedded-systems/217800350

#include "os_types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct g_timer_t g_timer_t;

/// @brief creates and initializes a single timer wheel
/// @param capacity total number of timer that can be started
/// @param wheel_size number of spokes in the wheel (32~4096)
/// @param granularity milliseconds between ticks (1~1000)
/// @return the handle of a g_timer_t, NULL for false
g_timer_t *g_timer_create(
	uint32_t capacity, uint32_t wheel_size, uint32_t granularity);

/// @brief destroys a timer wheel
void g_timer_destroy(g_timer_t *stw);

/// @brief drives time for the specified wheels, thread-safe
void g_timer_loop(g_timer_t *stw);


typedef void (* g_timer_event)(
	uint32_t id, void *client, void *param);

/// @brief starts a new timer, thread-safe
uint32_t g_timer_start(g_timer_t *stw, uint32_t delay, 
	g_timer_event func, void *client, void *param);

/// @brief stops a currently running timer, thread-safe
int g_timer_cancel(g_timer_t *stw, uint32_t id);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif//__TIMER_H_QS__
