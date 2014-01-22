#include <stdio.h>

#include "contiki.h"
#include "lwb.h"


PROCESS(lwb_test_process, "lwb test");
AUTOSTART_PROCESSES(&lwb_test_process);


#define LWB_HOST_ID             1
#define LWB_DATA_ECHO_CLIENT    4
#define LWB_DATA_ECHO_SERVER    5


extern uint16_t node_id;

static struct etimer et1;
static uint8_t ui8_i = 0;
static uint8_t buffer[50];
static uint8_t ui8_buf_len = 0;

//--------------------------------------------------------------------------------------------------
void on_data(uint8_t *p_data, uint8_t ui8_len, uint16_t ui16_from_id) {

    if (node_id == LWB_DATA_ECHO_SERVER) {

        printf("server: on_data from %u : %s\n", ui16_from_id, (char*)p_data);

        ui8_buf_len = (uint8_t)sprintf((char*)buffer, "echo %s", (char*)p_data);
        lwb_queue_packet(buffer, ui8_buf_len, LWB_DATA_ECHO_CLIENT);

    } else if(node_id == LWB_DATA_ECHO_CLIENT) {

        printf("client: on_data from %u : %s\n", ui16_from_id, (char*)p_data);
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

            etimer_set(&et1, CLOCK_SECOND * 2);

            while (1) {
                PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et1));
                ui8_buf_len = (uint8_t)sprintf((char*)buffer, "count %u", ui8_i++);
                lwb_queue_packet(buffer, ui8_buf_len, LWB_DATA_ECHO_SERVER);
                etimer_set(&et1, CLOCK_SECOND * 2);
            }
        } else if (node_id == LWB_DATA_ECHO_SERVER) {

            lwb_request_stream_add(2, 2);

        }
    }


    PROCESS_END();

    return PT_ENDED;
}

