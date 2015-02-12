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
#define TCP_CLIENT_NODE_ID  14 
#define LWB_HOST_NODE_ID    15
#define TCP_PORT            20222

#define TCP_CLIENT_CONNECT_DELAY    10

enum {
  ECHO_MSG_TYPE_REQ = 1,
  ECHO_MSG_TYPE_RES
};

typedef struct __attribute__ ((__packed__)) echo_msg {
  uint8_t ui8_type;
  uint8_t ui8_counter_l;
  uint8_t ui8_counter_h;
  uint8_t ui8_pad;
  uint8_t dummy[26];
} echo_msg_t;

typedef struct tcpstats {
  uint16_t seq;
  uint32_t lat;
  uint32_t on;
  uint32_t tot;  
} tcpstats_t;

#define TCPSTATS_ELEMENTS 40

tcpstats_t tcpstats_info[TCPSTATS_ELEMENTS];
uint16_t tcpstats_cnt;

static uip_ipaddr_t global_ip_addr;
static uint16_t ui16_counter = 0;
static echo_msg_t echo_msg_buf;
struct psock p_socket;
clock_time_t t_start = 0, t_end = 0; 
uint32_t en_on_start = 0, en_on_end = 0;
uint32_t en_tot_start = 0, en_tot_end = 0;

PROCESS(tcp_server_process, "TCP process");
AUTOSTART_PROCESSES(&tcp_server_process);

void send_stream_req() {
    if (((lwb_get_joining_state() == LWB_JOINING_STATE_NOT_JOINED) ||
         (lwb_get_joining_state() == LWB_JOINING_STATE_JOINED)) && 
        (lwb_get_n_my_slots() == 0)) {
        lwb_request_stream_mod(1, 1);
    }
}

// Commented for 6LoWPAN integration
//uint8_t lwb_tcpip_input(const uip_lladdr_t *a) {
//    send_stream_req();
//    lwb_queue_packet((uint8_t *)UIP_IP_BUF, uip_len, 0);
//    return 1;
//}

uint8_t lwb_tcpip_input(uint8_t *data, uint8_t len) {
    send_stream_req();
    lwb_queue_packet(data, len, 0);
    return 1; 
}

void on_data(uint8_t *p_data, uint8_t ui8_len, uint16_t ui16_from_id) {
    // Commented for 6LoWPAN integration
    //memcpy((uint8_t *)UIP_IP_BUF, p_data, ui8_len);
    //uip_len = ui8_len;
    //tcpip_input();
    sicslowpan_input(p_data, ui8_len);
}

void on_schedule_end(void) {

    uint32_t en_on = 0;
    uint32_t en_tot = 0;
    //uint32_t dcycle = 0;
    uint8_t i = 0;

    printf("P|");
    for(; i < tcpstats_cnt; i++) {
        en_on += tcpstats_info[i].on;
        en_tot += tcpstats_info[i].tot;
        printf("%u-%lu ", tcpstats_info[i].seq, (tcpstats_info[i].tot * 1000 /RTIMER_SECOND));
    }
    printf("\n");

    //dcycle = en_on * 1e4 / en_tot;
    //printf("%lu.%lu, ", dcycle / 100, dcycle % 100);

    tcpstats_cnt = 0;

    if(node_id == TCP_SERVER_NODE_ID || node_id == TCP_CLIENT_NODE_ID) {
        printf("T|%u %u\n", UIP_STAT(uip_stat.tcp.rexmit), UIP_STAT(uip_stat.tcp.drop));
    }

    lwb_print_stats();
}

static PT_THREAD(server_handler())
{
    PSOCK_BEGIN(&p_socket);

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
    PSOCK_SEND(&p_socket, (uint8_t *) &echo_msg_buf, sizeof(echo_msg_t));

    while (1) {

        t_start = clock_time();
        en_on_start = energest_type_time(ENERGEST_TYPE_LISTEN) + energest_type_time(ENERGEST_TYPE_TRANSMIT);
        en_tot_start = energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM);

        PSOCK_WAIT_UNTIL(&p_socket, PSOCK_NEWDATA(&p_socket));
        PSOCK_READBUF_LEN(&p_socket, sizeof(echo_msg_t));
     
        t_end = clock_time(); 
        en_on_end = energest_type_time(ENERGEST_TYPE_LISTEN) + energest_type_time(ENERGEST_TYPE_TRANSMIT);
        en_tot_end = energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM);

        uint16_t c = (uint16_t)echo_msg_buf.ui8_counter_l + ((uint16_t)(echo_msg_buf.ui8_counter_h) << 8);
        uint32_t t_diff = (uint32_t)(t_end - t_start) * 1e6 / CLOCK_SECOND; // in microseconds

        if (tcpstats_cnt < TCPSTATS_ELEMENTS) {
            tcpstats_info[tcpstats_cnt].seq = c;
            tcpstats_info[tcpstats_cnt].lat = t_diff;
            tcpstats_info[tcpstats_cnt].on = en_on_end - en_on_start;
            tcpstats_info[tcpstats_cnt].tot = en_tot_end - en_tot_start;
            tcpstats_cnt++;
        }
    }

    PSOCK_END(&p_socket);

    return PT_ENDED;
}


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

    // Commented for 6LoWPAN integration
    //tcpip_set_outputfunc(&lwb_tcpip_input);

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
        
        lwb_init(LWB_MODE_SOURCE, &lwb_callbacks);

        if (node_id == TCP_SERVER_NODE_ID || node_id == TCP_CLIENT_NODE_ID) {
            
            if (node_id == TCP_SERVER_NODE_ID) {
                    
                tcp_listen(UIP_HTONS(TCP_PORT));
                
            } else {
                
                etimer_set(&et, CLOCK_SECOND * TCP_CLIENT_CONNECT_DELAY);
                PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
                
                set_ipaddr_from_id(&global_ip_addr, TCP_SERVER_NODE_ID);
                printf("connecting..\n");
                tcp_connect(&global_ip_addr, UIP_HTONS(TCP_PORT), NULL);
            }
    
            while (1) {
    
                PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    
                if (uip_connected()) {
                    
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

