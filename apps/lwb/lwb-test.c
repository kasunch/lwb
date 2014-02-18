#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "lwb.h"


PROCESS(lwb_test_process, "lwb test");
AUTOSTART_PROCESSES(&lwb_test_process);


#define LWB_HOST_ID             1
#define LWB_DATA_ECHO_CLIENT    4
#define LWB_DATA_ECHO_SERVER    5


extern uint16_t node_id;

static struct etimer et;
static uint8_t buffer[100];
static uint8_t ui8_buf_len = 0;
static uint16_t ui16_c = 0;

//--------------------------------------------------------------------------------------------------
void on_data(uint8_t *p_data, uint8_t ui8_len, uint16_t ui16_from_id) {

    uint16_t ui16_val;
    if (node_id == LWB_DATA_ECHO_SERVER) {

        ui16_val = (uint16_t)p_data[0] | ((uint16_t)(p_data[1]) << 8);
        printf("server: on_data from %u len %u, counter %u\n", ui16_from_id, ui8_len, ui16_val);

        ui16_val = ui16_val * 2;
        p_data[0] = ui16_val & 0xFF;
        p_data[1] = (ui16_val >> 8) & 0xFF;
        lwb_queue_packet(p_data, ui8_len, LWB_DATA_ECHO_CLIENT);

    } else if(node_id == LWB_DATA_ECHO_CLIENT) {

        ui16_val = (uint16_t)p_data[0] | ((uint16_t)(p_data[1]) << 8);
        printf("client: on_data from %u len %u, counter %u\n", ui16_from_id, ui8_len, ui16_val);
    }
}

//--------------------------------------------------------------------------------------------------
void on_schedule_end(void) {

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

            lwb_request_stream_add(2, 2);

            etimer_set(&et, CLOCK_SECOND * 2);

            while (1) {
                PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
                buffer[0] = ui16_c & 0xFF;
                buffer[1] = (ui16_c >> 8) & 0xFF;
                memset(buffer + 2, 1, 80);
                ui8_buf_len = 82;
                ui16_c++;
                lwb_queue_packet(buffer, ui8_buf_len, 0);
                etimer_set(&et, CLOCK_SECOND * 2);
            }

        } else if (node_id == LWB_DATA_ECHO_SERVER) {
            lwb_request_stream_add(2, 2);
        }
    }


    PROCESS_END();

    return PT_ENDED;
}

