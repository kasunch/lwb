
#ifndef SYNC_H
#define SYNC_H

#include "contiki-conf.h"
#include "glossy.h"
#include "node-id.h"

enum {
	SYNCED,
	UNSYNCED_1,
	SYNCED_1,
	UNSYNCED_2,
	UNSYNCED_3,
	BOOTSTRAP
};

#define INITIATOR_NODE_ID       200
#define N_TX                    5
#define SYNC_PERIOD             ((uint32_t)RTIMER_SECOND * 10) //  10 s
#define SYNC_ON                 (RTIMER_SECOND / 20)           //  60 ms
#if COOJA
#define T_GUARD                 (RTIMER_SECOND / 500)          //   2 ms
#else
#define T_GUARD                 (RTIMER_SECOND / 2000)         // 500 us
#endif /* COOJA */
#define T_GUARD_1               (RTIMER_SECOND / 500)          //   2 ms
#define T_GUARD_2               (RTIMER_SECOND / 200)          //   5 ms
#define T_GUARD_3               (RTIMER_SECOND /  20)          //  50 ms

#if COOJA
#define N_NODES                 0
#else
#define N_NODES                 50
#endif /* COOJA */

#define BEACONS_UPDATE          10
#define ENERGEST_UPDATE         10

typedef struct {
	unsigned long seq_no;
	uint8_t schedule[N_NODES];
	uint8_t relay_cnt[N_NODES];
} sync_data_struct;

#define DATA_LEN                    sizeof(sync_data_struct)
#define IS_INITIATOR()              (node_id == INITIATOR_NODE_ID)
#define GLOSSY_IS_BOOTSTRAPPING()   (skew_estimated < GLOSSY_BOOTSTRAP_PERIODS)
#define GLOSSY_IS_SYNCED()          (is_t_ref_l_updated())
#define T_REF                       (get_t_ref_l())

#define SCHEDULE_SYNC(ref, offset)  rtimer_set_long(t, ref, offset, (rtimer_callback_t)sync, ptr)
#define SYNC_LEDS(leds)             { leds_off(LEDS_ALL); leds_on(leds); }

#endif /* SYNC_H */
