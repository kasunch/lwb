
#ifndef LWB_H
#define LWB_H

#include "contiki-conf.h"
#include "glossy.h"
#include "dev/cc2420.h"
#include "node-id.h"
#include "lib/random.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "dev/xmem.h"

#define JOINING_NODES                1
#define PRINT_BOOTSTRAP_MSG          0
#define REMOVE_NODES                 1
#define DYNAMIC_PERIOD               1
#define STATIC                       0
#define COMPRESS                     1
#define DYNAMIC_FREE_SLOTS           1
#define TWO_SCHEDS                   1
#define CONTROL_DC                   1

#ifndef TIME_SCALE
#define TIME_SCALE                   1
#endif /* TIME_SCALE */

/// @brief Synchronization states
enum {
  BOOTSTRAP,      ///< Initial state. Not synchronized
  QUASI_SYNCED,   ///< Received schedule when in BOOTSTRAP state. Clock drift is not estimated.
  SYNCED,         ///< Received schedule and clock drift is estimated.
  ALREADY_SYNCED, ///< @note Kasun: Still no idea why this is
  UNSYNCED_1,     ///< Missed schedule one time.
  UNSYNCED_2,     ///< Missed schedule two times.
  UNSYNCED_3,     ///< Missed schedule three times.
} sync_states;

const uint8_t sync_leds[] = {6, 3, 0, 1, 2, 4, 5};

/*--------------------------------------------------------------------------*/
#if COOJA
#define N_SLOTS_MAX                  4
#else
#if LONG_SLOTS
#define N_SLOTS_MAX                 30
#elif SHORT_SLOTS
#define N_SLOTS_MAX                120
#else
#define N_SLOTS_MAX                 60
#endif /* LONG_SLOTS */
#endif /* COOJA */
#define NODE_ID_BITS                 9
#define NODE_ID_MASK                 ((1 << NODE_ID_BITS) - 1)
#define N_STREAMS_MAX              300
#define N_STREAM_REQS_MAX            4

/*--------------------------------------------------------------------------*/
#if TWO_SCHEDS

#define N_SYNC                      3

#if LONG_SLOTS
#define T_SYNC_ON                   (RTIMER_SECOND / 33)           //  30 ms
#elif SHORT_SLOTS
#define T_SYNC_ON                   (RTIMER_SECOND / 133)          //  7.5 ms
#else
#define T_SYNC_ON                   (RTIMER_SECOND / 66)           //  15 ms
#endif /* LONG_SLOTS */
#else
#define N_SYNC                      5
#define T_SYNC_ON                   (RTIMER_SECOND / 50)           //  20 ms
#endif /* TWO_SCHEDS */
#if COOJA
#define T_GUARD                     (RTIMER_SECOND / 250)          //   4 ms
#else
#define T_GUARD                     (RTIMER_SECOND / 2000)         // 500 us
#endif /* COOJA */
#define T_GUARD_1                   (RTIMER_SECOND / 333)          //   3 ms
#define T_GUARD_2                   (RTIMER_SECOND / 200)          //   5 ms
#define T_GUARD_3                   (RTIMER_SECOND /  50)          //  20 ms
#define SCHEDULE(ref, offset, cb)   rtimer_set(&rt, ref + offset, 1, (rtimer_callback_t)cb, NULL)
#define SCHEDULE_L(ref, offset, cb) rtimer_set_long(&rt, ref, offset, (rtimer_callback_t)cb, NULL)
#define SYNC_LEDS()                 { leds_off(LEDS_ALL); leds_on(sync_leds[sync_state]); }
#define T_REF                       (get_t_ref_l())

/*--------------------------------------------------------------------------*/
typedef struct {
  uint16_t node_id;
  uint16_t dest;
#if STATIC
  uint8_t  relay_cnt;
#endif /* STATIC */
} data_header_struct;
#if STATIC
#define DATA_HEADER_LENGTH 5
#else
#define DATA_HEADER_LENGTH 4
#endif /* STATIC */

