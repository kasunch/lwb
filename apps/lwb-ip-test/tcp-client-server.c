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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "contiki.h"
#include "uip6/uip.h"
#include "uip6/uip-ds6.h"
#include "uip6/psock.h"

#if CONTIKI_TARGET_SKY
#include "dev/ds2411.h"
#endif // CONTIKI_TARGET_SKY

#include "deployment.h" 

#define DEBUG DEBUG_PRINT
#include "uip6/uip-debug.h"

#include "sicslowpan.h"

#include "lwb.h"
#include "lwb-print.h"

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define TCP_SERVER_NODE_ID  13
#define TCP_CLIENT_NODE_ID  2
#define LWB_HOST_NODE_ID    1
#define TCP_PORT            20222

// These delays are from the boot of the node
#define TCP_CLIENT_CONNECT_DELAY    60
#define SOURCE_NODE_START_DELAY     20

#define TCP_PAYLOAD_LEN_FRAG_1      70
#define TCP_PAYLOAD_LEN_FRAG_2      170
#define TCP_PAYLOAD_LEN_FRAG_3      270
#define TCP_PAYLOAD_LEN_FRAG_4      370

// Available for application data
// 127 - (1 (relay counter) + 1 (Glossy header) + 8 (LWB data header) + 20 (compressed IP header) + 20 (TCP))
// If the total payload is greater than (127 - (1 (relay counter) + 1 (Glossy header) + 8 (LWB data header)),
// 6LoWPAN fragmentation headers (4 (FAG_1) + 5 (FAG_N)) should also be considered.
// NOTE the size of 6LoWPAN fragments should be multiples of 8
#define TCP_PAYLOAD_LEN             TCP_PAYLOAD_LEN_FRAG_4

enum {
  ECHO_MSG_TYPE_REQ = 0xBB,
  ECHO_MSG_TYPE_RES = 0xDD
};

typedef struct __attribute__ ((__packed__)) echo_msg {
  uint8_t ui8_type;
  uint8_t ui8_counter_l;
  uint8_t ui8_counter_h;
  uint8_t ui8_pad;
  uint8_t dummy[TCP_PAYLOAD_LEN - 4];
} echo_msg_t;

typedef struct tcpstats {
  uint16_t seq;
  uint32_t lat;
} tcpstats_t;

extern lwb_context_t lwb_context;

#define TCPSTATS_ELEMENTS 40

tcpstats_t tcpstats_info[TCPSTATS_ELEMENTS];
uint16_t tcpstats_cnt;

static uip_ipaddr_t global_ip_addr;
static uint16_t ui16_counter = 0;
static echo_msg_t echo_msg_buf;
struct psock p_socket;
uint8_t connected = 0;

uint32_t en_on_start = 0;
uint32_t en_tot_start = 0;
uint32_t t_lat_start = 0;
uint32_t t_lat_end = 0;
uint32_t t_con_start = 0;
uint32_t t_con_now = 0;
uint32_t n_packets = 0;

PROCESS(tcp_server_process, "TCP process");
AUTOSTART_PROCESSES(&tcp_server_process);

void send_stream_req() {
    if (((lwb_get_joining_state() == LWB_JOINING_STATE_NOT_JOINED) ||
         (lwb_get_joining_state() == LWB_JOINING_STATE_JOINED)) && 
        (lwb_get_n_my_slots() == 0)) {
        lwb_request_stream_mod(1, 1);
    }
}

uint8_t lwb_tcpip_input(uint8_t *data, uint8_t len) {
    send_stream_req();
    lwb_queue_packet(data, len, 0);
    return 1; 
}

void on_data(uint8_t *p_data, uint8_t ui8_len, uint16_t ui16_from_id) {
    sicslowpan_input(p_data, ui8_len, ui16_from_id);
}


void on_schedule_end(void) {

    uint8_t i = 0;

    //t_con_now = clock_time();
    t_con_now = lwb_get_host_time();
    n_packets += tcpstats_cnt;

    if (node_id == TCP_CLIENT_NODE_ID && connected) {
        printf("P|");
        for(; i < tcpstats_cnt; i++) {
            printf("%u-%lu ", tcpstats_info[i].seq, (tcpstats_info[i].lat * 1000 / RTIMER_SECOND));
        }
        printf("|%lu %lu %lu", n_packets, (t_con_now - t_con_start), lwb_get_host_time());
        printf("\n");
    }

    tcpstats_cnt = 0;

    if((node_id == TCP_SERVER_NODE_ID || node_id == TCP_CLIENT_NODE_ID) && connected) {
        printf("T|%u |%lu %lu\n", UIP_STAT(uip_stat.tcp.rexmit), (t_con_now - t_con_start), lwb_get_host_time());
    }

    lwb_print_stats();
}

static PT_THREAD(server_handler())
{
    PSOCK_BEGIN(&p_socket);

    //t_con_start = clock_time(); // for total time
    t_con_start = lwb_get_host_time();

    PSOCK_WAIT_UNTIL(&p_socket, PSOCK_NEWDATA(&p_socket));
    PSOCK_READBUF_LEN(&p_socket, sizeof(echo_msg_t));

    if (echo_msg_buf.ui8_type == ECHO_MSG_TYPE_REQ) {

        ui16_counter = 0;

        while(1) {
            echo_msg_buf.ui8_type = ECHO_MSG_TYPE_RES;
            echo_msg_buf.ui8_counter_l = (uint8_t)(ui16_counter & 0xFF);
            echo_msg_buf.ui8_counter_h = (uint8_t)(ui16_counter >> 8);
            PSOCK_SEND(&p_socket, (uint8_t *) &echo_msg_buf, sizeof(echo_msg_t));
            ui16_counter++;
        }
    }

    PSOCK_END(&p_socket);

    return PT_ENDED;
}

