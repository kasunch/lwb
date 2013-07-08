/*
 * scheduler.c
 *
 *  Created on: Aug 3, 2011
 *      Author: fe
 */

static uint16_t n_added, n_deleted, n_no_space, n_replaced, n_duplicates = 0;
static rtimer_clock_t t_sched_start, t_sched_stop, t_period_start, t_period_stop;

static uint32_t data_cnt;
static uint16_t data_ipi, curr_gcd, T_opt;
static uint8_t  saturated, n_slots_assigned;
static slots_info *slot, *last_rtx, *last, *to_last;
#ifndef PERIOD_MAX
#define PERIOD_MAX 30
#endif /* PERIOD_MAX */

void add_stream(uint16_t id, stream_req req) {
	stream_info *stream = memb_alloc(&streams_memb);
	if (stream == NULL) {
		n_no_space++;
		// no space for new streams
		return;
	}
	stream->node_id = id;
	stream->ipi = req.ipi;
	stream->last_assigned = req.time_info - req.ipi;
	stream->next_ready = req.time_info;
	stream->stream_id = GET_STREAM_ID(req.req_type);
	stream->n_cons_missed = 0;
	// insert the stream into the list, ordered by node id
	stream_info *prev_stream;
	for (prev_stream = list_head(streams_list); prev_stream != NULL; prev_stream = prev_stream->next) {
		if ((id >= prev_stream->node_id) &&
				((prev_stream->next == NULL) || (id < prev_stream->next->node_id))) {
			break;
		}
	}
	list_insert(streams_list, prev_stream, stream);
	n_streams++;
	n_added++;
}

void del_stream(stream_info *stream) {
	if (stream == NULL) {
		// wrong pointer, don't do anything
		return;
	}
	n_slots_to_assign -= ((stream->next_ready - stream->last_assigned) / stream->ipi - 1);
#if RTX
	slots_info *slot;
	for (slot = list_head(slots_list); slot != NULL; ) {
		if (slot->stream == stream) {
			slots_info *slot_to_remove = slot;
			slot = slot->next;
			list_remove(slots_list, slot_to_remove);
			memb_free(&slots_memb, slot_to_remove);
		} else {
			slot = slot->next;
		}
	}
#endif /* RTX */
	list_remove(streams_list, stream);
	memb_free(&streams_memb, stream);
	n_streams--;
	n_deleted++;
}

void add_sink(uint16_t id) {
	sinks_info *sink = memb_alloc(&sinks_memb);
	if (sink == NULL) {
		// no more space for new sinks: exit
		return;
	}
	sink->node_id = id;
	sink->n_acks_missed = 0;
	// insert the sink into the list, ordered by node id
	sinks_info *prev_sink;
	for (prev_sink = list_head(sinks_list); prev_sink != NULL; prev_sink = prev_sink->next) {
		if ((id >= prev_sink->node_id) &&
				((prev_sink->next == NULL) || (id < prev_sink->next->node_id))) {
			break;
		}
	}
	list_insert(sinks_list, prev_sink, sink);
}

