#ifndef __LWB_CONF_DEFAULT_H__
#define __LWB_CONF_DEFAULT_H__

/// @file  lwb-conf-default.h
/// @brief Default configuration file

/// @defgroup Configurations
/// @{

/// @brief Maximum size of the TX RX buffer. Node Glossy use 2 bytes for relay counter and header.
#ifdef LWB_CONF_MAX_TXRX_BUF_LEN
#define LWB_MAX_TXRX_BUF_LEN        LWB_CONF_MAX_TXRX_BUF_LEN
#else
#define LWB_MAX_TXRX_BUF_LEN        125
#endif

/// @brief Maximum number of data buffer elements
#ifdef LWB_CONF_MAX_DATA_BUF_ELEMENTS
#define LWB_MAX_DATA_BUF_ELEMENTS   LWB_CONF_MAX_DATA_BUF_ELEMENTS
#else
#define LWB_MAX_DATA_BUF_ELEMENTS   5
#endif

/// @brief Maximum number of stream request elements
#ifdef LWB_CONF_MAX_STREAM_REQ_ELEMENTS
#define LWB_MAX_STREAM_REQ_ELEMENTS LWB_CONF_MAX_STREAM_REQ_ELEMENTS
#else
#define LWB_MAX_STREAM_REQ_ELEMENTS 5
#endif


/// @brief Maximum number of slots in a round
#ifdef LWB_CONF_SCHED_MAX_SLOTS
#define LWB_SCHED_MAX_SLOTS         LWB_CONF_SCHED_MAX_SLOTS
#else
#define LWB_SCHED_MAX_SLOTS         40
#endif

/// @brief Enable radio duty cycle calculation for control data
#if LWB_CONF_CONTROL_DC
#define LWB_CONTROL_DC              LWB_CONF_CONTROL_DC
#else
#define LWB_CONTROL_DC              0
#endif

/// @brief Number of Glossy retransmissions for synchronization (sending/receiving schedule)
#ifdef LWB_CONF_N_SYNC
#define N_SYNC                      LWB_CONF_N_SYNC
#else
#define N_SYNC                      3
#endif

/// @brief Number of Glossy retransmissions for data slots
#ifdef LWB_CONF_N_RR
#define N_RR                        LWB_CONF_N_RR
#else
#define N_RR                        3
#endif

/// @brief Glossy duration for synchronization (sending/receiving schedule) (30 ms)
#ifdef LWB_CONF_T_SYNC_ON
#define T_SYNC_ON                   LWB_CONF_T_SYNC_ON
#else
#define T_SYNC_ON                   (RTIMER_SECOND / 33)            // 30 ms
#endif

/// @brief Gap between slots.
#ifdef LWB_CONF_T_GAP
#define T_GAP                       LWB_CONF_T_GAP
#else
#define T_GAP                       (RTIMER_SECOND / 25)           // 40 ms
#endif

/// @brief The gap between schedule and first data slots
#ifdef LWB_CONF_T_S_R_GAP
#define T_S_R_GAP                   LWB_CONF_T_S_R_GAP
#else
#define T_S_R_GAP                   (1 * T_GAP)
#endif

/// @brief Time duration to exchange application data packets between external device
#ifdef LWB_CONF_T_HSLP_APP_DATA
#define T_HSLP_APP_DATA             LWB_CONF_T_HSLP_APP_DATA
#else
#define T_HSLP_APP_DATA             (RTIMER_SECOND / 33)           // 30 ms
#endif

/// @brief Time duration to get new schedule from the external device
#ifdef LWB_CONF_T_HSLP_SCHED
#define T_HSLP_SCHED                LWB_CONF_T_HSLP_SCHED
#else
#define T_HSLP_SCHED                (RTIMER_SECOND / 25)            // 40 ms
#endif

/*
#if COOJA
#define T_RR_ON                     (RTIMER_SECOND / 25)            // 40 ms
#else
#define T_RR_ON                     (RTIMER_SECOND / 33)            // 30 ms
#endif
*/

/// @brief Glossy duration for data slots
#ifdef LWB_CONF_T_RR_ON
#define T_RR_ON                     LWB_CONF_T_RR_ON
#else
#define T_RR_ON                     (RTIMER_SECOND / 33)            // 30 ms
#endif

/// @brief Glossy duration for free slots
#ifdef LWB_CONF_T_FREE_ON
#define T_FREE_ON                   LWB_CONF_T_FREE_ON
#else
#define T_FREE_ON                   (RTIMER_SECOND / 100)           // 10 ms
#endif

/// @brief Guard time 1
#ifdef LWB_CONF_T_GUARD_1
#define T_GUARD_1                   LWB_CONF_T_GUARD_1
#else
#define T_GUARD_1                   (RTIMER_SECOND / 333)           //  03 ms
#endif

/// @brief Guard time 2
#ifdef LWB_CONF_T_GUARD_2
#define T_GUARD_2                   LWB_CONF_T_GUARD_2
#else
#define T_GUARD_2                   (RTIMER_SECOND / 200)           //  05 ms
#endif

/// @brief Guard time 3
#ifdef LWB_CONF_T_GUARD_3
#define T_GUARD_3                   LWB_CONF_T_GUARD_3
#else
#define T_GUARD_3                   (RTIMER_SECOND /  50)           //  20 ms
#endif

/*
#if COOJA
#define T_GUARD                     (RTIMER_SECOND / 250)           //  04 ms
#else
#define T_GUARD                     (RTIMER_SECOND / 2000)          // 500 us
#endif
*/

/// @brief Normal guard time
#ifdef LWB_CONF_T_GUARD
#define T_GUARD                     LWB_CONF_T_GUARD
#else
#define T_GUARD                     (RTIMER_SECOND / 2000)          // 500 us
#endif


/// @}

#endif // __LWB_CONF_DEFAULT_H__
