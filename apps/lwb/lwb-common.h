#ifndef __LWB_COMMON_H__
#define __LWB_COMMON_H__

/// @file lwb-common.h
/// @brief Common header file

#include <stdint.h>

#include "contiki.h"

/// @defgroup Configurations
/// @{

#ifndef LWB_CONF_MAX_TXRX_BUF_LEN
#define LWB_CONF_MAX_TXRX_BUF_LEN    127
#endif

#ifndef LWB_CONF_SCHED_MAX_SLOTS
#define LWB_CONF_SCHED_MAX_SLOTS      20
#endif

/// @}


/// @brief LWB mode.
typedef enum {
    LWB_MODE_HOST = 1,  ///< LWB in host mode.
    LWB_MODE_SOURCE     ///< LWB in source mode.
} lwb_mode_t;

/// @brief Stream request types
typedef enum {
    LWB_STREAM_TYPE_ADD = 1,     ///< Stream add request.
    LWB_STREAM_TYPE_DEL,         ///< Stream delete request.
    LWB_STREAM_TYPE_MOD          ///< Stream repeat request. @note Not sure
} stream_req_types_t;

/// @brief LWB packet types
typedef enum {
    LWB_PKT_TYPE_NO_DATA = 0,   ///< No Type
    LWB_PKT_TYPE_STREAM_REQ,    ///< Stream requests
    LWB_PKT_TYPE_STREAM_ACK,    ///< Stream acknowledgments
    LWB_PKT_TYPE_DATA,          ///< Data packets
    LWB_PKT_TYPE_SCHED          ///< Schedule packets
} pkt_types_t;

/// @brief Synchronization states
typedef enum {
    LWB_SYNC_STATE_BOOTSTRAP = 0,      ///< Initial state. Not synchronized
    LWB_SYNC_STATE_QUASI_SYNCED,       ///< Received schedule when in BOOTSTRAP state. Clock drift is not estimated.
    LWB_SYNC_STATE_SYNCED,             ///< Received schedule and clock drift is estimated.
    LWB_SYNC_STATE_UNSYNCED_1,         ///< Missed schedule one time.
    LWB_SYNC_STATE_UNSYNCED_2,         ///< Missed schedule two times.
    LWB_SYNC_STATE_UNSYNCED_3,         ///< Missed schedule three times.
} lwb_sync_states_t;

typedef enum {
    LWB_JOINING_STATE_NOT_JOINED = 0,
    LWB_JOINING_STATE_JOINING,
    LWB_JOINING_STATE_PARTLY_JOINED,
    LWB_JOINING_STATE_JOINED
} lwb_joining_state_t;


typedef enum {
    LWB_POLL_FLAGS_DATA = 0,
    LWB_POLL_FLAGS_SCHED_END,
    LWB_POLL_FLAGS_SLOT_END
} lwb_poll_flags_t;

/// @brief Header to be used when sending data
typedef struct data_header {
    uint16_t ui16_from_id;  ///< From node ID
    uint16_t ui16_to_id;    ///< To node ID
    uint8_t  ui8_data_len;  ///< Data options
    uint8_t  ui8_options;   ///< Data options
} data_header_t;


/// @brief This header is used when sending stream requests as separate messages.
typedef struct lwb_stream_req_header {
    uint16_t ui16_from_id;      ///< ID of the node that send the packet
    uint8_t ui8_n_reqs;         ///< Number of stream requests
} lwb_stream_req_header_t;


/// @brief Structure for stream requests
typedef struct lwb_stream_req {
    uint16_t ui16_ipi;           ///< Inter-packet interval in seconds.
    uint16_t ui16_time_info;     ///< Starting time of the stream.
    uint8_t  ui8_req_type;       ///< Request type. Least significant 2 bits represent stream request type.
                                 ///  Most significant 6 bits represent request ID.
                                 ///  @see stream_req_types_t and GET_STREAM_ID and SET_STREAM_ID
} lwb_stream_req_t;

/// @brief Structure for the header of a schedule
typedef struct lwb_sched_info {
    uint16_t ui16_host_id;    ///< ID of the host.
    uint16_t ui16_time;       ///< Least significant 16 bits of the current time at the host.
    uint8_t  ui8_n_slots;     ///< Number of slots in the round.
                              ///  Most significant 2 bits represent free slots and least significant 6 bits represent data slots.
    uint8_t  ui8_T;           ///< Round period (duration between beginning of two rounds) in seconds
} lwb_sched_info_t;

/// @brief LWB schedule
typedef struct lwb_schedule {
    lwb_sched_info_t    sched_info;                                 ///< schedule information
    uint16_t            ui16arr_slots[LWB_CONF_SCHED_MAX_SLOTS];    ///< slots. The node ID will be stored.
} lwb_schedule_t;

