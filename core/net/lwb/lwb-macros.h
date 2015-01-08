#ifndef __LWB_MACROS_H___
#define __LWB_MACROS_H___

/// @file lwb-macros.h
/// @brief LWB context specific macros

#include "contiki.h"

#include "lwb-common.h"

#define N_CURRENT_DATA_SLOTS()          GET_N_DATA_SLOTS(lwb_context.current_sched.sched_info.n_slots)
#define N_CURRENT_FREE_SLOTS()          GET_N_FREE_SLOTS(lwb_context.current_sched.sched_info.n_slots)
#define CURRENT_SCHEDULE()              (lwb_context.current_sched)
#define CURRENT_SCHEDULE_INFO()         (lwb_context.current_sched.sched_info)

#define OLD_SCHEDULE()                  (lwb_context.old_sched)
#define OLD_SCHEDULE_INFO()             (lwb_context.old_sched.sched_info)

#define LWB_STATS_SYNC(statitem)        (lwb_context.sync_stats.statitem)
#define LWB_STATS_DATA(statitem)        (lwb_context.data_stats.statitem)
#define LWB_STATS_STREAM_REQ_ACK(statitem)  (lwb_context.stream_req_ack_stats.statitem)
#define LWB_STATS_SCHED(statitem)       (lwb_context.sched_stats.statitem)

#define LWB_SET_POLL_FLAG(flag)         (lwb_context.poll_flags |= 1 << flag)
#define LWB_UNSET_POLL_FLAG(flag)       (lwb_context.poll_flags &= ~(1 << flag))
#define LWB_IS_SET_POLL_FLAG(flag)      (lwb_context.poll_flags && (1 << flag))

/// @addtogroup rtimer scheduling
///             macro for rtimer based scheduling
/// @{
#define SCHEDULE(ref, offset, cb)   rtimer_set(&lwb_context.rt, ref + offset, 1, (rtimer_callback_t)cb, &lwb_context)
#define SCHEDULE_L(ref, offset, cb) rtimer_set_long(&lwb_context.rt, ref, offset, (rtimer_callback_t)cb, &lwb_context)
///  @}

#endif // __LWB_MACROS_H___
