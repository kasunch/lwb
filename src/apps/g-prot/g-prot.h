
#ifndef G_PROT_H
#define G_PROT_H

#include "contiki-conf.h"
#include "glossy.h"
#include "node-id.h"
#include "data_gen.h"

enum {
	SYNCED,
	UNSYNCED_1,
	SYNCED_1,
	UNSYNCED_2,
	UNSYNCED_3,
	BOOTSTRAP
} sync_states;

/*--------------------------------------------------------------------------*/
#define HOST_NODE_ID                200
#if COOJA
#define N_NODES_MAX                 4
#else
#define N_NODES_MAX                 45
#endif /* COOJA */
#define PERIOD                      ((uint32_t)RTIMER_SECOND * 3)  //   3 s

#define BEACONS_UPDATE              16
#define RELAY_CNT_UPDATE            10

/*--------------------------------------------------------------------------*/
typedef struct {
	uint32_t seq_no;
	uint8_t n_nodes;
	uint8_t schedule[N_NODES_MAX];
} beacon_struct;

#define N_SYNC                      3
#define T_SYNC_ON                   (RTIMER_SECOND / 33)           //  30 ms
#if COOJA
#define T_GUARD                     (RTIMER_SECOND / 500)          //   2 ms
#else
#define T_GUARD                     (RTIMER_SECOND / 2000)         // 500 us
#endif /* COOJA */
#define T_GUARD_1                   (RTIMER_SECOND / 500)          //   2 ms
#define T_GUARD_2                   (RTIMER_SECOND / 200)          //   5 ms
#define T_GUARD_3                   (RTIMER_SECOND /  50)          //  20 ms
#define SCHEDULE(ref, offset, cb)   rtimer_set_long(&rt, ref, offset, (rtimer_callback_t)cb, NULL)
#define SYNC_LEDS(leds)             { leds_off(LEDS_ALL); leds_on(leds); }
#define BEACON_LEN                  sizeof(beacon_struct)
#define T_REF                       (get_t_ref_l())

/*--------------------------------------------------------------------------*/
typedef struct {
	uint16_t Pb;
	uint16_t Dc;
	uint16_t Dc_tot;
#if STATIC
	uint8_t relay_cnt;
#endif /* STATIC */
#if !COOJA
	uint8_t foo[21];
#endif /* COOJA */
} data_struct;

#define N_RR                        1
#define T_GAP                       (RTIMER_SECOND / 100)           // 10 ms
#define T_RR_ON                     (RTIMER_SECOND / 100)          //  10 ms
#define DATA_LEN                    sizeof(data_struct)

/*--------------------------------------------------------------------------*/
#define IS_HOST()                   (node_id == HOST_NODE_ID)
#define IS_SYNCED()                 (is_t_ref_l_updated())

char g_sync(struct rtimer *t, void *ptr);
char g_rr(struct rtimer *t, void *ptr);

#endif /* G_PROT_H */
