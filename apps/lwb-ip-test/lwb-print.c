/*
 * Copyright (c) 2014, Uppsala University, Sweden.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author: Kasun Hewage <kasun.hewage@it.uu.se>
 *
 */

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


//    if (lwb_context.lwb_mode == LWB_MODE_HOST) {
//        uint8_t i = 0;
//        printf("SL|");
//        for (i = 0; i < GET_N_DATA_SLOTS(CURRENT_SCHEDULE_INFO().n_slots); i++) {
//            printf("%X ", CURRENT_SCHEDULE().slots[i]);
//        }
//        printf("\n");
//
//        lwb_sched_print();
//    }

    if (lwb_context.lwb_mode == LWB_MODE_HOST) {
        printf("S|%u %u |%lu\n", LWB_STATS_SCHED(n_unused_slots), LWB_STATS_SYNC(n_sync_missed), lwb_get_host_time());
    } else {
        //printf("L|%lu\n", lwb_context.ui32_time);
    }

}
