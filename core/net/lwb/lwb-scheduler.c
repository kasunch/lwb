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


typedef struct backlog_slot_item {
    struct backlog_slot_item *next;
    lwb_stream_info_t* stream;
} backlog_slot_item_t;

MEMB(backlog_slot_memb, backlog_slot_item_t, LWB_SCHED_MAX_SLOTS);
LIST(backlog_slot_list);
static uint16_t n_backlog_slots = 0;

//--------------------------------------------------------------------------------------------------
static void inline add_stream(uint16_t node_id, lwb_stream_req_t *p_req) {

    lwb_stream_info_t *crr_stream;

    for (crr_stream = list_head(streams_list); crr_stream != NULL; crr_stream = crr_stream->next) {

        if (node_id == crr_stream->node_id &&
                        GET_STREAM_ID(p_req->req_type) == crr_stream->stream_id) {

            // We have a stream add request with an existing stream ID.
            // This could happen due to the node has not received the stream acknowledgement.
            // So we add an acknowledgement from him. Otherwise he will keep sending stream
            // requests.

            LWB_STATS_SCHED(n_duplicates)++;

            if (lwb_context.n_stream_acks >= LWB_SCHED_MAX_SLOTS) {
                // Maximum space for stream acks exceeded.
                // We just ignore the stream request.
                return;
            }

            lwb_context.ui16arr_stream_akcs[lwb_context.n_stream_acks++] = node_id;

        }
    }

    crr_stream = memb_alloc(&streams_memb);
    if (crr_stream == NULL) {
        // Oops..! no space for new streams
        LWB_STATS_SCHED(n_no_space)++;
        return;
    }

    memset(crr_stream, 0, sizeof(lwb_stream_info_t));
    crr_stream->node_id = node_id;
    crr_stream->ipi = p_req->ipi;
    crr_stream->last_assigned = p_req->time_info - p_req->ipi;
    crr_stream->next_ready = p_req->time_info;
    crr_stream->stream_id = GET_STREAM_ID(p_req->req_type);

    list_add(streams_list, crr_stream);
    n_streams++;

    LWB_STATS_SCHED(n_added)++;

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
static inline void del_stream(uint16_t id, lwb_stream_req_t *p_req) {

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
static inline void delete_all_from_backlog(lwb_stream_info_t *stream) {
    backlog_slot_item_t* item;
    backlog_slot_item_t* removed;

    for (item = list_head(backlog_slot_list); item != NULL;) {
        if (item->stream == stream) {
            removed = item;
            item = item->next;
            list_remove(backlog_slot_list, removed);
            memb_free(&backlog_slot_memb, removed);
            n_backlog_slots--;
        } else {
            item = item->next;
        }
    }
}

static inline void add_to_backlog(lwb_stream_info_t *stream) {
    backlog_slot_item_t* item = memb_alloc(&backlog_slot_memb);
    if (item) {
        item->stream = stream;
        list_add(backlog_slot_list, item);
        n_backlog_slots++;
    }
}

//--------------------------------------------------------------------------------------------------
void lwb_sched_init(void) {
    // Initialize streams memory blocks and list
    memb_init(&streams_memb);
    list_init(streams_list);
    n_streams = 0;
    memb_init(&backlog_slot_memb);
    list_init(backlog_slot_list);
    n_backlog_slots = 0;
}

//--------------------------------------------------------------------------------------------------
void lwb_sched_compute_schedule(lwb_schedule_t* p_sched) {

    uint8_t n_free_slots;
    uint8_t n_possible_data_slots;
    uint8_t n_data_slots_assigned = 0;
    lwb_stream_info_t *crr_stream;
    lwb_stream_info_t *to_be_removed;
    backlog_slot_item_t* item;

    // Recycle unused data slots based on activity
    for (crr_stream = list_head(streams_list); crr_stream != NULL;) {

        LWB_STATS_SCHED(n_unused_slots) += crr_stream->n_slots_allocated - crr_stream->n_slots_used;
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

    //n_possible_data_slots = ((T_ROUND_PERIOD_MIN * RTIMER_SECOND) -
    //                           T_SYNC_ON -
    //                           T_COMP -
    //                           (n_free_slots * T_FREE_ON) -
    //                           (n_free_slots * T_GAP)) / (T_GAP + T_RR_ON);


    if (lwb_context.n_stream_acks > 0) {
        // We have acks to be sent.
        curr_sched_streams[n_curr_streams++] = NULL; // There is no stream associated for acks
        p_sched->slots[n_data_slots_assigned++] = 0;
    }

    // Allocate slots from the backlog first
    while ((item = list_pop(backlog_slot_list)) != NULL) {
        p_sched->slots[n_data_slots_assigned++] = item->stream->node_id;
        item->stream->n_slots_allocated++;
        curr_sched_streams[n_curr_streams++] = item->stream;
        memb_free(&backlog_slot_memb, item);
    }
    n_backlog_slots = 0;

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
                add_to_backlog(crr_stream);
            }

        }
    }

    // Set number of free and assigned data slots
    SET_N_FREE_SLOTS(p_sched->sched_info.n_slots, n_free_slots);
    SET_N_DATA_SLOTS(p_sched->sched_info.n_slots, n_data_slots_assigned);

    p_sched->sched_info.host_id = node_id;
    p_sched->sched_info.time = UI32_GET_LOW(lwb_context.time);
    p_sched->sched_info.round_period = T_ROUND_PERIOD_MIN;

}

//--------------------------------------------------------------------------------------------------
void lwb_sched_process_stream_req(uint16_t id, lwb_stream_req_t *p_req) {

    switch (GET_STREAM_TYPE(p_req->req_type)) {
        case LWB_STREAM_TYPE_ADD:
            add_stream(id, p_req);
            break;
        case LWB_STREAM_TYPE_DEL:
            del_stream(id, p_req);
            break;
        case LWB_STREAM_TYPE_MOD:
            del_stream(id, p_req);
            add_stream(id, p_req);
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

void lwb_sched_print() {
    printf("SH|%u %u |%lu\n", n_streams, n_backlog_slots, lwb_context.time);
}