void process_stream_req(uint16_t id, stream_req req) {
	stream_info *curr_stream;
	if (GET_STREAM_ID(req.req_type) <= 2 && id != node_id) {
		// FIXME: temporary fix to filter out corrupted requests received during free slots
	if (IS_STREAM_TO_ADD(req.req_type)) {
		// check if there is already in the list a stream for this node with the same stream id
		for (curr_stream = list_head(streams_list); curr_stream != NULL; curr_stream = curr_stream->next) {
			if ((curr_stream->node_id == id) && (curr_stream->stream_id == GET_STREAM_ID(req.req_type))) {
				// if so, do not add it: it's a duplicate
				// the node is still alive
				curr_stream->n_cons_missed = 0;
				n_duplicates++;
				return;
			}
		}
		// add the new stream
		add_stream(id, req);
	} else {
		if (IS_STREAM_TO_REP(req.req_type)) {
			// check if there is already in the list a stream for this node with the same stream id
			for (curr_stream = list_head(streams_list); curr_stream != NULL; curr_stream = curr_stream->next) {
				if ((curr_stream->node_id == id) && (curr_stream->stream_id == GET_STREAM_ID(req.req_type))) {
					// if so, remove it in order to replace it with the new stream
					del_stream(curr_stream);
					break;
				}
			}
			// add the new stream
			add_stream(id, req);
			n_replaced++;
		} else {
			if (IS_STREAM_TO_DEL(req.req_type)) {
				// check if there is already in the list a stream for this node with the same stream id
				for (curr_stream = list_head(streams_list); curr_stream != NULL; curr_stream = curr_stream->next) {
					if ((curr_stream->node_id == id) && (curr_stream->stream_id == GET_STREAM_ID(req.req_type))) {
						// if so, remove it in order to replace it with the new stream
						del_stream(curr_stream);
						break;
					}
				}
			} else {
				if (IS_DATA_SINK(req.req_type)) {
					sinks_info *sink;
					// check if this sink is already in the list
					for (sink = list_head(sinks_list); sink != NULL; sink = sink->next) {
						if (sink->node_id == id) {
							// if so, do not add it
							return;
						}
					}
					// add this node to the list of sinks
					add_sink(id);
				}
			}
		}
	}
	}
}

// implementation of the binary GCD algorithm
static inline uint16_t gcd(uint16_t u, uint16_t v) {
    uint16_t shift;

    /* GCD(0,x) := x */
    if (u == 0 || v == 0) {
      return u | v;
    }

    /* Let shift := lg K, where K is the greatest power of 2
       dividing both u and v. */
    for (shift = 0; ((u | v) & 1) == 0; ++shift) {
        u >>= 1;
        v >>= 1;
    }

    while ((u & 1) == 0)
      u >>= 1;

    /* From here on, u is always odd. */
    do {
        while ((v & 1) == 0)  /* Loop X */
          v >>= 1;

        /* Now u and v are both odd, so diff(u, v) is even.
           Let u = min(u, v), v = diff(u, v)/2. */
        if (u < v) {
            v -= u;
        } else {
            uint16_t diff = u - v;
            u = v;
            v = diff;
        }
        v >>= 1;
    } while (v != 0);

    return u << shift;
}

static inline void change_period(void) {
	saturated = 0;
	if (data_cnt) {
		T_opt = (data_ipi * N_SLOTS_MAX) / data_cnt;
		if (T_opt < 1 * TIME_SCALE) {
			saturated = 1;
		}
	} else {
		// no streams yet: keep period at the current value
		return;
	}
	if (time < 180 * TIME_SCALE) {
		period = INIT_PERIOD;
		return;
	}
#if !COOJA
	if (!IS_LESS_EQ_THAN_TIME_LONG(last_free_reqs_time + T_LAST_FREE_REQS)) {
		// we have received stream requests in the last T_LAST_FREE_REQS seconds:
		// set the period to a low value
		period = 2;
		return;
	}
#endif /* !COOJA */
	if (T_opt <= PERIOD_MAX * TIME_SCALE) {
		if (saturated) {
			// T_opt < 1 * TIME_SCALE
			// TODO: notify about insufficient bandwidth
			period = INIT_PERIOD;
		} else {
			// T_opt between [1, PERIOD_MAX] * TIME_SCALE
			period = T_opt / TIME_SCALE;
		}
	} else {
		// T_opt > PERIOD_MAX
		period = PERIOD_MAX;
	}
//	period = 15;
}