/// @brief LWB callbacks
typedef struct lwb_callbacks {
    void (*p_on_data)(uint8_t*, uint8_t, uint16_t);
    void (*p_on_sched_end)(void);
} lwb_callbacks_t;


/// @brief Scheduler related statistics
typedef struct lwb_sched_stats {
    uint16_t ui16_n_added;          ///< Number of streams added so far
    uint16_t ui16_n_deleted;        ///< Number of streams deleted so far
    uint16_t ui16_n_no_space;       ///< Number of streams that are unable to add due to space unavailability
    uint16_t ui16_n_modified;       ///< Number of streams modified
    uint16_t ui16_n_duplicates;     ///< Number of duplicated stream requests
} lwb_sched_stats_t;

/// @brief Glossy synchronization related statistics
typedef struct lwb_sync_stats {
    uint16_t ui16_synced;           ///< Number of instances that the schedule is received
    uint16_t ui16_sync_missed;      ///< Number of instances that the schedule is not received
} lwb_sync_stats_t;

/// @brief Statistics related to data packets
typedef struct lwb_data_stats {
    uint16_t ui16_n_tx_nospace;     ///< Number of instances in which unable to allocate memory for transmitting data
    uint16_t ui16_n_rx_nospace;     ///< Number of instances in which unable to allocate memory for receiving data
    uint16_t ui16_n_tx;             ///< Number of data packets transmitted
    uint16_t ui16_n_rx;             ///< Number of data packets received
    uint16_t ui16_n_rx_dropped;     ///< Number of data packets dropped
} lwb_data_stats_t;

/// @brief Stream requests and acknowledgment related statistics
typedef struct lwb_stream_req_ack_stats {
    uint16_t ui16_n_req_tx;         ///< Number of stream requests sent
    uint16_t ui16_n_req_rx;         ///< Number of stream requests received
    uint16_t ui16_n_ack_tx;         ///< Number of stream acknowledgments sent
    uint16_t ui16_n_ack_rx;         ///< Number of stream acknowledgments received
} lwb_stream_req_ack_stats_t;


/// @brief LWB context
typedef struct lwb_context {
    lwb_schedule_t  current_sched;           ///< Current schedule.
    lwb_schedule_t  old_sched;               ///< Old schedule.
    uint32_t        ui32_time;               ///< Global time in seconds
    int32_t         i32_skew;                ///< Clock skew per unit time in rtimer ticks.
    uint16_t        ui16_t_sync_guard;       ///< Guard time that Glossy should be started earlier for receiving schedule.
    uint16_t        ui16_t_sync_ref;         ///< Reference time of the host when schedule is received in rtimer ticks.
    uint16_t        ui16_t_last_sync_ref;    ///< Reference time of the host when last schedule is received in rtimer ticks.
    uint16_t        ui16_t_start;            ///< Start time of the first schedule transmission in a round.
    lwb_callbacks_t *p_callbacks;            ///< Call back functions.
    uint8_t         ui8_lwb_mode;            ///< Mode of LWB @see lwb_mode_t
    struct rtimer  rt;                      ///< the real-time timer used to start glossy phases
    uint8_t         ui8arr_txrx_buf[LWB_CONF_MAX_TXRX_BUF_LEN]; ///< TX/RX buffer
    uint8_t         ui8_txrx_buf_len;        ///< The length of the data in TX/RX buffer
    uint8_t         ui8_poll_flags;          ///< Flags that indicate why LWB main process is polled.

    // source node
    uint8_t         ui8_sync_state;         ///< Synchronization state

    // hosts
    uint16_t        ui16arr_stream_akcs[LWB_CONF_SCHED_MAX_SLOTS];  ///< IDs of the nodes which stream acks to be sent
    uint8_t         ui8_n_stream_acks;                              ///< Number of stream acks

    // stats
    lwb_sync_stats_t            sync_stats;
    lwb_data_stats_t            data_stats;
    lwb_stream_req_ack_stats_t  stream_req_ack_stats;
    lwb_sched_stats_t           sched_stats;


} lwb_context_t;


/// @brief Structure for data buffer element.
typedef struct data_buf {
    data_header_t   header;
    uint8_t         data[LWB_CONF_MAX_TXRX_BUF_LEN];
} data_buf_t;

/// @brief Structure for data buffer element.
typedef struct data_buf_lst_item {
    struct data_buf* next;
    data_buf_t        buf;
} data_buf_lst_item_t;


typedef struct stream_req_lst_item {
    struct stream_req_lst_item*   next;
    lwb_stream_req_t            req;
} stream_req_lst_item_t;

