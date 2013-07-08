
#include "sync.h"

static sync_data_struct sync_data;
static struct rtimer rt;
static struct pt pt;
static rtimer_clock_t t_ref_l_old = 0;
static uint8_t skew_estimated = 0;
static uint8_t sync_missed = 0;
static rtimer_clock_t t_start = 0;
static int period_skew = 0;


static unsigned long last_seq_no = 0;
static uint8_t beacons_total = 0;
static unsigned long beacons_rcvd = 0;
static unsigned long last_radio_on = 0;
static unsigned long last_energest = 0;
static unsigned long energest_total = 0;
static unsigned long long radio_on = 0;

static uint8_t sync_state;
static uint8_t sync_rx_cnt;
static rtimer_clock_t T_guard;

PROCESS(sync_print, "Sync print");
PROCESS_THREAD(sync_print, ev, data)
{
	PROCESS_BEGIN();

	while(1) {
		PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
//		printf("%u, skew %d\t", sync_state, period_skew);
//		printf("first_seq_no %lu sync_rcvd %lu\n", first_seq_no, sync_rcvd);
//		printf("%6lu\t", sync_data.seq_no);
//		printf("%6lu\t", sync_data.seq_no - first_seq_no + 1);
//		unsigned long Pb = 10000 * sync_rcvd / (sync_data.seq_no - first_seq_no + 1);
//		printf("%3lu.%02lu\t", Pb / 100, Pb % 100);
//		unsigned long Dc = (10000 *
//				((energest_type_time(ENERGEST_TYPE_LISTEN) +
//						energest_type_time(ENERGEST_TYPE_TRANSMIT)) / (RTIMER_SECOND / 4))) /
//				((energest_type_time(ENERGEST_TYPE_CPU) +
//						energest_type_time(ENERGEST_TYPE_LPM)) / (RTIMER_SECOND / 4));
		if (last_seq_no != 0) {
			beacons_total += (sync_data.seq_no - last_seq_no);
			beacons_rcvd += 10000;
			uint8_t add_one = 0;
			energest_total += (energest_type_time(ENERGEST_TYPE_CPU) +
					energest_type_time(ENERGEST_TYPE_LPM) - last_energest);
			radio_on += 10000 *
					(energest_type_time(ENERGEST_TYPE_LISTEN) +
					energest_type_time(ENERGEST_TYPE_TRANSMIT) - last_radio_on);

			if (beacons_total >= BEACONS_UPDATE) {
				beacons_total /= 2;
				if (beacons_rcvd & 1) {
					add_one = 1;
				}
				beacons_rcvd = beacons_rcvd / 2 + add_one;
			}
			if (energest_total >= ENERGEST_UPDATE) {
				energest_total /= 2;
				radio_on /= 2;
			}

			printf("%6lu\t", sync_data.seq_no);
			printf("%5u\t", (uint16_t)(beacons_rcvd / beacons_total));
			printf("%5u\n", (uint16_t)(radio_on / energest_total));
//			printf("%lu %lu\n", (unsigned long)(10000 *
//					(energest_type_time(ENERGEST_TYPE_LISTEN) +
//					energest_type_time(ENERGEST_TYPE_TRANSMIT) - last_radio_on)), (energest_type_time(ENERGEST_TYPE_CPU) +
//							energest_type_time(ENERGEST_TYPE_LPM) - last_energest));
//			printf("%8lu\t", energest_type_time(ENERGEST_TYPE_LISTEN));
//			printf("%8lu\t", energest_type_time(ENERGEST_TYPE_TRANSMIT));
//			printf("%8lu\t", energest_type_time(ENERGEST_TYPE_CPU));
//			printf("%8lu\n", energest_type_time(ENERGEST_TYPE_LPM));
//			printf("%3lu.%02lu\n", Dc / 100, Dc % 100);
		}
		last_seq_no = sync_data.seq_no;
		last_energest = energest_type_time(ENERGEST_TYPE_CPU) +
				energest_type_time(ENERGEST_TYPE_LPM);
		last_radio_on = energest_type_time(ENERGEST_TYPE_LISTEN) +
				energest_type_time(ENERGEST_TYPE_TRANSMIT);
	}

	PROCESS_END();
}