typedef struct {
  uint32_t en_on;
  uint32_t en_tot;
#if LATENCY
  uint16_t gen_time;
#endif /* LATENCY */


#if !COOJA
#if LWB_DEBUG
  uint32_t en_control;
  uint8_t  relay_cnt;
#if (!(LATENCY || MOBILE || USERINT_INT))
  uint8_t  foo[2];
#endif /* (!(LATENCY || MOBILE)) */
#else
#if LATENCY || MOBILE
  uint8_t  foo[5];
#elif USERINT_INT
  uint8_t  foo[3];
#else
  uint8_t  foo[7];
#endif /* LATENCY */
#endif /* LWB_DEBUG */
#endif /* !COOJA */
} data_struct;
#if COOJA
#define DATA_LENGTH 10
#else
#define DATA_LENGTH 15
#endif /* COOJA */

typedef struct {
  uint16_t ipi;
  uint16_t time_info;
  uint8_t  req_type;
} stream_req;
#define STREAM_REQ_LENGTH 5

enum {
  ADD_STREAM,
  DEL_STREAM,
  REP_STREAM
} stream_req_types;

enum {
  NO_DATA,
  DATA,
  STREAM_ACK,
  SCHED
} pkt_types;

typedef struct stream_info {
  struct stream_info *next;
  uint16_t node_id;
  uint16_t ipi;
  uint16_t last_assigned;
  uint16_t next_ready;
  uint8_t  stream_id;
#if REMOVE_NODES
  uint8_t  n_cons_missed;
#endif /* REMOVE_NODES */
} stream_info;

/// @brief Structure for schedule information
typedef struct {
  uint16_t host_id;     ///< ID of the host
  uint16_t time;        ///< Duration of the round @note Kasun: Not sure
  uint8_t  n_slots;     ///< Number of slots in the round
  uint8_t  T;           ///< Round period (duration between beginning of two rounds)
  uint8_t  slot[120];
} sched_struct;

#define SYNC_HEADER_LENGTH 6


#if COOJA
#define N_RR                        2
#else
#define N_RR                        3
#endif /* COOJA */
#define T_GAP                       (RTIMER_SECOND / 250)           //  4 ms
#if LONG_SLOTS
#define T_RR_ON                     (RTIMER_SECOND / 50)            // 20 ms
#elif SHORT_SLOTS
#define T_RR_ON                     (RTIMER_SECOND / 200)           //  5 ms
#else
#define T_RR_ON                     (RTIMER_SECOND / 100)           // 10 ms
#endif /* LONG_SLOTS */

/*--------------------------------------------------------------------------*/
#define IS_SYNCED()                 (is_t_ref_l_updated())
#define SET_UNSYNCED()              (set_t_ref_l_updated(0))
#define IS_DATA()

PT_THREAD(g_sync(struct rtimer *t, void *ptr));
PT_THREAD(g_rr_host(struct rtimer *t, void *ptr));
PT_THREAD(g_rr_source(struct rtimer *t, void *ptr));
PROCESS_NAME(print);
PROCESS_NAME(scheduler);

#if JOINING_NODES
enum {
  NOT_JOINED,
  JOINING,
  JOINED,
  JUST_TRIED
} bs_state;
#define T_FREE_ON                    (RTIMER_SECOND / 100)           // 10 ms
#if DYNAMIC_FREE_SLOTS
#define T_LAST_FREE_REQS            (60 * TIME_SCALE)
#define T_LAST_FREE_SLOT            (60 * TIME_SCALE)
#endif /* DYNAMIC_FREE_SLOTS */
#endif /* JOINING_NODES */

#if PRINT_BOOTSTRAP_MSG
#define T_BOOTSTRAP_GAP (RTIMER_SECOND / 2 + (random_rand() % (RTIMER_SECOND / 1000))) // [10,11] ms
#endif /* PRINT_BOOTSTRAP_MSG */

#if REMOVE_NODES
#define N_CONS_MISSED_MAX          10
#endif /* REMOVE_NODES */


#ifndef AVG_RELAY_CNT_UPDATE
#define AVG_RELAY_CNT_UPDATE         4
#endif /* AVG_RELAY_CNT_UPDATE */
#define RELAY_CNT_INVALID         0xff
#ifndef RELAY_CNT_FACTOR
#define RELAY_CNT_FACTOR            16
#endif /* RELAY_CNT_FACTOR */

#endif /* LWB_H */
