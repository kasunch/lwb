/*
 * g_rr.c
 *
 *  Created on: Aug 3, 2011
 *      Author: fe
 */

#define PKT_LEN_MAX (DATA_HEADER_LENGTH + N_STREAM_REQS_MAX * STREAM_REQ_LENGTH + DATA_LENGTH)

#define IS_THERE_DATA(a)      ((a & 0x3) == DATA)
#define IS_STREAM_ACK(a)      ((a & 0x3) == STREAM_ACK)
#define GET_N_STREAM_REQS(a)  (a >> 2)
#define SET_N_STREAM_REQS(a)  (a << 2)
#define SET_ACK(x)            (ack |= ((uint64_t)1 << x))

static uint8_t pkt_len;
static uint8_t pkt[PKT_LEN_MAX];
static uint8_t pkt_type;
static uint16_t curr_ipi;
static slots_info *slot;
static uint8_t n_rcvd = 0, n_delivered = 0, n_already_received = 0;
static sinks_info *sink;
static uint8_t ack_len;
static uint16_t n_null = 0, n_not_zero_rcvd = 0, n_not_zero_already_rcvd = 0;

void add_relay_cnt(uint16_t id, uint8_t relay_cnt) {
	stream_info *curr_stream = memb_alloc(&streams_memb);
	if (curr_stream == NULL) {
		return;
	}
	curr_stream->node_id = id;
	curr_stream->stream_id = relay_cnt;
	list_add(streams_list, curr_stream);
	n_streams++;
}

void prepare_stream_ack(void) {
	data_header.node_id = node_id;
	pkt_type = STREAM_ACK;
	uint8_t stream_ack_len = stream_ack_idx * STREAM_ACK_LENGTH;
	pkt_len = DATA_HEADER_LENGTH + STREAM_ACK_IDX_LENGTH + stream_ack_len;
	memcpy(pkt, &data_header, DATA_HEADER_LENGTH);
	memcpy(pkt + DATA_HEADER_LENGTH, &stream_ack_idx, STREAM_ACK_IDX_LENGTH);
	memcpy(pkt + DATA_HEADER_LENGTH + STREAM_ACK_IDX_LENGTH, stream_ack, stream_ack_len);
}

void prepare_data_packet(uint8_t free_slot) {
	data_header.node_id = node_id;
	data_header.gen_time = TIME - pkt_id[slot_idx];
	data_header.ps = ps[node_id];
	data_header.Ts = Ts[node_id];
	data_header.Td = Td[node_id];
	pkt_type = SET_N_STREAM_REQS(n_stream_reqs);
	if (!free_slot) {
		pkt_type |= DATA;
#if !COOJA
		data.en_on = energest_type_time(ENERGEST_TYPE_LISTEN) + energest_type_time(ENERGEST_TYPE_TRANSMIT);
		data.en_tot = energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM);
#endif /* !COOJA */
		data.seq_no = (TIME - pkt_id[slot_idx] - INIT_DELAY) / INIT_IPI;
	}

	uint8_t stream_reqs_len = GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH;
	uint8_t data_len = IS_THERE_DATA(pkt_type) * DATA_LENGTH;
	pkt_len = DATA_HEADER_LENGTH + stream_reqs_len + data_len;
	memcpy(pkt, &data_header, DATA_HEADER_LENGTH);
	memcpy(pkt + DATA_HEADER_LENGTH, stream_reqs, stream_reqs_len);
	memcpy(pkt + DATA_HEADER_LENGTH + stream_reqs_len, &data, data_len);
}

