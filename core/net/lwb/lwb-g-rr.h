#ifndef __LWB_G_RR_H__
#define __LWB_G_RR_H__

/// @file lwb-g-rr.h

#include "contiki.h"
#include "lwb-common.h"

PT_THREAD(lwb_g_rr_host(struct rtimer *t, lwb_context_t *p_context));

PT_THREAD(lwb_g_rr_source(struct rtimer *t, lwb_context_t *p_context));

/// @brief Initialize g_rr
void lwb_g_rr_init();

/// @brief This function is used to queue application data to be sent over LWB
/// @param p_data A pointer to the data
/// @param ui8_len The length of the data
/// @param ui16_to_node_id The ID of the destination node
/// @return Non-zero if queuing is successful.
uint8_t lwb_g_rr_queue_packet(uint8_t* p_data, uint8_t ui8_len, uint16_t ui16_to_node_id);

/// @brief This function to be called to deliver data to application layer.
void lwb_g_rr_data_output();

/// @brief Request from LWB host to add a stream for the node
uint8_t lwb_g_rr_stream_add(uint16_t ui16_ipi, uint16_t ui16_time_offset);

/// @brief Request from LWB host to delete a stream for the node
void lwb_g_rr_stream_del(uint8_t ui8_id);

/// @brief Request from LWB host to modify a stream request.
void lwb_g_rr_stream_mod(uint8_t ui8_id, uint16_t ui16_ipi);

#endif // __LWB_G_RR_H__
