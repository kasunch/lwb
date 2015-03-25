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


#define TCP_PAYLOAD_LEN_FRAG_1      70
#define TCP_PAYLOAD_LEN_FRAG_2      170
#define TCP_PAYLOAD_LEN_FRAG_3      270
#define TCP_PAYLOAD_LEN_FRAG_4      370

// Available for application data
// 127 - (1 (relay counter) + 1 (Glossy header) + 8 (LWB data header) + 20 (compressed IP header) + 20 (TCP))
// If the total payload is greater than (127 - (1 (relay counter) + 1 (Glossy header) + 8 (LWB data header)),
// 6LoWPAN fragmentation headers (4 (FAG_1) + 5 (FAG_N)) should also be considered.
// NOTE the size of 6LoWPAN fragments should be multiples of 8
#define TCP_PAYLOAD_LEN             TCP_PAYLOAD_LEN_FRAG_1

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

uint8_t is_server = 0;
uint8_t is_client = 0;
uint16_t client_start_time = 0;
uint16_t server_node_id = 0;

typedef struct {
    uint16_t client;
    uint16_t server;
    uint16_t start_time;
} tcp_client_server_t;

#define TCP_PORT                    20222
#define LWB_HOST_NODE_ID            1
// These delays are from the boot of the node
#define SOURCE_NODE_START_DELAY     15
tcp_client_server_t clients_servers[] = {   {2, 13, 60},
                                            {3, 10, 120},
                                            {4, 11, 180},
                                            {5, 12, 240},
                                            {6, 14, 300}
                                        };

static void set_servers_and_clients() {
    tcp_client_server_t* pair;
    uint8_t i;

    for (i = 0; i < sizeof(clients_servers)/sizeof(tcp_client_server_t); i++) {
        pair = &clients_servers[i];

        if (node_id == pair->client) {
            is_client = 1;
            client_start_time = pair->start_time;
            server_node_id = pair->server;
        } else if (node_id == pair->server) {
            is_server = 1;
        }
    }
}

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

    if (is_client && connected) {
        printf("P|");
        for(; i < tcpstats_cnt; i++) {
            printf("%u-%lu ", tcpstats_info[i].seq, (tcpstats_info[i].lat * 1000 / RTIMER_SECOND));
        }
        printf("|%lu %lu %lu", n_packets, (t_con_now - t_con_start), lwb_get_host_time());
        printf("\n");
    }

    tcpstats_cnt = 0;

    if((is_client || is_server) && connected) {
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


        set_servers_and_clients();

        if (is_server) {
            tcp_listen(UIP_HTONS(TCP_PORT));

        } else if (is_client) {

            while (client_start_time > lwb_get_host_time()) {
                etimer_set(&et, CLOCK_SECOND);
                PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
            }

            set_ipaddr_from_id(&global_ip_addr, server_node_id);
            printf("connecting..\n");
            tcp_connect(&global_ip_addr, UIP_HTONS(TCP_PORT), NULL);
        }


        if (is_client || is_server) {
            while (1) {

                PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

                if (uip_connected()) {

                    connected = 1;
                    printf("connected\n");
                    PSOCK_INIT(&p_socket, (uint8_t *) &echo_msg_buf, sizeof(echo_msg_t));

                    while(!(uip_aborted() || uip_closed() || uip_timedout())) {

                        PROCESS_YIELD();

                        if (is_server && ev == tcpip_event) {
                            server_handler();
                        } else if (is_client && ev == tcpip_event) {
                            client_handler();
                        } else {
                            // Other event
                        }
                    }

                    printf("aborted|closed|timedout\n");

                } else if (is_client &&
                            (uip_aborted() || uip_closed() || uip_timedout())) {

                    printf("aborted|closed|timedout-reconnecting\n");
                    tcp_connect(&global_ip_addr, UIP_HTONS(TCP_PORT), NULL);
                }
            }
        }
    }

    PROCESS_END();
}