static PT_THREAD(client_handler())
{
    PSOCK_BEGIN(&p_socket);

    echo_msg_buf.ui8_type = ECHO_MSG_TYPE_REQ;

    //t_con_start = clock_time(); // for total time
    t_con_start = lwb_get_host_time();
    // for duty cycle
    en_on_start = energest_type_time(ENERGEST_TYPE_LISTEN) + energest_type_time(ENERGEST_TYPE_TRANSMIT);
    en_tot_start = energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM);

    PSOCK_SEND(&p_socket, (uint8_t *) &echo_msg_buf, sizeof(echo_msg_t));

    while (1) {

        t_lat_start = energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM);

        PSOCK_WAIT_UNTIL(&p_socket, PSOCK_NEWDATA(&p_socket));
        PSOCK_READBUF_LEN(&p_socket, sizeof(echo_msg_t));
     
        t_lat_end = energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM);

        if (echo_msg_buf.ui8_type == ECHO_MSG_TYPE_RES) {
            uint16_t c = (uint16_t)echo_msg_buf.ui8_counter_l + ((uint16_t)(echo_msg_buf.ui8_counter_h) << 8);

            if (tcpstats_cnt < TCPSTATS_ELEMENTS) {
                tcpstats_info[tcpstats_cnt].seq = c;
                tcpstats_info[tcpstats_cnt].lat = t_lat_end - t_lat_start;
                tcpstats_cnt++;
            }
        }
    }

    PSOCK_END(&p_socket);

    return PT_ENDED;
}

#if 0
static void
print_local_addresses(void)
{
    int i;
    uint8_t state;

    PRINTF("Node %d, Server IPv6 addresses: ", node_id);
    for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
        state = uip_ds6_if.addr_list[i].state;
        if(state == ADDR_TENTATIVE || state == ADDR_PREFERRED) {
            PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
            PRINTF("\n");
            /* hack to make address "final" */
            if (state == ADDR_TENTATIVE) {
                uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
            }
        }
    }
}
#endif

lwb_callbacks_t lwb_callbacks = {on_data, on_schedule_end};                                                                                                                       
PROCESS_THREAD(tcp_server_process, ev, data)
{

    static struct etimer et;
    
    PROCESS_BEGIN();

#if CONTIKI_TARGET_SKY
    cc2420_set_tx_power(RF_POWER);
    printf("channel %d, tx power %d\n", RF_CHANNEL, RF_POWER);
#endif // CONTIKI_TARGET_SKY

#if UIP_CONF_IPV6 && CONTIKI_TARGET_SKY
    memcpy(&uip_lladdr.addr, ds2411_id, sizeof(uip_lladdr.addr));
#endif // UIP_CONF_IPV6


    sicslowpan_init();
    sicslowpan_set_outputfunc(lwb_tcpip_input);

    process_start(&tcpip_process, NULL);

    set_ipaddr_from_id(&global_ip_addr, node_id);   
    uip_ds6_addr_add(&global_ip_addr, 0, ADDR_MANUAL);

    memset(tcpstats_info, 0, sizeof(tcpstats_t) * TCPSTATS_ELEMENTS);
    tcpstats_cnt = 0;

    if (node_id == LWB_HOST_NODE_ID) {

        lwb_init(LWB_MODE_HOST, &lwb_callbacks);
    
    } else {
        
        etimer_set(&et, CLOCK_SECOND * SOURCE_NODE_START_DELAY);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

        lwb_init(LWB_MODE_SOURCE, &lwb_callbacks);

        if (node_id == TCP_SERVER_NODE_ID || node_id == TCP_CLIENT_NODE_ID) {
            
            if (node_id == TCP_SERVER_NODE_ID) {
                    
                tcp_listen(UIP_HTONS(TCP_PORT));
                
            } else {
                
                etimer_set(&et, CLOCK_SECOND * (TCP_CLIENT_CONNECT_DELAY - SOURCE_NODE_START_DELAY));
                PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
                
                set_ipaddr_from_id(&global_ip_addr, TCP_SERVER_NODE_ID);
                printf("connecting..\n");
                tcp_connect(&global_ip_addr, UIP_HTONS(TCP_PORT), NULL);
            }
    
            while (1) {
    
                PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    
                if (uip_connected()) {

                    connected = 1; 
                    printf("connected\n");
                    PSOCK_INIT(&p_socket, (uint8_t *) &echo_msg_buf, sizeof(echo_msg_t));
    
                    while(!(uip_aborted() || uip_closed() || uip_timedout())) {
                        
                        PROCESS_YIELD();
                        
                        if (node_id == TCP_SERVER_NODE_ID && ev == tcpip_event) {
                            server_handler();
                        } else if (node_id == TCP_CLIENT_NODE_ID && ev == tcpip_event) {
                            client_handler();
                        } else {
                            // Other event
                        }
                    }
                    
                    printf("aborted|closed|timedout\n");
                    
                } else if (node_id == TCP_CLIENT_NODE_ID && 
                            (uip_aborted() || uip_closed() || uip_timedout())) {
                                
                    printf("aborted|closed|timedout-reconnecting\n");
                    tcp_connect(&global_ip_addr, UIP_HTONS(TCP_PORT), NULL);
                }
            }
        } else {
            // Other node id
        }
    }

    PROCESS_END();
}

