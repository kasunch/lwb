#ifndef __LWB_H__
#define __LWB_H__

/// @brief Application header file for LWB

#include "lwb-common.h"

/// @brief Initialize LWB.
/// @param ui8_mode The mode of LWB. @see lwb_mode_t
/// @param p_callbacks A pointer to callback functions
/// @return Non-zero if initialization is successful.
uint8_t lwb_init(uint8_t ui8_mode, lwb_callbacks_t *p_callbacks);

/// @brief Queue packet to be sent over LWB.
/// @param p_data A pointer to the data buffer.
/// @param ui8_len The length of data.
/// @param ui16_to_node_id The ID of the destination node
/// @return Non-zero if queuing is successful.
uint8_t lwb_queue_packet(uint8_t* p_data, uint8_t ui8_len, uint16_t ui16_to_node_id);

/// @brief Get the time of LWB in seconds from when host is started.
uint32_t lwb_get_host_time();

/// @brief Request from LWB host to add a stream for the node.
/// @param ui16_ipi Inter-packet interval in seconds.
/// @param ui16_time_offset The time offset when the slot should be allocated.
/// @return The ID of the stream
uint8_t lwb_request_stream_add(uint16_t ui16_ipi, uint16_t ui16_time_offset);

/// @brief Request from LWB host to delete a stream for the node
/// @param ui8_id The stream ID
void lwb_request_stream_del(uint8_t ui8_id);

/// @brief Request from LWB host to modify a stream request.
/// @param ui8_id The ID of the stream to be modified.
/// @param ui16_ipi New inter-packet interval
void lwb_request_stream_mod(uint8_t ui8_id, uint16_t ui16_ipi);

#endif // __LWB_H__