static inline void update_streams_info(void) {
	stream_info *curr_stream;
	for (curr_stream = list_head(streams_list); curr_stream != NULL; curr_stream = curr_stream->next) {
		// compute how many (new) slots have to be allocated for this stream
		if (IS_LESS_EQ_THAN_TIME(curr_stream->next_ready)) {
			uint16_t n_new_slots = ((uint16_t)time - curr_stream->next_ready) / curr_stream->ipi + 1;
			n_slots_to_assign += n_new_slots;
			curr_stream->next_ready += n_new_slots * curr_stream->ipi;
		}
		if (RTIMER_CLOCK_LT(curr_stream->last_assigned + (BUFFER_SIZE + 1) * curr_stream->ipi, curr_stream->next_ready)) {
			// buffer overflow at this stream: do not consider the oldest packets anymore
			uint16_t last_assigned = curr_stream->last_assigned;
			curr_stream->last_assigned = curr_stream->next_ready - (BUFFER_SIZE + 1) * curr_stream->ipi;
			n_slots_to_assign -= (curr_stream->last_assigned - last_assigned) / curr_stream->ipi;
		}
		pkt_buf[curr_stream->node_id] = (curr_stream->next_ready - curr_stream->last_assigned) / curr_stream->ipi - 1;
	}
}

