
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

#define RTX          1
#define BUFFER_SIZE  64

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
#define N_SLOTS_MAX                  5
#else
#define N_SLOTS_MAX                 45
#endif /* COOJA */
#define N_STREAMS_MAX              120
#define N_STREAM_REQS_MAX            4
#define N_SINKS_MAX                 30

/*--------------------------------------------------------------------------*/
#define N_SYNC                      3
#define T_SYNC_ON                   (RTIMER_SECOND / 50)           //  20 ms
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
	uint16_t gen_time;
	uint16_t dest;
	uint16_t ps;
	uint16_t Ts;
	uint16_t Td;
} data_header_struct;
#define DATA_HEADER_LENGTH 12

typedef struct {
	uint16_t seq_no;
#if !COOJA
	uint32_t en_on;
	uint32_t en_tot;
	uint8_t  foo[5];
#endif /* !COOJA */
} data_struct;
#if COOJA
#define DATA_LENGTH 2
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
	REP_STREAM,
	DATA_SINK
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
	uint8_t  n_cons_missed;
} stream_info;

typedef struct slots_info {
	struct slots_info *next;
	stream_info *stream;
	uint16_t gen_time;
	uint8_t  rtx;
} slots_info;

typedef struct {
	uint16_t host_id;
	uint16_t time;
	uint8_t  seq_no;
	uint8_t  n_slots;
	uint8_t  T;
	uint8_t  slot[120];
} sched_struct;
#define SYNC_HEADER_LENGTH 7

typedef struct pkt_info {
	uint16_t node_id;
	uint16_t gen_time;
	uint8_t  stream_id;
	data_struct data;
} pkt_info;

typedef struct rcvd_pkt_info {
	struct rcvd_pkt_info *next;
	pkt_info pkt_info;
} rcvd_pkt_info;

typedef struct sinks_info {
	struct sinks_info *next;
	uint16_t node_id;
	uint64_t ack;
	uint8_t  n_acks_missed;
} sinks_info;

#if COOJA
#define N_RR                        2
#else
#define N_RR                        3
#endif /* COOJA */
#define T_GAP                       (RTIMER_SECOND / 250)           //  4 ms
#define T_RR_ON                     (RTIMER_SECOND / 100)           // 10 ms

/*--------------------------------------------------------------------------*/
#define IS_SYNCED()                 (is_t_ref_l_updated())
#define SET_UNSYNCED()              (set_t_ref_l_updated(0))
#define IS_DATA()

PT_THREAD(g_sync(struct rtimer *t, void *ptr));
PT_THREAD(g_rr_host(struct rtimer *t, void *ptr));
PT_THREAD(g_rr_source(struct rtimer *t, void *ptr));
PROCESS_NAME(print);
PROCESS_NAME(scheduler);

enum {
	NOT_JOINED,
	JOINING,
	JOINED,
	JUST_TRIED
} bs_state;
#define T_FREE_ON                    (RTIMER_SECOND / 100)           // 10 ms
#define T_LAST_FREE_REQS            (60 * TIME_SCALE)
#define T_LAST_FREE_SLOT            (60 * TIME_SCALE)

#define N_CONS_MISSED_MAX          50

#ifndef AVG_RELAY_CNT_UPDATE
#define AVG_RELAY_CNT_UPDATE         4
#endif /* AVG_RELAY_CNT_UPDATE */
#define RELAY_CNT_INVALID         0xff
#ifndef RELAY_CNT_FACTOR
#define RELAY_CNT_FACTOR            16
#endif /* RELAY_CNT_FACTOR */

#define SCHED_THR 128
#define DATA_THR  128

#endif /* LWB_H */