void check_for_stream_requests(void) {
	if (GET_N_STREAM_REQS(pkt_type)) {
		last_free_reqs_time = time;
		// check if we already processed stream requests from this node during this round
		for (idx = 0; idx < stream_ack_idx; idx++) {
			if (stream_ack[idx] == data_header.node_id) {
				// if so, do not process them again
				return;
			}
		}
		// cycle through the stream requests and process them
		for (idx = 0; idx < GET_N_STREAM_REQS(pkt_type) && stream_ack_idx < PKT_LEN_MAX - DATA_HEADER_LENGTH; idx++) {
			process_stream_req(data_header.node_id, stream_reqs[idx]);
			// prepare the stream acknowledgment for this node
			stream_ack[stream_ack_idx] = data_header.node_id;
			stream_ack_idx++;
		}
	}
}

void deliver_data_packets(void) {
	if (list_length(already_rcvd_list)) {
		n_not_zero_already_rcvd++;
	}
	n_delivered = 0;
	rcvd_pkt_info *packet;
	for (packet = list_head(rcvd_list); packet != NULL; ) {
		uint8_t to_deliver = 1;
		for (slot_idx = 0; slot_idx < GET_N_SLOTS(N_SLOTS); slot_idx++) {
			if (snc[slot_idx] == packet->pkt_info.node_id &&
					pkt_id[slot_idx] == TIME - packet->pkt_info.gen_time &&
					stream_id[slot_idx] == packet->pkt_info.stream_id) {
				// there is still a slot for this packet in the current schedule: do not deliver it
				to_deliver = 0;
				break;
			}
		}
		if (to_deliver) {
			// deliver it to the application
			delivered_pkt[n_delivered] = packet->pkt_info;
			n_delivered++;
		} else {
			// copy it to the list of already received packets
			rcvd_pkt_info *p = memb_alloc(&already_rcvd_memb);
			if (p == NULL) {
				n_null++;
			}
			p->pkt_info = packet->pkt_info;
			list_add(already_rcvd_list, p);
			n_already_received++;
		}
		// remove it from the list of received packets
		rcvd_pkt_info *to_delete = packet;
		packet = packet->next;
		memb_free(&rcvd_memb, to_delete);
		list_remove(rcvd_list, to_delete);
	}
	if (list_length(rcvd_list)) {
		n_not_zero_rcvd++;
	}
	ack = 0;
	n_rcvd = 0;
}

void receive_data_packet(uint16_t id) {
	// get the data header
	memcpy(&data_header, pkt, DATA_HEADER_LENGTH);
	pkt_type = get_header();

	uint8_t already_rcvd = 0;
	if (is_sink) {
		// check if this packet has already been received in previous rounds
		rcvd_pkt_info *packet;
		for (packet = list_head(already_rcvd_list); packet != NULL; packet = packet->next) {
			if (packet->pkt_info.node_id == id &&
					packet->pkt_info.gen_time == TIME - pkt_id[slot_idx] &&
					packet->pkt_info.stream_id == stream_id[slot_idx]) {
				// if so, consider it as received and add it to the list of received packets
				already_rcvd = 1;
				SET_ACK(slot_idx);
				rcvd_pkt_info *p = memb_alloc(&rcvd_memb);
				if (p == NULL) {
					n_null++;
				}
				p->pkt_info = packet->pkt_info;
				list_add(rcvd_list, p);
				n_rcvd++;
				// and remove it from the list of already received packets
				list_remove(already_rcvd_list, packet);
				memb_free(&already_rcvd_memb, packet);
				n_already_received--;
			}
		}
	}

	// check if it was successfully received
	if (get_rx_cnt() && (data_header.node_id == id) && (data_header.gen_time == TIME - pkt_id[slot_idx])
				  && random_rand() < 62259
	) {
		// packet correctly received from a source node
		ps[id] = data_header.ps;
		Ts[id] = data_header.Ts;
		Td[id] = data_header.Td;
		// if not already received, add it to the list of received packets
		if (is_sink && !already_rcvd) {
			SET_ACK(slot_idx);
			rcvd_pkt_info *packet = memb_alloc(&rcvd_memb);
			if (packet == NULL) {
				n_null++;
			}
			if (IS_THERE_DATA(pkt_type)) {
				memcpy(&data, pkt + DATA_HEADER_LENGTH + GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH, IS_THERE_DATA(pkt_type) * DATA_LENGTH);
				packet->pkt_info.node_id = id;
				packet->pkt_info.stream_id = stream_id[slot_idx];
				packet->pkt_info.gen_time = data_header.gen_time;
				packet->pkt_info.data = data;
#if !COOJA
				dc[id] = (uint16_t)((10000LLU * data.en_on) / data.en_tot);
#endif /* !COOJA */
			}
			list_add(rcvd_list, packet);
			n_rcvd++;
		}
		if (is_host) {
			slot->stream->n_cons_missed = 0;
			memcpy(stream_reqs, pkt + DATA_HEADER_LENGTH, GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH);
			check_for_stream_requests();
		}
	}
}