void compute_schedule(void) {
	t_sched_start = RTIMER_NOW();
	leds_on(LEDS_ALL);
	// store information about the last schedule
	old_sched = sched;
	// clear the content of the last schedule
	memset((uint8_t *)&sched.slot, 0, sizeof(sched.slot));
	data_ipi = 1;
	data_cnt = 0;
	s_ack = 0;
	// number of slots assigned in the schedule
	n_slots_assigned = 0;
	// embed the node id of the host
	sched.host_id = node_id;
	last_rtx = NULL;
	to_last = NULL;
	last = NULL;

	if (stream_ack_idx) {
		// there's at least one s-ack to send: allocate the first slot to the host itself
		s_ack = 1;
		n_slots_assigned = 1;
	}
	// current stream being processed
	stream_info *curr_stream;
	for (curr_stream = list_head(streams_list); curr_stream != NULL; ) {
		if (curr_stream->n_cons_missed & 0x80) {
			// no packets were received from this stream during the last round
			curr_stream->n_cons_missed &= 0x7f;
			curr_stream->n_cons_missed++;
		}
		if (curr_stream->n_cons_missed > N_CONS_MISSED_MAX) {
			// too many consecutive rounds without reception: delete this stream
			stream_info *stream_to_remove = curr_stream;
			curr_stream = curr_stream->next;
			del_stream(stream_to_remove);
		} else {
			curr_gcd = gcd(data_ipi, curr_stream->ipi);
			uint16_t k1 = curr_stream->ipi / curr_gcd;
			uint16_t k2 = data_ipi / curr_gcd;
			data_cnt = data_cnt * k1 + k2;
			data_ipi = data_ipi * k1;
			curr_stream = curr_stream->next;
		}
	}

	t_period_start = RTIMER_NOW();
	// adapt the round period
	change_period();
	t_period_stop = RTIMER_NOW();

	// increment time by the current period
	time += (period * TIME_SCALE);

	uint8_t n_free = 1;
	if (IS_LESS_EQ_THAN_TIME_LONG(last_free_reqs_time + T_LAST_FREE_REQS) &&
			(!IS_LESS_EQ_THAN_TIME_LONG(last_free_slot_time + T_LAST_FREE_SLOT))) {
		// the last stream request during a free slot was received more than T_LAST_FREE_REQS ago
		// and we scheduled a free slot less than T_LAST_FREE_SLOT ago:
		// do not schedule any free slot
		n_free = 0;
	}
	if (n_free) {
		last_free_slot_time = time;
	}
	N_SLOTS = SET_N_FREE(n_free);
	if (n_streams == 0) {
		// no streams to process
		goto set_schedule;
	}

	update_streams_info();

	if (list_head(slots_list) != NULL) {
		// some data slots were allocated in the previous round: compute cumulative ack
		cumulative_ack = ack;
		sinks_info *sink;
		for (sink = list_head(sinks_list); sink != NULL; ) {
			cumulative_ack &= sink->ack;
			if (sink->ack == 0) {
				sink->n_acks_missed++;
			}
			if (sink->n_acks_missed == N_CONS_MISSED_MAX) {
				// too many rounds without receiving an ack: remove this sink
				sinks_info *sink_to_remove = sink;
				sink = sink->next;
				list_remove(sinks_list, sink_to_remove);
			} else {
				sink = sink->next;
			}
		}
	}

#if RTX
	for (slot = list_head(slots_list), idx = 0; slot != NULL; idx++) {
		uint16_t id = slot->stream->node_id;
		pkt_tot[id]++;
		if ((cumulative_ack >> idx) & 1) {
//				|| (RTIMER_CLOCK_LT(slot->gen_time + BUFFER_SIZE * slot->stream->ipi, slot->stream->next_ready))) {
			// the packet was received:
			// do not allocate any more slots for this packet
			slots_info *slot_to_remove = slot;
			slot = slot->next;
			list_remove(slots_list, slot_to_remove);
			memb_free(&slots_memb, slot_to_remove);
			if ((cumulative_ack >> idx) & 1) {
				pkt_rcvd[id]++;
			}
			if (pkt_tot[id] >= DATA_THR) {
				pkt_tot[id] = (pkt_tot[id] % 2) + (pkt_tot[id] / 2);
				pkt_rcvd[id] = (pkt_rcvd[id] % 2) + (pkt_rcvd[id] / 2);
			}
		} else {
			// the packet was not received:
			// allocate again a slot for this packet in the next round
			slot->rtx++;
			last_rtx = slot;
			slot = slot->next;
			n_slots_assigned++;
			uint8_t b = (slot->stream->next_ready - slot->gen_time) / slot->stream->ipi;
			if (b > pkt_buf[slot->stream->node_id]) {
				pkt_buf[slot->stream->node_id] = b;
			}
		}
	}
#else
	// re-initialize the list of slots
	memb_init(&slots_memb);
	list_init(slots_list);
#endif /* RTX */

	// random initial position in the list
	uint16_t rand_init_pos = (random_rand() >> 1) % n_streams;
	uint16_t i;
	curr_stream = list_head(streams_list);
	// make curr_stream point to the random initial position
	for (i = 0; i < rand_init_pos; i++) {
		curr_stream = curr_stream->next;
	}
	// initial stream being processed
	stream_info *init_stream = curr_stream;
	do {
		// allocate slots for this stream, if possible
		if ((n_slots_assigned < N_SLOTS_MAX) &&
				IS_LESS_EQ_THAN_TIME(curr_stream->last_assigned + curr_stream->ipi)) {
			uint16_t to_assign = ((uint16_t)time - curr_stream->last_assigned) / curr_stream->ipi;
			if (saturated) {
				if (curr_stream->next == init_stream || (curr_stream->next == NULL && rand_init_pos == 0)) {
					// last random stream: allocate all possible slots
				} else {
					// ensure fairness among source nodes when the bandwidth saturates
					// allocate a number of slots proportional to (1/IPI)
					uint16_t slots_ipi = T_opt / curr_stream->ipi;
					if (to_assign > slots_ipi) {
						to_assign = slots_ipi;
						if (to_assign == 0 && curr_stream == init_stream) {
							// first random stream: allocate one slot to it, even if it has very long IPI
							to_assign = 1;
						}
					}
				}
			}
			if (to_assign > N_SLOTS_MAX - n_slots_assigned) {
				to_assign = N_SLOTS_MAX - n_slots_assigned;
			}
			for (i = 0; i < to_assign; i++) {
				slot = memb_alloc(&slots_memb);
				if (slot != NULL) {
					slot->stream = curr_stream;
					slot->gen_time = curr_stream->last_assigned + (i+1) * curr_stream->ipi;
					slot->rtx = 0;
					list_add(slots_list, slot);
					last = slot;
				} else {
					leds_toggle(LEDS_RED);
				}
			}
			n_slots_assigned += to_assign;
			n_slots_to_assign -= to_assign;
			curr_stream->last_assigned += to_assign * curr_stream->ipi;
			curr_stream->n_cons_missed |= 0x80;
		}

		// go to the next stream in the list
		curr_stream = curr_stream->next;
		if (curr_stream == NULL) {
			// end of the list: start again from the head of the list
			curr_stream = list_head(streams_list);
			to_last = last;
		}
	} while (curr_stream != init_stream);

set_schedule:
	// order "new" slots by node_id (and stream_id)
	if (last != NULL && to_last != NULL && to_last != last) {
		if (last_rtx == NULL) {
			last->next = *slots_list;
			*slots_list = to_last->next;
		} else {
			last->next = last_rtx->next;
			last_rtx->next = to_last->next;
		}
		to_last->next = NULL;
	}
	if (last_rtx != NULL) {
		slots_info *first_new_slot = last_rtx->next;
		for (slot = list_head(slots_list); slot != first_new_slot; ) {
			uint16_t id = slot->stream->node_id;
			slots_info *prev_slot;
			for (prev_slot = first_new_slot; prev_slot != NULL; prev_slot = prev_slot->next) {
				if ((id > prev_slot->stream->node_id) &&
						((prev_slot->next == NULL) || (id <= prev_slot->next->stream->node_id))) {
					break;
				}
			}
			slots_info *slot_to_insert = slot;
			slot = slot->next;
			if (prev_slot != NULL) {
				list_remove(slots_list, slot_to_insert);
				list_insert(slots_list, prev_slot, slot_to_insert);
			}
		}
	}

	// allocate all current slots to the schedule
	if (s_ack) {
		snc[0] = 0;
		stream_id[0] = 0;
		pkt_id[0] = 0;
		idx = 1;
	} else {
		idx = 0;
	}
	for (slot = list_head(slots_list); slot != NULL; slot = slot->next, idx++) {
		if (slot->stream != NULL) {
			curr_stream = slot->stream;
			curr_stream->n_cons_missed |= 0x80;
			snc[idx] = curr_stream->node_id;
			stream_id[idx] = curr_stream->stream_id;
			pkt_id[idx] = (uint16_t)time - slot->gen_time;
		}
	}

	// allocate ack slots for sinks different from the host
	n_sinks = 0;
	if (n_slots_assigned - s_ack) {
		sinks_info *sink;
		for (sink = list_head(sinks_list); sink != NULL; sink = sink->next, n_sinks++) {
			curr_sinks[n_sinks] = sink->node_id;
			sink->ack = 0;
		}
	}

	// embed the number of assigned slots into the schedule
	N_SLOTS |= GET_N_SLOTS(n_slots_assigned);
	compress_schedule(GET_N_SLOTS(N_SLOTS));
	if (period != 1) {
		// this schedule is sent at the end of a round: do not communicate
		PERIOD = period | SET_COMM(0);
		TIME = (uint16_t)time - (period - 1) * TIME_SCALE;
	} else {
		// T = 1: there are no schedules sent at the end of a round, always communicate
		PERIOD = period | SET_COMM(1);
		TIME = (uint16_t)time;
	}
	t_sched_stop = RTIMER_NOW();
	leds_off(LEDS_ALL);
}

void scheduler_init(void) {
	// initialize streams member and list
	memb_init(&streams_memb);
	list_init(streams_list);
	memb_init(&slots_memb);
	list_init(slots_list);
	memb_init(&sinks_memb);
	list_init(sinks_list);

	// initialize persistent variables
	time = INIT_PERIOD * TIME_SCALE;
	n_slots_to_assign = 0;
	period = INIT_PERIOD;
	PERIOD = period | SET_COMM(1);
	if (period != 1) {
		OLD_PERIOD = period | SET_COMM(0);
	} else {
		OLD_PERIOD = 1 | SET_COMM(1);
	}

	n_sinks = 0;
	n_streams = 0;
	compute_schedule();
}
