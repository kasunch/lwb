
#include "g-prot.h"

#if COOJA
	const uint8_t schedule[] = {2, 201, 202, 203};
#else
//	static const uint8_t schedule[] = {101, 102, 103, 104, 105, 106, 107, 108, 109};
	static const uint8_t schedule[] = {
			 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17,
			18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
			101, 102, 103, 104, 105, 106, 107, 108, 109};
#endif /* COOJA */

static struct rtimer rt;
static struct pt pt_sync, pt_rr;
static beacon_struct beacon;
static data_struct data, tmp;
static rtimer_clock_t t_ref_l_old = 0;
static rtimer_clock_t t_start = 0;
static int16_t  period_skew = 0;

static uint32_t last_seq_no = 0;
static uint8_t  beacons_total = 0;
static uint32_t beacons_rcvd = 0;
static uint32_t last_radio_on = 0;
static uint32_t last_energest = 0;
static uint32_t energest_total = 0;
static uint64_t radio_on = 0;

static uint16_t Pb, Dc, Dc_tot;
static uint8_t  sync_state;
static uint8_t  rr_idx;
#if STATIC
static uint8_t  min_relay_cnt = 0xff;
static uint8_t  min_relay_cnt_tmp = 0xff;
static uint8_t  relay_cnt[N_NODES_MAX];
#endif /* STATIC */

static uint8_t  rx_cnt[N_NODES_MAX];
static uint16_t Pb_nodes[N_NODES_MAX];
static uint16_t Dc_nodes[N_NODES_MAX];
static uint16_t Dc_nodes_tot[N_NODES_MAX];
static uint32_t pkt_tot, pkt_rcvd = 0;

static rtimer_clock_t T_guard;
static rtimer_clock_t T_delay = 0;

PROCESS(print, "Print");
PROCESS_THREAD(print, ev, data)
{
	PROCESS_BEGIN();

	static uint8_t sum_rx_cnt;

	if (IS_HOST()) {
		while(1) {
			PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

			printf("\n\n\n");
			sum_rx_cnt = 0;
			uint8_t ffs = 0;
			for (rr_idx = 0; rr_idx < beacon.n_nodes; rr_idx++) {
				printf("%6lu  ", beacon.seq_no);
				printf("%2u  ", beacon.n_nodes);
				printf("%3u ", beacon.schedule[rr_idx]);
#if STATIC
				printf("%3u ", relay_cnt[rr_idx]);
#endif /* STATIC */
				printf("%5u ", Pb_nodes[rr_idx]);
				printf("%4u ", Dc_nodes[rr_idx]);
				printf("%4u ", Dc_nodes_tot[rr_idx]);
				printf("%1u ", !!rx_cnt[rr_idx]);
				sum_rx_cnt += !!rx_cnt[rr_idx];
				printf("%2u ", sum_rx_cnt);
				if (Pb_nodes[rr_idx] == 0) {
					ffs++;
					pkt_rcvd = 0;
					pkt_tot = 0;
				}
				if (!ffs) {
					pkt_rcvd += !! rx_cnt[rr_idx];
					pkt_tot++;
				}
				uint32_t rel = pkt_tot ? (10000LU * pkt_rcvd) / pkt_tot : 0;
				printf("%5u\n", (uint16_t)rel);
			}
		}
	} else {
		while(1) {
			PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
			printf("%6lu ", beacon.seq_no);
			printf("%2u ", get_relay_cnt());
#if STATIC
//			printf("%2u ", relay_cnt);
			printf("%2u ", min_relay_cnt);
#endif /* STATIC */
			printf("%5u ", T_delay);
			printf("%5u ", Pb);
			printf("%4u\n", Dc);
		}
	}

	PROCESS_END();
}

static inline void estimate_period_skew(void) {
	if (sync_state == UNSYNCED_1) {
		period_skew = T_REF - (t_ref_l_old + 2 * (rtimer_clock_t)PERIOD);
	} else if (sync_state == UNSYNCED_2) {
		period_skew = T_REF - (t_ref_l_old + 3 * (rtimer_clock_t)PERIOD);
	} else if (sync_state == UNSYNCED_3) {
		period_skew = T_REF - (t_ref_l_old + 4 * (rtimer_clock_t)PERIOD);
	} else {
		period_skew = T_REF - (t_ref_l_old + (rtimer_clock_t)PERIOD);
	}
}

#if STATIC
static inline void update_relay_cnt(void) {
	uint8_t curr_relay_cnt = get_relay_cnt();

	if ((beacon.seq_no % BEACONS_UPDATE) == 0) {
		T_delay = 0;
	} else if ((beacon.seq_no % BEACONS_UPDATE) == 1) {
		if (sync_state == BOOTSTRAP) {
			T_delay = 0;
		} else {
			T_delay = min_relay_cnt_tmp * (get_T_slot_h() / CLOCK_PHI);
			min_relay_cnt = min_relay_cnt_tmp;
			min_relay_cnt_tmp++;
		}
	}
	if (curr_relay_cnt < min_relay_cnt_tmp) {
		min_relay_cnt_tmp = curr_relay_cnt;
	}
}
#endif /* STATIC */