//
static inline void estimate_period_skew(void) {
	if (sync_state == UNSYNCED_1) {
		period_skew = T_REF - (t_ref_l_old + 2 * (rtimer_clock_t)SYNC_PERIOD);
	} else if (sync_state == UNSYNCED_2) {
		period_skew = T_REF - (t_ref_l_old + 3 * (rtimer_clock_t)SYNC_PERIOD);
	} else if (sync_state == UNSYNCED_3) {
		period_skew = T_REF - (t_ref_l_old + 4 * (rtimer_clock_t)SYNC_PERIOD);
	} else {
		period_skew = T_REF - (t_ref_l_old + (rtimer_clock_t)SYNC_PERIOD);
	}
}


char sync(struct rtimer *t, void *ptr) {
	PT_BEGIN(&pt);

	if (IS_INITIATOR()) {	// initiator
		while (1) {
			leds_on(LEDS_GREEN);
			sync_data.seq_no++;
			glossy_start((uint8_t *)&sync_data, DATA_LEN, GLOSSY_INITIATOR, GLOSSY_SYNC, N_TX);
			t_start = RTIMER_TIME(t);
			SCHEDULE_SYNC(t_start, SYNC_ON);
			PT_YIELD(&pt);

			leds_off(LEDS_GREEN);
			glossy_stop();
			SCHEDULE_SYNC(t_start, SYNC_PERIOD);
			PT_YIELD(&pt);
		}
	} else {	// receiver
		SYNC_LEDS(sync_state + 1);
		while (1) {
			if (sync_state == BOOTSTRAP) {
				glossy_start((uint8_t *)&sync_data, DATA_LEN, GLOSSY_RECEIVER, GLOSSY_SYNC, N_TX);
				SCHEDULE_SYNC(RTIMER_TIME(t), SYNC_ON + T_GUARD_1);
				PT_YIELD(&pt);
				while (!is_t_ref_l_updated()) {
					glossy_reset();
					SCHEDULE_SYNC(RTIMER_TIME(t), SYNC_ON + T_GUARD_1);
					PT_YIELD(&pt);
				};

			} else {
				glossy_start((uint8_t *)&sync_data, DATA_LEN, GLOSSY_RECEIVER, GLOSSY_SYNC, N_TX);
				SCHEDULE_SYNC(RTIMER_TIME(t), SYNC_ON + T_guard);
				PT_YIELD(&pt);
			}

			sync_rx_cnt = glossy_stop();
			if (GLOSSY_IS_SYNCED()) {
				if (sync_state == BOOTSTRAP) {
					period_skew = 0;
					sync_state = SYNCED_1;
					T_guard = T_GUARD_3;
				} else {
					estimate_period_skew();
					sync_state = SYNCED;
					T_guard = T_GUARD;
				}
				SCHEDULE_SYNC(T_REF, SYNC_PERIOD + period_skew - T_guard);
				t_ref_l_old = T_REF;
				process_poll(&sync_print);
			} else {
				set_t_ref_l(T_REF + SYNC_PERIOD + period_skew);
				if (sync_state == SYNCED) {
					sync_state = UNSYNCED_1;
					T_guard = T_GUARD_1;
					SCHEDULE_SYNC(T_REF, SYNC_PERIOD + period_skew - T_guard);
				} else if (sync_state == UNSYNCED_1) {
					sync_state = UNSYNCED_2;
					T_guard = T_GUARD_2;
					SCHEDULE_SYNC(T_REF, SYNC_PERIOD + period_skew - T_guard);
				} else if (sync_state == UNSYNCED_2) {
					sync_state = UNSYNCED_3;
					T_guard = T_GUARD_3;
					SCHEDULE_SYNC(T_REF, SYNC_PERIOD + period_skew - T_guard);
				} else {
					sync_state = BOOTSTRAP;
				}
			}
			SYNC_LEDS(sync_state + 1);
			if (sync_state != BOOTSTRAP) {
				PT_YIELD(&pt);
			}
		}
	}

	PT_END(&pt);
}

static inline void set_tx_power(uint8_t power) {
	uint16_t reg;
	FASTSPI_GETREG(CC2420_TXCTRL, reg);
	reg = (reg & 0xffe0) | (power & 0x1f);
	FASTSPI_SETREG(CC2420_TXCTRL, reg);
}

PROCESS(sync_init, "Sync init");
AUTOSTART_PROCESSES(&sync_init);
PROCESS_THREAD(sync_init, ev, data)
{
	PROCESS_BEGIN();

	set_tx_power(31);
	sync_data.seq_no = 0;
	sync_state = BOOTSTRAP;
	process_start(&sync_print, NULL);
	process_start(&glossy_process, NULL);
	rtimer_set(&rt, RTIMER_NOW() + RTIMER_SECOND, 1, (rtimer_callback_t)sync, NULL);

	PROCESS_END();
}