PT_THREAD(g_rr_host(struct rtimer *t, void *ptr)) {
	PT_BEGIN(&pt_rr);

	while (1) {
		if (is_sink) {
			deliver_data_packets();
		}
		slot = list_head(slots_list);
		for (slot_idx = 0; slot_idx < GET_N_SLOTS(N_SLOTS); slot_idx++) {
			leds_off(LEDS_ALL);
			SCHEDULE(T_REF, T_SYNC_ON + 10 * T_GAP + slot_idx * (T_RR_ON + T_GAP), g_rr_host);
			PT_YIELD(&pt_rr);

			leds_on(slot_idx + 1);
			if (snc[slot_idx] == 0) {
				prepare_stream_ack();
				save_energest_values();
				glossy_start((uint8_t *)pkt, pkt_len, GLOSSY_INITIATOR, GLOSSY_NO_SYNC, N_RR, pkt_type,
						T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 9 * T_GAP, (rtimer_callback_t)g_rr_host, &rt, NULL);
			} else {
				glossy_start((uint8_t *)pkt, 0, GLOSSY_RECEIVER, GLOSSY_NO_SYNC, N_RR, 0,
						T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 9 * T_GAP, (rtimer_callback_t)g_rr_host, &rt, NULL);
			}
			PT_YIELD(&pt_rr);

			glossy_stop();
			uint16_t id = snc[slot_idx];
			if (id == 0) {
				// we just transmitted a s-ack
				update_control_dc();
				memset(stream_ack, 0, STREAM_ACK_LENGTH);
				stream_ack_idx = 0;
			} else {
				if (id != node_id) {
					// we are expecting a data packet
					receive_data_packet(id);
				}
				slot = slot->next;
			}
		}
		if (is_sink && snc[0] == 0) {
			ack >>= 1;
		}
		sink = list_head(sinks_list);
		for (; slot_idx < GET_N_SLOTS(N_SLOTS) + n_sinks; slot_idx++) {
			leds_off(LEDS_ALL);
			SCHEDULE(T_REF, T_SYNC_ON + 10 * T_GAP + slot_idx * (T_RR_ON + T_GAP), g_rr_host);
			PT_YIELD(&pt_rr);

			leds_on(slot_idx + 1);
			ack_len = GET_N_SLOTS(N_SLOTS) / 8 + ((GET_N_SLOTS(N_SLOTS) % 8) ? 1 : 0);
			save_energest_values();
			// receive ack
			glossy_start((uint8_t *)&sink->ack, ack_len, GLOSSY_RECEIVER, GLOSSY_NO_SYNC, N_RR, pkt_type,
					T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 9 * T_GAP, (rtimer_callback_t)g_rr_host, &rt, NULL);
			PT_YIELD(&pt_rr);

			glossy_stop();
			sink = sink->next;
		}
		for (; slot_idx < GET_N_SLOTS(N_SLOTS) + n_sinks + GET_N_FREE(N_SLOTS); slot_idx++) {
			leds_off(LEDS_ALL);
			SCHEDULE(T_REF, T_SYNC_ON + 10 * T_GAP + slot_idx * (T_RR_ON + T_GAP), g_rr_host);
			PT_YIELD(&pt_rr);

			leds_on(slot_idx + 1);
			save_energest_values();
			glossy_start((uint8_t *)pkt, 0, GLOSSY_RECEIVER, GLOSSY_NO_SYNC, N_RR, 0,
					T_REF + T_SYNC_ON + (slot_idx) * (T_RR_ON + T_GAP) + 10 * T_GAP + T_FREE_ON, (rtimer_callback_t)g_rr_host, &rt, NULL);
			PT_YIELD(&pt_rr);

			if (glossy_stop() && ((get_data_len() - DATA_HEADER_LENGTH) % STREAM_REQ_LENGTH == 0)) {
				// get the data header
				memcpy(&data_header, pkt, DATA_HEADER_LENGTH);
				pkt_type = get_header();
				memcpy(stream_reqs, pkt + DATA_HEADER_LENGTH, GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH);
				check_for_stream_requests();
			}
			update_control_dc();
		}
		compute_schedule();
		// schedule g_sync in 1 second
		if (period == 1) {
			// use t_start as the reference point
			SCHEDULE(t_start, RTIMER_SECOND * 1, g_sync);
		} else {
			// use t_ref as the reference point
			SCHEDULE(T_REF, RTIMER_SECOND * 1, g_sync);
		}
		PT_YIELD(&pt_rr);
	}

	PT_END(&pt_rr);
}