static inline void update_Pb_Dc(void) {
	if (last_seq_no != 0) {
		beacons_total += (beacon.seq_no - last_seq_no);
		beacons_rcvd += 10000;
		uint8_t add_one = 0;
		energest_total += (energest_type_time(ENERGEST_TYPE_CPU) +
				energest_type_time(ENERGEST_TYPE_LPM) - last_energest);
		radio_on += 10000 *
				(energest_type_time(ENERGEST_TYPE_LISTEN) +
				energest_type_time(ENERGEST_TYPE_TRANSMIT) - last_radio_on);
		Pb = (uint16_t)(beacons_rcvd / beacons_total);
		Dc = (uint16_t)(radio_on / energest_total);
		Dc_tot = (uint16_t)((10000LLU *
				(energest_type_time(ENERGEST_TYPE_LISTEN) +
				energest_type_time(ENERGEST_TYPE_TRANSMIT))) /
				(energest_type_time(ENERGEST_TYPE_CPU) +
				energest_type_time(ENERGEST_TYPE_LPM)));

		if (beacons_total >= BEACONS_UPDATE) {
			beacons_total /= 2;
			if (beacons_rcvd & 1) {
				add_one = 1;
			}
			beacons_rcvd = beacons_rcvd / 2 + add_one;
			energest_total /= 2;
			radio_on /= 2;
		}
	}
	last_seq_no = beacon.seq_no;
	last_energest = energest_type_time(ENERGEST_TYPE_CPU) +
			energest_type_time(ENERGEST_TYPE_LPM);
	last_radio_on = energest_type_time(ENERGEST_TYPE_LISTEN) +
			energest_type_time(ENERGEST_TYPE_TRANSMIT);
}

char g_sync(struct rtimer *t, void *ptr) {
	PT_BEGIN(&pt_sync);

	if (IS_HOST()) {	// host
		beacon.seq_no = 0;
		beacon.n_nodes = sizeof(schedule);
		memcpy(&beacon.schedule, &schedule, beacon.n_nodes);
		while (1) {
			leds_on(LEDS_GREEN);
			beacon.seq_no++;
			glossy_start((uint8_t *)&beacon, BEACON_LEN, GLOSSY_INITIATOR, GLOSSY_SYNC, N_SYNC);
			t_start = RTIMER_TIME(t);
			SCHEDULE(t_start, T_SYNC_ON, g_sync);
			PT_YIELD(&pt_sync);

			leds_off(LEDS_GREEN);
			glossy_stop();
			if (!IS_SYNCED()) {
				set_t_ref_l(T_REF + PERIOD);
			}
			g_rr(&rt, NULL);
			PT_YIELD(&pt_sync);
		}
	} else {	// source node
		sync_state = BOOTSTRAP;
		SYNC_LEDS(sync_state + 1);
		while (1) {
			if (sync_state == BOOTSTRAP) {
				glossy_start((uint8_t *)&beacon, BEACON_LEN, GLOSSY_RECEIVER, GLOSSY_SYNC, N_SYNC);
				SCHEDULE(RTIMER_TIME(t), T_SYNC_ON, g_sync);
				PT_YIELD(&pt_sync);
				while (!IS_SYNCED()) {
					glossy_reset();
					SCHEDULE(RTIMER_TIME(t), T_SYNC_ON, g_sync);
					PT_YIELD(&pt_sync);
				};

			} else {
				glossy_start((uint8_t *)&beacon, BEACON_LEN, GLOSSY_RECEIVER, GLOSSY_SYNC, N_SYNC);
				SCHEDULE(RTIMER_TIME(t), T_SYNC_ON + T_guard - T_delay, g_sync);
				PT_YIELD(&pt_sync);
			}

			glossy_stop();
			if (IS_SYNCED()) {
#if STATIC
				update_relay_cnt();
#endif /* STATIC */
				if (sync_state == BOOTSTRAP) {
					period_skew = 0;
					sync_state = SYNCED_1;
					T_guard = T_GUARD_3;
				} else {
					estimate_period_skew();
					sync_state = SYNCED;
					T_guard = T_GUARD;
				}
				t_ref_l_old = T_REF;
				SYNC_LEDS(sync_state + 1);
				g_rr(&rt, NULL);
				PT_YIELD(&pt_sync);
			} else {
				set_t_ref_l(T_REF + PERIOD + period_skew);
#if STATIC
				T_delay = 0;
#endif /* STATIC */
				if ((sync_state == UNSYNCED_3) || (sync_state == SYNCED_1)) {
					sync_state = BOOTSTRAP;
					SYNC_LEDS(sync_state + 1);
				} else {
					if (sync_state == SYNCED) {
						sync_state = UNSYNCED_1;
						T_guard = T_GUARD_1;
					} else if (sync_state == UNSYNCED_1) {
						sync_state = UNSYNCED_2;
						T_guard = T_GUARD_2;
					} else if (sync_state == UNSYNCED_2) {
						sync_state = UNSYNCED_3;
						T_guard = T_GUARD_3;
					}
					SCHEDULE(T_REF, PERIOD + period_skew - T_guard, g_sync);
					SYNC_LEDS(sync_state + 1);
					PT_YIELD(&pt_sync);
				}
			}
		}
	}

	PT_END(&pt_sync);
}

