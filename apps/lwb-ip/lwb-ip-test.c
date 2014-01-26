#include <stdio.h>

#include "contiki.h"
#include "lwb.h"

#include "uip6/uip.h"
#include "uip6/uip-ds6.h"
#include "uip6/psock.h"


PROCESS(lwb_test_process, "lwb test");
AUTOSTART_PROCESSES(&lwb_test_process);


#define LWB_CONF_HOST_ID             1

#define LWB_CONF_DATA_ECHO_CLIENT    2
#define LWB_CONF_DATA_ECHO_SERVER    3

//#define LWB_CONF_DATA_ECHO_CLIENT    7
//#define LWB_CONF_DATA_ECHO_SERVER    11

#define TCP_PORT            UIP_HTONS(20222)

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[UIP_LLIPH_LEN])

enum {
  ECHO_MSG_TYPE_REQ = 1,
  ECHO_MSG_TYPE_RES
};

typedef struct __attribute__ ((__packed__)) echo_msg {
  uint8_t ui8_type;
  uint8_t ui8_counter_l;
  uint8_t ui8_counter_h;
} echo_msg_t;

extern uint16_t node_id;

static uip_ipaddr_t ipaddr;
uint16_t ui16_echo_counter = 0;
echo_msg_t echo_msg_buf;
struct psock p_socket;

//--------------------------------------------------------------------------------------------------
uint8_t lwb_tcpip_input(uip_lladdr_t *a) {
    lwb_queue_packet((uint8_t *)UIP_IP_BUF, uip_len, 0);
    return 1;
}

//--------------------------------------------------------------------------------------------------
void on_data(uint8_t *p_data, uint8_t ui8_len, uint16_t ui16_from_id) {
    memcpy((uint8_t *)UIP_IP_BUF, p_data, ui8_len);
    uip_len = ui8_len;
    tcpip_input();
}

//--------------------------------------------------------------------------------------------------
void on_schedule_end(void) {

}

lwb_callbacks_t lwb_callbacks = {on_data, on_schedule_end};

//--------------------------------------------------------------------------------------------------
static void
set_global_address(void)
{
    uip_ipaddr_t node_ipaddr;
    uip_ip6addr(&node_ipaddr, 0xfec0, 0, 0, 0, 0, 0, 0, node_id);
    uip_ds6_addr_add(&node_ipaddr, 0, ADDR_MANUAL);
}

//--------------------------------------------------------------------------------------------------
static PT_THREAD(server_handler())
{
    PSOCK_BEGIN(&p_socket);

    PSOCK_WAIT_UNTIL(&p_socket, PSOCK_NEWDATA(&p_socket));

    PSOCK_READBUF_LEN(&p_socket, sizeof(echo_msg_t));

    if (echo_msg_buf.ui8_type == ECHO_MSG_TYPE_REQ) {

        uint16_t c = (uint16_t)echo_msg_buf.ui8_counter_l + ((uint16_t)(echo_msg_buf.ui8_counter_h) << 8);
        printf("received %u\n", c);

        echo_msg_buf.ui8_type = ECHO_MSG_TYPE_RES;
        PSOCK_SEND(&p_socket, (uint8_t *) &echo_msg_buf, sizeof(echo_msg_t));
    }

    PSOCK_END(&p_socket);

    return PT_ENDED;
}

//--------------------------------------------------------------------------------------------------
static PT_THREAD(client_handler())
{
    PSOCK_BEGIN(&p_socket);

    echo_msg_buf.ui8_counter_l = (uint8_t)(ui16_echo_counter & 0xFF);
    echo_msg_buf.ui8_counter_h = (uint8_t)(ui16_echo_counter >> 8);
    echo_msg_buf.ui8_type = ECHO_MSG_TYPE_REQ;

    printf("sending %u\n", ui16_echo_counter);

    PSOCK_SEND(&p_socket, (uint8_t *) &echo_msg_buf, sizeof(echo_msg_t));

    PSOCK_WAIT_UNTIL(&p_socket, PSOCK_NEWDATA(&p_socket));

    PSOCK_READBUF_LEN(&p_socket, sizeof(echo_msg_t));

    if (echo_msg_buf.ui8_type == ECHO_MSG_TYPE_RES) {

        uint16_t c = (uint16_t)echo_msg_buf.ui8_counter_l + ((uint16_t)(echo_msg_buf.ui8_counter_h) << 8);
        printf("received %u\n", c);
        ui16_echo_counter++;
    }

    PSOCK_END(&p_socket);

    return PT_ENDED;
}

//--------------------------------------------------------------------------------------------------
PROCESS_THREAD(lwb_test_process, ev, data)
{
    PROCESS_BEGIN();

    if(node_id == LWB_CONF_HOST_ID) {
        lwb_init(LWB_MODE_HOST, &lwb_callbacks);
    } else {
        lwb_init(LWB_MODE_SOURCE, &lwb_callbacks);
    }

    tcpip_set_outputfunc(&lwb_tcpip_input);
    process_start(&tcpip_process, NULL);
    set_global_address();

    if (node_id == LWB_CONF_DATA_ECHO_SERVER) {
        lwb_request_stream_add(1, 0);
        tcp_listen(TCP_PORT);
    } else if (node_id == LWB_CONF_DATA_ECHO_CLIENT) {
        lwb_request_stream_add(1, 0);
        uip_ip6addr(&ipaddr, 0xfec0, 0, 0, 0, 0, 0, 0, LWB_CONF_DATA_ECHO_SERVER);
        tcp_connect(&ipaddr, TCP_PORT, NULL);
    }

    while (1) {

        PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

        if (uip_connected()) {

            printf("connected\n");
            PSOCK_INIT(&p_socket, (uint8_t *) &echo_msg_buf, sizeof(echo_msg_t));

            while(!(uip_aborted() || uip_closed() || uip_timedout())) {

                PROCESS_YIELD();

                if (node_id == LWB_CONF_DATA_ECHO_CLIENT && ev == tcpip_event) {

                    client_handler();

                } else if (node_id == LWB_CONF_DATA_ECHO_SERVER && ev == tcpip_event) {

                    server_handler();

                } else {
                    // Other nodes
                }
            }
        }
    }

    PROCESS_END();

    return PT_ENDED;
}

