
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

enum {
	BOOTSTRAP,
	QUASI_SYNCED,
	SYNCED,
	ALREADY_SYNCED,
	UNSYNCED_1,
	UNSYNCED_2,
	UNSYNCED_3,
} sync_states;
const uint8_t sync_leds[] = {6, 3, 0, 1, 2, 4, 5};

/*--------------------------------------------------------------------------*/
#if COOJA
#define N_SLOTS_MAX                  3
#else
#if LONG_SLOTS
#define N_SLOTS_MAX                 30
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
#if KANSEI_SCALABILITY && (INIT_IPI == 5)
#define N_SYNC                      5
#else
#define N_SYNC                      3
#endif /* KANSEI_SCALABILITY && (INIT_IPI == 5) */
#if LONG_SLOTS
#define T_SYNC_ON                   (RTIMER_SECOND / 33)           //  30 ms
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
#if MOBILE
	uint16_t n_rcvd_tot;
#endif /* MOBILE */
#if !COOJA
#if LWB_DEBUG
	uint32_t en_control;
	uint8_t  relay_cnt;
#if (!(LATENCY || MOBILE))
	uint8_t  foo[2];
#endif /* (!(LATENCY || MOBILE)) */
#else
#if LATENCY || MOBILE
	uint8_t  foo[5];
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

typedef struct {
#if COMPRESS
	uint16_t host_id;
#endif /* COMPRESS */
	uint16_t time;
	uint8_t  n_slots;
	uint8_t  T;
#if COMPRESS
	uint8_t  slot[120];
#else
	uint8_t  slot[(N_SLOTS_MAX * NODE_ID_BITS) / 8 + 1];
#endif /* COMPRESS */
} sched_struct;
#if COMPRESS
#define SYNC_HEADER_LENGTH 6
#else
#define SYNC_HEADER_LENGTH 4
#endif /* COMPRESS */

#if COOJA
#define N_RR                        2
#else
#define N_RR                        3
#endif /* COOJA */
#define T_GAP                       (RTIMER_SECOND / 250)           //  4 ms
#if LONG_SLOTS
#define T_RR_ON                     (RTIMER_SECOND / 50)            // 20 ms
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

#if USERINT_INT
#define IS_USERINT_SET() (P2IN & BV(7))
#define ENABLE_USERINT_INT() do { \
	P2SEL &= ~BV(7); \
	P2DIR &= ~BV(7); \
	P2IES |= BV(7); \
	P2IFG = 0; \
	P2IE |= BV(7); \
} while (0)
#endif /* USERINT_INT */

#if FLASH
#define FLASH_OFFSET (1 * XMEM_ERASE_UNIT_SIZE)
#define FLASH_SIZE   (14 * XMEM_ERASE_UNIT_SIZE)
typedef struct {
	uint16_t node_id;
	uint16_t time;
	uint16_t n_rcvd;
	uint16_t n_tot;
} flash_struct_dy;
typedef struct {
	uint16_t time;
	uint32_t en_on;
	uint32_t en_tot;
} flash_struct_dc;
#endif /* FLASH */

#ifndef AVG_RELAY_CNT_UPDATE
#define AVG_RELAY_CNT_UPDATE         4
#endif /* AVG_RELAY_CNT_UPDATE */
#define RELAY_CNT_INVALID         0xff
#ifndef RELAY_CNT_FACTOR
#define RELAY_CNT_FACTOR            16
#endif /* RELAY_CNT_FACTOR */

#endif /* LWB_H */