char g_rr(struct rtimer *t, void *ptr) {
	PT_BEGIN(&pt_rr);

	if (IS_HOST()) {	// host
		while (1) {
			for (rr_idx = 0; rr_idx < beacon.n_nodes; rr_idx++) {
				leds_off(LEDS_ALL);
				SCHEDULE(T_REF, T_SYNC_ON + T_GAP + rr_idx * (T_RR_ON + T_GAP), g_rr);
				PT_YIELD(&pt_rr);

				leds_on(rr_idx + 1);
				glossy_start((uint8_t *)&data, DATA_LEN, GLOSSY_RECEIVER, GLOSSY_NO_SYNC, N_RR);
				SCHEDULE(T_REF, T_SYNC_ON + (rr_idx + 1) * (T_RR_ON + T_GAP), g_rr);
				PT_YIELD(&pt_rr);

				rx_cnt[rr_idx] = glossy_stop();
				if (rx_cnt[rr_idx]) {
#if STATIC
					relay_cnt[rr_idx] = data.relay_cnt;
#endif /* STATIC */
					Pb_nodes[rr_idx] = data.Pb;
					Dc_nodes[rr_idx] = data.Dc;
					Dc_nodes_tot[rr_idx] = data.Dc_tot;
				}
			}
			leds_off(LEDS_ALL);
			SCHEDULE(t_start, PERIOD, g_sync);
			process_poll(&print);
			PT_YIELD(&pt_rr);
		}
	} else {	// source node
		while (1) {
			update_Pb_Dc();
			for (rr_idx = 0; rr_idx < beacon.n_nodes; rr_idx++) {
				leds_off(LEDS_ALL);
				SCHEDULE(T_REF, T_SYNC_ON + T_GAP + rr_idx * (T_RR_ON + T_GAP), g_rr);
				PT_YIELD(&pt_rr);

				leds_on(rr_idx + 1);
				if (beacon.schedule[rr_idx] == node_id) {
#if STATIC
					data.relay_cnt = min_relay_cnt;
#endif /* STATIC */
					data.Pb = Pb;
					data.Dc = Dc;
					data.Dc_tot = Dc_tot;
					glossy_start((uint8_t *)&data, DATA_LEN, GLOSSY_INITIATOR, GLOSSY_NO_SYNC, N_RR);
					SCHEDULE(T_REF, T_SYNC_ON + (rr_idx + 1) * (T_RR_ON + T_GAP), g_rr);
					PT_YIELD(&pt_rr);

					glossy_stop();
				} else {
#if STATIC
					if ((relay_cnt[rr_idx] >= min_relay_cnt) || (relay_cnt[rr_idx] < 0xff) ||
							((beacon.seq_no % BEACONS_UPDATE) == 0)) {
						if ((relay_cnt[rr_idx] > min_relay_cnt + 1) && (relay_cnt[rr_idx] < 0xff)) {
							rtimer_clock_t now = RTIMER_NOW();
							while (RTIMER_CLOCK_LT(RTIMER_NOW(), now +
									(relay_cnt[rr_idx] - 1 - min_relay_cnt) *
									((rtimer_clock_t)(DATA_LEN + FOOTER_LEN + GLOSSY_HEADER_LEN) *
									35 + 400) * 4 / CLOCK_PHI));
						}
#endif /* STATIC */
						glossy_start((uint8_t *)&tmp, DATA_LEN, GLOSSY_RECEIVER, GLOSSY_NO_SYNC, N_RR);
						SCHEDULE(T_REF, T_SYNC_ON + (rr_idx + 1) * (T_RR_ON + T_GAP), g_rr);
						PT_YIELD(&pt_rr);

						rx_cnt[rr_idx] = glossy_stop();
						if (rx_cnt[rr_idx]) {
#if STATIC
							relay_cnt[rr_idx] = tmp.relay_cnt;
#endif /* STATIC */
						}
#if STATIC
					}
#endif /* STATIC */
				}
			}
			SCHEDULE(T_REF, PERIOD + period_skew + T_delay - T_guard, g_sync);
			SYNC_LEDS(sync_state + 1);
			process_poll(&print);
			PT_YIELD(&pt_rr);
		}
	}

	PT_END(&pt_rr);
}

PROCESS(g_prot_init, "G-Prot init");
AUTOSTART_PROCESSES(&g_prot_init);
PROCESS_THREAD(g_prot_init, ev, data)
{
	PROCESS_BEGIN();

	if (!IS_HOST()) {
		data_gen_init();
	}
	process_start(&print, NULL);
	process_start(&glossy_process, NULL);
	SCHEDULE(RTIMER_NOW(), RTIMER_SECOND, g_sync);

	PROCESS_END();
}