PT_THREAD(g_rr_source(struct rtimer *t, void *ptr)) {
	PT_BEGIN(&pt_rr);

	while (1) {
		if (is_sink) {
			deliver_data_packets();
		}
		for (slot_idx = 0; slot_idx < GET_N_SLOTS(N_SLOTS); slot_idx++) {
			leds_off(LEDS_ALL);
			SCHEDULE(T_REF, T_SYNC_ON + 10 * T_GAP + slot_idx * (T_RR_ON + T_GAP), g_rr_source);
			PT_YIELD(&pt_rr);

			leds_on(slot_idx + 1);
			save_energest_values();
			if (snc[slot_idx] == node_id) {
				prepare_data_packet(0);
				glossy_start((uint8_t *)pkt, pkt_len, GLOSSY_INITIATOR, GLOSSY_NO_SYNC, N_RR, pkt_type,
						T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 9 * T_GAP, (rtimer_callback_t)g_rr_source, &rt, NULL);
				PT_YIELD(&pt_rr);

				glossy_stop();
				update_Td();
				if (is_sink) {
					SET_ACK(slot_idx);
				}
			} else {
				glossy_start((uint8_t *)pkt, 0, GLOSSY_RECEIVER, GLOSSY_NO_SYNC, N_RR, 0,
						T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 9 * T_GAP, (rtimer_callback_t)g_rr_source, &rt, NULL);
				PT_YIELD(&pt_rr);

				glossy_stop();
				update_Td();
				uint16_t id = snc[slot_idx];
				if (id == 0) {
					// we are expecting a s-ack
					if (get_rx_cnt()) {
						// get the data header
						memcpy(&data_header, pkt, DATA_HEADER_LENGTH);
						pkt_type = get_header();
						if (IS_STREAM_ACK(pkt_type)) {
							update_control_dc();
							memcpy(&stream_ack_idx, pkt + DATA_HEADER_LENGTH, STREAM_ACK_IDX_LENGTH);
							memcpy(stream_ack, pkt + DATA_HEADER_LENGTH + STREAM_ACK_IDX_LENGTH, stream_ack_idx * STREAM_ACK_LENGTH);
							for (idx = 0; idx < stream_ack_idx; idx++) {
								if (stream_ack[idx] == node_id) {
									n_stream_reqs--;
									if (n_stream_reqs == 0) {
										joining_state = JOINED;
									}
									curr_ipi = stream_reqs[0].ipi;
								}
							}
						}
					}
				} else {
					if (id != node_id) {
						// we are expecting a data packet
						receive_data_packet(id);
					}
				}
			}
		}
		if (is_sink && snc[0] == 0) {
			ack >>= 1;
		}
		for (; slot_idx < GET_N_SLOTS(N_SLOTS) + n_sinks; slot_idx++) {
			leds_off(LEDS_ALL);
			SCHEDULE(T_REF, T_SYNC_ON + 10 * T_GAP + slot_idx * (T_RR_ON + T_GAP), g_rr_source);
			PT_YIELD(&pt_rr);

			leds_on(slot_idx + 1);
			ack_len = GET_N_SLOTS(N_SLOTS) / 8 + ((GET_N_SLOTS(N_SLOTS) % 8) ? 1 : 0);
			save_energest_values();
			if (is_sink && curr_sinks[slot_idx - GET_N_SLOTS(N_SLOTS)] == node_id) {
				// transmit ack
				glossy_start((uint8_t *)&ack, ack_len, GLOSSY_INITIATOR, GLOSSY_NO_SYNC, N_RR, pkt_type,
						T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 9 * T_GAP, (rtimer_callback_t)g_rr_source, &rt, NULL);
			} else {
				// receive ack
				glossy_start((uint8_t *)pkt, ack_len, GLOSSY_RECEIVER, GLOSSY_NO_SYNC, N_RR, pkt_type,
						T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 9 * T_GAP, (rtimer_callback_t)g_rr_source, &rt, NULL);
			}
			PT_YIELD(&pt_rr);

			glossy_stop();
			update_Td();
		}
		for (; slot_idx < GET_N_SLOTS(N_SLOTS) + n_sinks + GET_N_FREE(N_SLOTS); slot_idx++) {
			leds_off(LEDS_ALL);
			SCHEDULE(T_REF, T_SYNC_ON + 10 * T_GAP + slot_idx * (T_RR_ON + T_GAP), g_rr_source);
			PT_YIELD(&pt_rr);

			leds_on(slot_idx + 1);
			if (joining_state == JOINING && (sync_state == SYNCED || sync_state == QUASI_SYNCED) && (is_source || is_sink)) {
				n_stream_reqs = 1;
				if (rounds_to_wait == 0) {
					// try to join
					joining_state = JUST_TRIED;
					prepare_data_packet(1);
					save_energest_values();
					glossy_start((uint8_t *)pkt, pkt_len, GLOSSY_INITIATOR, GLOSSY_NO_SYNC, N_RR, pkt_type,
							T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 9 * T_GAP, (rtimer_callback_t)g_rr_source, &rt, NULL);
				} else {
					// keep waiting, decrease the number of rounds to wait
					rounds_to_wait--;
					save_energest_values();
					glossy_start((uint8_t *)pkt, 0, GLOSSY_RECEIVER, GLOSSY_NO_SYNC, N_RR, 0,
							T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 9 * T_GAP, (rtimer_callback_t)g_rr_source, &rt, NULL);
				}
			} else {
				// already joined or not SYNCED
				save_energest_values();
				glossy_start((uint8_t *)pkt, 0, GLOSSY_RECEIVER, GLOSSY_NO_SYNC, N_RR, 0,
						T_REF + T_SYNC_ON + (slot_idx) * (T_RR_ON + T_GAP) + 10 * T_GAP + T_FREE_ON, (rtimer_callback_t)g_rr_source, &rt, NULL);
			}
			PT_YIELD(&pt_rr);

			glossy_stop();
			update_control_dc();
		}
		// schedule g_sync in 1 second
		SCHEDULE(T_REF, PERIOD_RT(1) - T_guard, g_sync);
		SYNC_LEDS();

		// copy the current schedule to old_sched
		old_sched = sched;
		// erase the schedule before the receiving the next one
		memset((uint8_t *)&sched.slot, 0, sizeof(sched.slot));

		PT_YIELD(&pt_rr);
	}

	PT_END(&pt_rr);
}

