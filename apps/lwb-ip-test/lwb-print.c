#include "contiki.h"

#include "glossy.h"

#include "lwb-print.h"
#include "lwb.h"
#include "lwb-macros.h"

extern lwb_context_t lwb_context;

void lwb_print_stats() {

    printf("L|%u %u %u %u %u %u |%lu\n",
                    lwb_context.sync_state,
                    CURRENT_SCHEDULE_INFO().time,
                    CURRENT_SCHEDULE_INFO().round_period,
                    GET_N_DATA_SLOTS(CURRENT_SCHEDULE_INFO().n_slots),
                    GET_N_FREE_SLOTS(CURRENT_SCHEDULE_INFO().n_slots),
                    lwb_context.n_my_slots,
                    lwb_get_host_time());

//    if (lwb_context.lwb_mode == LWB_MODE_SOURCE) {
//        uint8_t i = 0;
//        if (lwb_context.sync_state == LWB_SYNC_STATE_SYNCED ||
//            lwb_context.sync_state == LWB_SYNC_STATE_UNSYNCED_1) {
//            printf("S|");
//            for (i = 0; i < GET_N_DATA_SLOTS(CURRENT_SCHEDULE_INFO().n_slots); i++) {
//                printf("%X ", CURRENT_SCHEDULE().slots[i]);
//            }
//            printf("\n");
//        }
//    }

    if (lwb_context.lwb_mode == LWB_MODE_HOST) {
        printf("S|%u %u |%lu\n", LWB_STATS_SCHED(ui16_n_unused_slots), LWB_STATS_SYNC(ui16_sync_missed), lwb_get_host_time());
    } else {
        //printf("L|%lu\n", lwb_context.ui32_time);
    }

}
