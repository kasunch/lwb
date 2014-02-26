#include "contiki.h"
#include "lib/memb.h"
#include "lib/list.h"

#include "glossy.h"

#include "lwb-common.h"
#include "lwb-scheduler.h"
#include "lwb-macros.h"


// Configurations
#ifndef LWB_CONF_MAX_N_STREAMS
#define LWB_CONF_MAX_N_STREAMS  20
#endif

typedef struct lwb_stream_info {
    struct lwb_stream_info *next;
    uint16_t ui16_node_id;             ///< Node ID
    uint16_t ui16_ipi;                 ///< Inter-packet interval in seconds
    uint16_t ui16_last_assigned;
    uint16_t ui16_next_ready;
    uint8_t  ui8_stream_id;           ///< Stream ID
    uint8_t  ui8_n_cons_missed;       ///< Number of consecutive slot misses for the stream
} lwb_stream_info_t;


extern uint16_t node_id;

extern lwb_context_t lwb_context;

MEMB(streams_memb, lwb_stream_info_t, LWB_CONF_MAX_N_STREAMS);

LIST(streams_list);

static uint16_t ui16_n_streams = 0;

static uint8_t ui8_n_slots_assigned = 0;

//--------------------------------------------------------------------------------------------------
static void inline add_stream(uint16_t ui16_node_id, lwb_stream_req_t *p_req) {

    lwb_stream_info_t *prev_stream;
    lwb_stream_info_t *stream;

    // insert the stream into the list, ordered by node id
    for (prev_stream = list_head(streams_list); prev_stream != NULL; prev_stream = prev_stream->next) {

        if (ui16_node_id >= prev_stream->ui16_node_id) {

            if (ui16_node_id == prev_stream->ui16_node_id &&
                GET_STREAM_ID(p_req->ui8_req_type) == prev_stream->ui8_stream_id) {
                // We have a stream add request with an existing stream ID.
                // This could happen due to the node has not received the stream acknowledgment.
                // So we add an acknowledgment from him. Otherwise he will keep sending stream
                // requests.

                LWB_STATS_SCHED(ui16_n_duplicates)++;

                if (lwb_context.ui8_n_stream_acks >= LWB_SCHED_MAX_SLOTS) {
                    // Maximum space for stream acks exceeded.
                    // We just ignore the stream request.
                    return;
                }

                lwb_context.ui16arr_stream_akcs[lwb_context.ui8_n_stream_acks++] = ui16_node_id;

                return;
            }

            if ((prev_stream->next == NULL) || (ui16_node_id < prev_stream->next->ui16_node_id)) {
                // We found the right place to add new stream information
                break;
            }
        }
    }

    stream = memb_alloc(&streams_memb); // Allocate memory for the stream information
    if (stream == NULL) {
        // Oops..! no space for new streams
        LWB_STATS_SCHED(ui16_n_no_space)++;
        return;
    }

    stream->ui16_node_id = ui16_node_id;
    stream->ui16_ipi = p_req->ui16_ipi;
    stream->ui16_last_assigned = p_req->ui16_time_info - p_req->ui16_ipi;
    stream->ui16_next_ready = p_req->ui16_time_info;
    stream->ui8_stream_id = GET_STREAM_ID(p_req->ui8_req_type);
    stream->ui8_n_cons_missed = 0;

    //  list_add(streams_list, stream);
    list_insert(streams_list, prev_stream, stream);
    ui16_n_streams++;
    LWB_STATS_SCHED(ui16_n_added)++;
}

//--------------------------------------------------------------------------------------------------
static void inline del_stream(uint16_t id, lwb_stream_req_t *p_req) {

    lwb_stream_info_t *prev_stream;

    for (prev_stream = list_head(streams_list); prev_stream != NULL; prev_stream = prev_stream->next) {

        if ((id == prev_stream->ui16_node_id) &&
            (GET_STREAM_ID(p_req->ui8_req_type) == prev_stream->ui8_stream_id)) {

            list_remove(streams_list, prev_stream);
            ui16_n_streams--;
            return;
        }
    }
}

//--------------------------------------------------------------------------------------------------
void lwb_sched_init(void) {
    // Initialize streams memory blocks and list
    memb_init(&streams_memb);
    list_init(streams_list);
}

//--------------------------------------------------------------------------------------------------
void lwb_sched_compute_schedule(lwb_schedule_t* p_sched) {

    lwb_stream_info_t *prev_stream;


    p_sched->sched_info.ui16_host_id = node_id;
    p_sched->sched_info.ui16_time = UI32_GET_LOW(lwb_context.ui32_time);


    ui8_n_slots_assigned = 0;

    if (lwb_context.ui8_n_stream_acks > 0) {
        p_sched->ui16arr_slots[ui8_n_slots_assigned++] = 0;
    }


    for (prev_stream = list_head(streams_list);
         prev_stream != NULL && ui8_n_slots_assigned < LWB_SCHED_MAX_SLOTS;
         prev_stream = prev_stream->next) {
        p_sched->ui16arr_slots[ui8_n_slots_assigned++] = prev_stream->ui16_node_id;
    }


    p_sched->sched_info.ui8_T = 1;
    SET_N_FREE_SLOTS(p_sched->sched_info.ui8_n_slots, 2);
    SET_N_DATA_SLOTS(p_sched->sched_info.ui8_n_slots, ui8_n_slots_assigned);
}

//--------------------------------------------------------------------------------------------------
void lwb_sched_process_stream_req(uint16_t ui16_id, lwb_stream_req_t *p_req) {

    switch (GET_STREAM_TYPE(p_req->ui8_req_type)) {
        case LWB_STREAM_TYPE_ADD:
            add_stream(ui16_id, p_req);
            break;
        case LWB_STREAM_TYPE_DEL:
            del_stream(ui16_id, p_req);
            break;
        case LWB_STREAM_TYPE_MOD:
            add_stream(ui16_id, p_req);
            del_stream(ui16_id, p_req);
            break;
        default:
            break;
    }

}
