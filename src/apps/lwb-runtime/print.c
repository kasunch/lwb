/*
 * print.c
 *
 *  Created on: Aug 3, 2011
 *      Author: fe
 */

static uint32_t tot = 0;

void print_logs(uint8_t times) {
#if !LWB_DEBUG
	printf("n_sinks %u\n", n_sinks);
	if (is_sink) {
		for (idx = 0; idx < n_delivered; idx++) {
#if COOJA
			printf("rcvd %5lu %3u %5u %5u %3u 0 0\n", tot, delivered_pkt[idx].node_id, delivered_pkt[idx].gen_time, delivered_pkt[idx].data.seq_no, (is_host ? (old_sched.time + 1) : TIME) - delivered_pkt[idx].gen_time);
#else
			printf("rcvd %5lu %3u %5u %5u %3u %10lu %10lu\n", tot, delivered_pkt[idx].node_id, delivered_pkt[idx].gen_time, delivered_pkt[idx].data.seq_no, (is_host ? (old_sched.time + 1) : TIME) - delivered_pkt[idx].gen_time, delivered_pkt[idx].data.en_on, delivered_pkt[idx].data.en_tot);
#endif /* COOJA */
			tot++;
		}
		n_delivered = 0;
		printf("my ack: %012llx\n", ack);
		printf("n_null %u, n_not_zero_rcvd %u, n_not_zero_already_rcvd %u\n", n_null, n_not_zero_rcvd, n_not_zero_already_rcvd);
	}
	if (is_host) {
		printf("cumulative ack: %012llx\n", cumulative_ack);
		printf("sched_size %u\n", sched_size);
//		printf("my ack: %llu\n", ack);
//		sinks_info *sink;
//		for (sink = list_head(sinks_list); sink != NULL; sink = sink->next) {
//			printf("%u ack: %llu\n", sink->node_id, sink->ack);
//		}
	}
	uint8_t i;
	if (is_sink) {
		if (is_host) {
			for (idx = 0; idx < N_NODES_MAX; idx++) {
				if (pkt_tot[idx]) {
					for (i = 0; i < times; i++) {
						printf("%3u ", node_id);
						printf("1 %3u %5u %5u %5u %5u %5u %2u\n", idx, dc[idx],
								 ps[idx], (uint16_t)((10000LU * pkt_rcvd[idx]) / pkt_tot[idx]), Ts[idx], Td[idx], pkt_buf[idx]);
					}
				}
			}
#if RTX
//			for (idx = 0; idx < GET_N_SLOTS(N_SLOTS); idx++) {
//				if (snc[idx] != 0) {
//					printf("time %u - %u slot %u %u %u\n", TIME, idx, snc[idx], stream_id[idx], pkt_id[idx]);
//				}
//			}
//			printf("slots ");
//			for (idx = 0; idx < GET_N_SLOTS(N_SLOTS); idx++) {
//				printf("%u[%u] ", snc[idx], pkt_id[idx]);
//			}
//			printf("\n");
			printf("time %u - %u slots\n", TIME, GET_N_SLOTS(N_SLOTS));
			if (s_ack) {
				idx = 1;
				printf("slot 0: 0 0 0 0 0\n");
			} else {
				idx = 0;
			}
			slots_info *slot;
			for (slot = list_head(slots_list); slot != NULL; slot = slot->next, idx++) {
				if (slot->stream != NULL) {
					printf("slot %u: %u %u %u %u %u\n", idx, slot->stream->node_id, slot->stream->stream_id, slot->gen_time, pkt_id[idx], slot->rtx);
				}
			}
#endif /* RTX */
		} else {
			for (idx = 0; idx < N_NODES_MAX; idx++) {
				if (pkt_rcvd[idx]) {
					for (i = 0; i < times; i++) {
						printf("%3u ", node_id);
						printf("2 %3u %5u %5u 0 0\n", idx, dc[idx], pkt_rcvd[idx]);
					}
				}
			}
		}
	} else {
		if (is_host) {
			for (idx = 0; idx < N_NODES_MAX; idx++) {
				if (pkt_tot[idx]) {
					for (i = 0; i < times; i++) {
						printf("%3u ", node_id);
						printf("0 %3u 0 0 0 %5u\n", idx, pkt_tot[idx]);
					}
				}
			}
		} else {
		}
	}
	// print information about myself
	uint32_t en_on = energest_type_time(ENERGEST_TYPE_LISTEN) + energest_type_time(ENERGEST_TYPE_TRANSMIT);
	uint32_t en_tot = energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM);
	for (i = 0; i < times; i++) {
		printf("%3u ", node_id);
		printf("3 %3u %1u %3u %5ld %5u %5u %2u %4u\n", node_id, sync_state,
				(relay_cnt_cnt) ? ((relay_cnt_sum * RELAY_CNT_FACTOR) / relay_cnt_cnt) : (RELAY_CNT_INVALID),
				skew,
				(uint16_t)((10000LLU * en_control)  / en_tot),
				(uint16_t)((10000LLU * en_on) / en_tot),
				period,
				(uint16_t)((  100LLU * period * data_cnt) / data_ipi)
		);
	}
	#else
	if (period > 3) {
		if (is_host) {
			uint32_t T_scheduler = 1000000LU * (t_sched_stop - t_sched_start) / RTIMER_SECOND;
			printf("time %5lu, streams %3u, slots %3u, to_assign %3u, sched us %6lu",
					time, n_streams, GET_N_SLOTS(N_SLOTS), n_slots_to_assign, T_scheduler);
			printf(", sack_idx %2u", stream_ack_idx);
			if (stream_ack_idx) {
				printf(", sack ");
				for (idx = 0; idx < stream_ack_idx; idx++) {
					printf("%3u ", stream_ack[idx]);
				}
			}
			printf("\n");
			printf("last_free_slot %u, last_free_reqs %u, n_free %u\n",
					last_free_slot_time, last_free_reqs_time, GET_N_FREE(N_SLOTS));
			uint32_t T_period = 1000000LU * (t_period_stop - t_period_start) / RTIMER_SECOND;
			printf("period us %6lu\n", T_period);
			uint16_t ratio = 0;
			if (GET_N_SLOTS(N_SLOTS)) {
				ratio = (10000LU * (sched_size - SYNC_HEADER_LENGTH)) / (GET_N_SLOTS(N_SLOTS) * 2);
			}
			uint32_t T_compress = 1000000LU * (t_compr_stop - t_compr_start) / RTIMER_SECOND;
			printf("sched size %u, compr ratio %u, ack_snc %u, first_snc %u, us %lu\n",
					sched_size, ratio, ack_snc, first_snc, T_compress);
			printf("control_dc %5u\n", (uint16_t)((10000LLU * en_control) / (energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM))));
			printf("period %u, T_min %u, data_ipi %u, data_cnt %lu, saturated %u\n", period, T_min, data_ipi, data_cnt, saturated);
			printf("n_added %u, n_deleted %u, n_no_space %u, n_replaced %u, n_duplicates %u\n",
					n_added, n_deleted, n_no_space, n_replaced, n_duplicates);
#if GLOSSY_DEBUG
			printf("high_T_irq %u, rx_timeout %u, bad_length %u, bad_header %u, bad_crc %u\n",
					high_T_irq, rx_timeout, bad_length, bad_header, bad_crc);
#endif /* GLOSSY_DEBUG */
		} else {
			printf("time %5lu, period %3u, skew %ld, sync_state %1u, otg %2u\n", time, period, skew, sync_state, rt.overflows_to_go);
			uint32_t en_on = energest_type_time(ENERGEST_TYPE_LISTEN) + energest_type_time(ENERGEST_TYPE_TRANSMIT);
			uint32_t en_tot = energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM);
			printf("comm %1u, dc %5u, en_on %lu, en_tot %lu\n", GET_COMM(OLD_PERIOD), (uint16_t)((10000LLU * en_on) / en_tot), en_on, en_tot);
			printf("control_dc %5u\n", (uint16_t)((10000LLU * en_control) / en_tot));
			uint32_t T_uncompress = 1000000LU * (t_uncompr_stop - t_uncompr_start) / RTIMER_SECOND;
			printf("uncompr d_bits %u, l_bits %u, us %lu\n", old_sched.slot[2] / 8, old_sched.slot[2] % 8, T_uncompress);
			if (joining_state != JOINED) {
				printf("joining_state %1u, n_joining_trials %3u, rounds_to_wait %3u\n",
						joining_state, n_joining_trials, rounds_to_wait);
			}
