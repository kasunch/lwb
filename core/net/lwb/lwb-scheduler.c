#include <string.h>

#include "contiki.h"
#include "lib/memb.h"
#include "lib/list.h"
#include "lib/random.h"

#include "glossy.h"

#include "lwb-common.h"
#include "lwb-scheduler.h"
#include "lwb-macros.h"

#define T_ROUND_PERIOD_MIN  1

extern uint16_t node_id;

extern lwb_context_t lwb_context;

MEMB(streams_memb, lwb_stream_info_t, LWB_MAX_N_STREAMS);

LIST(streams_list);
static uint16_t n_streams = 0;

lwb_stream_info_t* curr_sched_streams[LWB_SCHED_MAX_SLOTS];  ///< Pointer to the lwb_stream_info_t
uint8_t n_curr_streams;                                      ///< Number of streams

LIST(backlog_slot_list);
static uint16_t n_backlog_slots = 0;

//--------------------------------------------------------------------------------------------------
static void inline add_stream(uint16_t ui16_node_id, lwb_stream_req_t *p_req) {

    lwb_stream_info_t *prev_stream;
    lwb_stream_info_t *stream;

    // insert the stream into the list, ordered by node id
    for (prev_stream = list_head(streams_list); prev_stream != NULL; prev_stream = prev_stream->next) {

        if (ui16_node_id >= prev_stream->node_id) {

            if (ui16_node_id == prev_stream->node_id &&
                GET_STREAM_ID(p_req->req_type) == prev_stream->stream_id) {
                // We have a stream add request with an existing stream ID.
                // This could happen due to the node has not received the stream acknowledgment.
                // So we add an acknowledgment from him. Otherwise he will keep sending stream
                // requests.

                LWB_STATS_SCHED(ui16_n_duplicates)++;

                if (lwb_context.n_stream_acks >= LWB_SCHED_MAX_SLOTS) {
                    // Maximum space for stream acks exceeded.
                    // We just ignore the stream request.
                    return;
                }

                lwb_context.ui16arr_stream_akcs[lwb_context.n_stream_acks++] = ui16_node_id;

                return;
            }

            if ((prev_stream->next == NULL) || (ui16_node_id < prev_stream->next->node_id)) {
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

    memset(stream, 0, sizeof(lwb_stream_info_t));
    stream->node_id = ui16_node_id;
    stream->ipi = p_req->ipi;
    stream->last_assigned = p_req->time_info - p_req->ipi;
    stream->next_ready = p_req->time_info;
    stream->stream_id = GET_STREAM_ID(p_req->req_type);

    list_insert(streams_list, prev_stream, stream);
    n_streams++;

    LWB_STATS_SCHED(ui16_n_added)++;
}

//--------------------------------------------------------------------------------------------------
static void del_stream_ex(lwb_stream_info_t *stream) {

    if (stream == NULL) {
        return;
    }
    list_remove(streams_list, stream);
    memb_free(&streams_memb, stream);
    n_streams--;
}

//--------------------------------------------------------------------------------------------------
static void inline del_stream(uint16_t id, lwb_stream_req_t *p_req) {

    lwb_stream_info_t *prev_stream;

    for (prev_stream = list_head(streams_list); prev_stream != NULL; prev_stream = prev_stream->next) {

        if ((id == prev_stream->node_id) &&
            (GET_STREAM_ID(p_req->req_type) == prev_stream->stream_id)) {
            del_stream_ex(prev_stream);
            return;
        }
    }
}

//--------------------------------------------------------------------------------------------------
static void delete_all_from_backlog(lwb_stream_info_t *stream) {
    lwb_stream_info_t *crr_stream;
    lwb_stream_info_t *to_be_removed;

    if (stream == NULL) {
        return;
    }

    for (crr_stream = list_head(backlog_slot_list); crr_stream != NULL;) {
        if (crr_stream == stream) {
            to_be_removed = crr_stream;
            crr_stream = crr_stream->next;
            list_remove(backlog_slot_list, to_be_removed);
            n_backlog_slots--;
        } else {
            crr_stream = crr_stream->next;
        }

    }
}

//--------------------------------------------------------------------------------------------------
void lwb_sched_init(void) {
    // Initialize streams memory blocks and list
    memb_init(&streams_memb);
    list_init(streams_list);
    list_init(backlog_slot_list);
}

//--------------------------------------------------------------------------------------------------
void lwb_sched_compute_schedule(lwb_schedule_t* p_sched) {

    uint8_t n_free_slots;
    uint8_t n_possible_data_slots;
    uint8_t n_data_slots_assigned = 0;
    lwb_stream_info_t *crr_stream;
    lwb_stream_info_t *to_be_removed;

    // Recycle unused data slots based on activity
    for (crr_stream = list_head(streams_list); crr_stream != NULL;) {

        LWB_STATS_SCHED(ui16_n_unused_slots) += crr_stream->n_slots_allocated - crr_stream->n_slots_used;
        // Reset counters before computing schedule
        crr_stream->n_slots_allocated = 0;
        crr_stream->n_slots_used = 0;

        if (crr_stream->n_cons_missed > LWB_SCHED_N_CONS_MISSED_MAX) {

            to_be_removed = crr_stream;
            crr_stream = crr_stream->next;

            // Delete corresponding backlog slots
            delete_all_from_backlog(to_be_removed);
            // Delete from streams list and free the allocated memory.
            del_stream_ex(to_be_removed);

        } else {
            crr_stream = crr_stream->next;
        }
    }


    // reset current streams
    memset(curr_sched_streams, 0, sizeof(lwb_stream_info_t*) * LWB_SCHED_MAX_SLOTS);
    n_curr_streams = 0;

    n_free_slots = (uint8_t)(random_rand() % 2);

    n_possible_data_slots = ((T_ROUND_PERIOD_MIN * RTIMER_SECOND) -
                                (T_SYNC_ON + (n_free_slots * T_FREE_ON) + T_COMP)) / (T_GAP + T_RR_ON);

    if (lwb_context.n_stream_acks > 0) {
        // We have acks to be sent.
        curr_sched_streams[n_curr_streams++] = NULL; // There is no stream associated for acks
        p_sched->slots[n_data_slots_assigned++] = 0;
    }

    // Allocate slots from the backlog first
    for (crr_stream = list_head(backlog_slot_list);
                    crr_stream != NULL && n_data_slots_assigned < n_possible_data_slots;) {

        p_sched->slots[n_data_slots_assigned++] = crr_stream->node_id;
        crr_stream->n_slots_allocated++;
        curr_sched_streams[n_curr_streams++] = crr_stream;
        // Remove the slot from backlog
        to_be_removed = crr_stream;
        crr_stream = crr_stream->next;
        list_remove(backlog_slot_list, to_be_removed);
        n_backlog_slots--;
    }

    // Allocate slots based on the main streams
    while (n_streams != 0 && n_data_slots_assigned < n_possible_data_slots) {

        for (crr_stream = list_head(streams_list);
                        crr_stream != NULL;
                        crr_stream = crr_stream->next) {

            if (n_data_slots_assigned < n_possible_data_slots) {
                // We have enough space in the schedule
                p_sched->slots[n_data_slots_assigned++] = crr_stream->node_id;
                crr_stream->n_slots_allocated++;
                curr_sched_streams[n_curr_streams++] = crr_stream;
            } else {
                // No remaining slots in this schedule. So adding to the backlog
                list_add(backlog_slot_list, crr_stream);
                n_backlog_slots++;
            }

        }
    }

    // Set number of free and assigned data slots
    SET_N_FREE_SLOTS(p_sched->sched_info.n_slots, n_free_slots);
    SET_N_DATA_SLOTS(p_sched->sched_info.n_slots, n_data_slots_assigned);

    p_sched->sched_info.host_id = node_id;
    p_sched->sched_info.time = UI32_GET_LOW(lwb_context.ui32_time);
    p_sched->sched_info.round_period = T_ROUND_PERIOD_MIN;

}

//--------------------------------------------------------------------------------------------------
void lwb_sched_process_stream_req(uint16_t ui16_id, lwb_stream_req_t *p_req) {

    switch (GET_STREAM_TYPE(p_req->req_type)) {
        case LWB_STREAM_TYPE_ADD:
            add_stream(ui16_id, p_req);
            break;
        case LWB_STREAM_TYPE_DEL:
            del_stream(ui16_id, p_req);
            break;
        case LWB_STREAM_TYPE_MOD:
            del_stream(ui16_id, p_req);
            add_stream(ui16_id, p_req);
            break;
        default:
            break;
    }

}

//--------------------------------------------------------------------------------------------------

void lwb_sched_update_data_slot_usage(uint8_t slot_index, uint8_t used) {

    if (slot_index > 0 && slot_index < n_curr_streams && curr_sched_streams[slot_index]) {

        if (used) {
            curr_sched_streams[slot_index]->n_slots_used++;
            curr_sched_streams[slot_index]->n_cons_missed = 0;
        } else {
            curr_sched_streams[slot_index]->n_cons_missed++;
        }
    }
}
