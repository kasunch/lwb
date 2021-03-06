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

#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "lwb.h"

#include "lwb-print.h"


PROCESS(lwb_test_process, "lwb test");
AUTOSTART_PROCESSES(&lwb_test_process);


#define LWB_HOST_ID             1
#define LWB_DATA_ECHO_CLIENT    12
#define LWB_DATA_ECHO_SERVER    11


extern uint16_t node_id;

static struct etimer et;
static uint8_t buffer[100];
static uint8_t ui8_buf_len = 0;
static uint16_t ui16_c = 0;

inline void send_stream_mod() {

    if (((lwb_get_joining_state() == LWB_JOINING_STATE_NOT_JOINED) ||
                    (lwb_get_joining_state() == LWB_JOINING_STATE_JOINED)) &&
                    (lwb_get_n_my_slots() == 0)) {
        lwb_request_stream_mod(1, 1);
    } else {
        // It is possible that we are joining or partly joined.
        // This means that we are waiting for an acknowledgment. We do not send more stream add(mod)
        // requests until we receive an acknowledgment.

        // It is also possible that the host has assigned some slots for us.
        // We just use them
    }
}

//--------------------------------------------------------------------------------------------------
void on_data(uint8_t *p_data, uint8_t ui8_len, uint16_t ui16_from_id) {

    uint16_t ui16_val;
    if (node_id == LWB_DATA_ECHO_SERVER) {

        ui16_val = (uint16_t)p_data[0] | ((uint16_t)(p_data[1]) << 8);
        printf("server: on_data from %u len %u, counter %u\n", ui16_from_id, ui8_len, ui16_val);
        lwb_queue_packet(p_data, ui8_len, LWB_DATA_ECHO_CLIENT);
        send_stream_mod();

    } else if(node_id == LWB_DATA_ECHO_CLIENT) {

        ui16_val = (uint16_t)p_data[0] | ((uint16_t)(p_data[1]) << 8);
        printf("client: on_data from %u len %u, counter %u\n", ui16_from_id, ui8_len, ui16_val);
    }
}

//--------------------------------------------------------------------------------------------------
void on_schedule_end(void) {
    lwb_print_stats();
}

lwb_callbacks_t lwb_callbacks = {on_data, on_schedule_end};

//--------------------------------------------------------------------------------------------------
PROCESS_THREAD(lwb_test_process, ev, data)
{
    PROCESS_BEGIN();

    if(node_id == LWB_HOST_ID) {

        lwb_init(LWB_MODE_HOST, &lwb_callbacks);

    } else {

        lwb_init(LWB_MODE_SOURCE, &lwb_callbacks);

        if (node_id == LWB_DATA_ECHO_CLIENT) {

            send_stream_mod();
            etimer_set(&et, CLOCK_SECOND);

            while (1) {
                PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
                buffer[0] = ui16_c & 0xFF;
                buffer[1] = (ui16_c >> 8) & 0xFF;
                memset(buffer + 2, 1, 80);
                ui8_buf_len = 82;
                ui16_c++;
                lwb_queue_packet(buffer, ui8_buf_len, LWB_DATA_ECHO_SERVER);
                etimer_set(&et, CLOCK_SECOND);
            }

        } else if (node_id == LWB_DATA_ECHO_SERVER) {

        }
    }


    PROCESS_END();

    return PT_ENDED;
}