#if GLOSSY_DEBUG
			printf("high_T_irq %u, rx_timeout %u, bad_length %u, bad_header %u, bad_crc %u\n",
					high_T_irq, rx_timeout, bad_length, bad_header, bad_crc);
#endif /* GLOSSY_DEBUG */
		}
	}
#endif /* !LWB_DEBUG */
}

PROCESS(print, "Print");
PROCESS_THREAD(print, ev, data)
{
	PROCESS_BEGIN();

	while (1) {
		PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

#if PRINT_PERIOD
		// print only if it's time to do it
		if ((time / TIME_SCALE) / PRINT_PERIOD > print_cnt) {
			print_logs(1);
			print_cnt++;
		}
#else
		// always print
		print_logs(1);
#endif /* PRINT_PERIOD */

#if EXP_LENGTH
		if ((TIME / TIME_SCALE) > EXP_LENGTH - 300) {
			// end of the experiment: stop timers and interrupts
			TBCTL = 0;
			TACCTL0 = 0; TBCCTL0 = 0; TBCCTL1 = 0;
#if COOJA
			print_logs(1);
#else
			// wait a few random seconds to avoid nodes printing all at the same time
			clock_time_t T_wait = random_rand() % (180 * CLOCK_SECOND);
			clock_time_t now = clock_time();
			while (clock_time() < now + T_wait) {
				watchdog_periodic();
			}
			TACCTL1 = 0;
			// print the logs several times at the end of the experiment
			print_logs(10);
#endif /* COOJA */
		}
#endif /* EXP_LENGTH */

	}

	PROCESS_END();
}