/// @defgroup Stream and schedule related macros
/// @{


/// @brief Get the ID of the stream. This is to be used with lwb_stream_req#ui8_req_type.
#define GET_STREAM_ID(req_opt)                  (req_opt >> 2)
/// @brief Set the ID of the stream. This is to be used to set lwb_stream_req#ui8_req_type .
#define SET_STREAM_ID(req_opt, id)              (req_opt = (id << 2) | (req_opt & 0x03))

#define SET_STREAM_TYPE(req_opt, type)          (req_opt = ((req_opt >> 2) << 2) | (type & 0x03))

#define GET_STREAM_TYPE(req_opt)                (req_opt & 0x03)

/// @brief Get number of free slots in a schedule. This is to be used with lwb_sched_header#ui8_n_slots.
#define GET_N_FREE_SLOTS(a)                     (a >> 6)
/// @brief Set number of free slots in a schedule. This is to be used to set lwb_sched_header#ui8_n_slots.
#define SET_N_FREE_SLOTS(slots, free_slots)     (slots = (slots & 0x3f)  | (free_slots << 6))
/// @brief Get number of data slots in a schedule. This is to be used with lwb_sched_header#ui8_n_slots.
#define GET_N_DATA_SLOTS(a)                     (a & 0x3f)
/// @brief Set number of data slots in a schedule. This is to be used to set lwb_sched_header#ui8_n_slots.
#define SET_N_DATA_SLOTS(slots, data_slots)     (slots = (slots & 0xc0) | (data_slots & 0x3f))

/// @}

/// @addtogroup UI32 Macros
///           Unsign 32-bit integer lated macros to get/set low/high segments.
/// @{
#define UI32_GET_LOW(ui32_var)             (uint16_t)(ui32_var & 0xffff)
#define UI32_GET_HIGH(ui32_var)            (uint16_t)(ui32_var >> 16)
#define UI32_SET_LOW(ui32_var, ui16_val)   (ui32_var = ((uint32_t)UI32_GET_HIGH(ui32_var) << 16) | (uint32_t)((ui16_val) & 0xffff))
#define UI32_SET_HIGH(ui32_var, ui16_val)  (ui32_var = ((uint32_t)(ui16_val) << 16) | (uint32_t)UI32_GET_LOW(ui32_var))

/// @}

/// @addtogroup glossy macros
/// @{
/// @brief Reference time of Glossy.
#define GLOSSY_T_REF                (get_t_ref_l())
/// @brief Check if glossy's reference time is updated.
#define GLOSSY_IS_SYNCED()          (is_t_ref_l_updated())
/// @brief Set glossy's reference time not updated.
#define GLOSSY_SET_UNSYNCED()       (set_t_ref_l_updated(0))
/// @}

/// @addtogroup General configurations
/// @{
/// @brief Number of Glossy retransmissions for synchronization (sending/receiving schedule)
#define N_SYNC                      3
/// @brief Number of Glossy retransmissions for data slots
#define N_RR                        3
/// @brief Glossy duration for synchronization (sending/receiving schedule) (30 ms)
#define T_SYNC_ON                   (RTIMER_SECOND / 33)            // 30 ms
/// @brief Gap between slots (4 ms).
#define T_GAP                       (RTIMER_SECOND / 250)           // 04 ms
/// @brief Glossy duration for data slots (20 ms)
#if COOJA
#define T_RR_ON                     (RTIMER_SECOND / 25)            // 40 ms
#else
#define T_RR_ON                     (RTIMER_SECOND / 40)            // 25 ms
#endif
/// @brief Glossy duration for free slots
#define T_FREE_ON                   (RTIMER_SECOND / 100)           // 10 ms

/// @brief Guard time 1
#define T_GUARD_1                   (RTIMER_SECOND / 333)           //  03 ms
/// @brief Guard time 2
#define T_GUARD_2                   (RTIMER_SECOND / 200)           //  05 ms
/// @brief Guard time 3
#define T_GUARD_3                   (RTIMER_SECOND /  50)           //  20 ms

#if COOJA
/// @brief Normal guard time
#define T_GUARD                     (RTIMER_SECOND / 250)           //  04 ms
#else
/// @brief Normal guard time
#define T_GUARD                     (RTIMER_SECOND / 2000)          // 500 us
#endif

/// @brief Only one schedule is used by default
#ifndef TWO_SCHEDS
#define TWO_SCHEDS                  0
#endif

#ifndef TIME_SCALE
#define TIME_SCALE                  1
#endif

/// @}

//#define PERIOD_RT(T)     ((T) * (uint32_t)RTIMER_SECOND + ((int32_t)((T) * skew) / (int32_t)64))


#endif // __LWB_COMMON_H__
