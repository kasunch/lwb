#include "contiki.h"

#include "glossy.h"

#include "lwb-print.h"
#include "lwb-common.h"
#include "lwb-macros.h"

extern lwb_context_t lwb_context;

void lwb_print_stats() {

    if (lwb_context.lwb_mode == LWB_MODE_SOURCE) {
        printf("state %u, ref %u, rtimer next %u, oflows %u, skew %ld, guard %u\n",
               lwb_context.sync_state,
               lwb_context.t_last_sync_ref,
               RTIMER_TIME(&lwb_context.rt),
               lwb_context.rt.overflows_to_go,
               (lwb_context.skew/(int32_t)64),
               lwb_context.t_sync_guard);

    }

    printf("time %u, n_slots [data %u, free %u, my %u], T %u\n",
           CURRENT_SCHEDULE_INFO().time,
           GET_N_DATA_SLOTS(CURRENT_SCHEDULE_INFO().n_slots),
           GET_N_FREE_SLOTS(CURRENT_SCHEDULE_INFO().n_slots),
           lwb_context.n_my_slots,
           CURRENT_SCHEDULE_INFO().round_period);

    uint8_t i = 0;

    if (lwb_context.sync_state == LWB_SYNC_STATE_SYNCED ||
        lwb_context.sync_state == LWB_SYNC_STATE_UNSYNCED_1) {
        printf("sched : ");
        for (i = 0; i < GET_N_DATA_SLOTS(CURRENT_SCHEDULE_INFO().n_slots); i++) {
            printf("%04X ", CURRENT_SCHEDULE().slots[i]);
        }
        printf("\n");
    }

    if (lwb_context.lwb_mode == LWB_MODE_HOST) {
        printf("stream stats [added %u, dups : %u, no_space %u]\n",
                LWB_STATS_SCHED(ui16_n_added),
                LWB_STATS_SCHED(ui16_n_duplicates),
                LWB_STATS_SCHED(ui16_n_no_space));
    }

}