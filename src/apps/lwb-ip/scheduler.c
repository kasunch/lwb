/*
 * scheduler.c
 *
 *  Created on: Aug 3, 2011
 *      Author: fe
 */

static uint16_t n_added, n_deleted, n_no_space, n_replaced, n_duplicates = 0;
static rtimer_clock_t t_sched_start, t_sched_stop, t_period_start, t_period_stop;

#if DYNAMIC_PERIOD
static uint32_t data_cnt;
static uint16_t data_ipi, curr_gcd, T_min;
static uint8_t  saturated;
#ifndef PERIOD_MAX
#define PERIOD_MAX 30
#endif /* PERIOD_MAX */
#if MINIMIZE_LATENCY
static uint16_t min_next_ready;
#endif /* MINIMIZE_LATENCY */
#endif /* DYNAMIC_PERIOD */

//--------------------------------------------------------------------------------------------------
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
#if REMOVE_NODES
  stream->n_cons_missed = 0;
#endif /* REMOVE_NODES */
  // insert the stream into the list, ordered by node id
  stream_info *prev_stream;
  for (prev_stream = list_head(streams_list); prev_stream != NULL; prev_stream = prev_stream->next) {
    if ((id >= prev_stream->node_id) &&
        ((prev_stream->next == NULL) || (id < prev_stream->next->node_id))) {
      break;
    }
  }
//  list_add(streams_list, stream);
  list_insert(streams_list, prev_stream, stream);
  n_streams++;
  n_added++;
}

//--------------------------------------------------------------------------------------------------
void del_stream(stream_info *stream) {
  if (stream == NULL) {
    // wrong pointer, don't do anything
    return;
  }
  n_slots_to_assign -= ((stream->next_ready - stream->last_assigned) / stream->ipi - 1);
  list_remove(streams_list, stream);
  memb_free(&streams_memb, stream);
  n_streams--;
  n_deleted++;
}

//--------------------------------------------------------------------------------------------------
void process_stream_req(uint16_t id, stream_req req) {
  stream_info *curr_stream;

  if (GET_STREAM_ID(req.req_type) <= 2 && id != node_id) {
    // FIXME: temporary fix to filter out corrupted requests received during free slots
    if (IS_STREAM_TO_ADD(req.req_type)) {
      // check if there is already in the list a stream for this node with the same stream id
      for (curr_stream = list_head(streams_list); curr_stream != NULL; curr_stream = curr_stream->next) {
        if ((curr_stream->node_id == id) &&
            (curr_stream->stream_id == GET_STREAM_ID(req.req_type))) {
          // if so, do not add it: it's a duplicate
#if REMOVE_NODES
          // the node is still alive
          curr_stream->n_cons_missed = 0;
#endif /* REMOVE_NODES */
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
          if ((curr_stream->node_id == id) &&
              (curr_stream->stream_id == GET_STREAM_ID(req.req_type))) {
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
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------
#if DYNAMIC_PERIOD
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

//--------------------------------------------------------------------------------------------------
static inline void change_period(void) {
#if MINIMIZE_LATENCY
#if JOINING_NODES && DYNAMIC_FREE_SLOTS
  if (!IS_LESS_EQ_THAN_TIME(last_free_reqs_time + T_LAST_FREE_REQS) || (time < 180 * TIME_SCALE)) {
    // we have received stream requests in the last T_LAST_FREE_REQS seconds:
    // set the period to the lowest value
    period = INIT_PERIOD;
    return;
  }
#endif /* JOINING_NODES && DYNAMIC_FREE_SLOTS */
  if (n_slots_to_assign) {
    // there is still some backlog: use a short period
    period = 2;
  } else {
    period = min_next_ready;
  }
#else
  saturated = 0;
  if (data_cnt) {
    T_min = (data_ipi * N_SLOTS_MAX) / data_cnt;
    if (T_min < 1 * TIME_SCALE) {
      saturated = 1;
    } else {
      if ((data_ipi * N_SLOTS_MAX) % data_cnt == 0 && n_slots_to_assign) {
        // there is still some backlog from the previous rounds
        // and T_min is just sufficient for the current amount of data:
        // decrease T_min by 1 second
//        T_min -= (1 * TIME_SCALE);
        if (T_min < 1 * TIME_SCALE) {
          saturated = 1;
        }
      }
    }
  } else {
    // no streams yet: keep period at the current value
    return;
  }
#if JOINING_NODES && DYNAMIC_FREE_SLOTS
//  if (time < 180 * TIME_SCALE) {
//    period = INIT_PERIOD;
//    return;
//  }
  if (!IS_LESS_EQ_THAN_TIME(last_free_reqs_time + T_LAST_FREE_REQS)) {
    // we have received stream requests in the last T_LAST_FREE_REQS seconds:
    // set the period to a low value
    period = 2;
    return;
  }
#endif /* JOINING_NODES && DYNAMIC_FREE_SLOTS */
  if (T_min <= PERIOD_MAX * TIME_SCALE) {
    if (saturated) {
      // T_min < 1 * TIME_SCALE
      // TODO: notify about insufficient bandwidth
      period = INIT_PERIOD;
    } else {
      // T_min between [1, PERIOD_MAX] * TIME_SCALE
      period = T_min / TIME_SCALE;
    }
  } else {
    // T_min > PERIOD_MAX
    period = PERIOD_MAX;
  }
#endif /* MINIMIZE_LATENCY */
}
#endif /* DYNAMIC_PERIOD */

//--------------------------------------------------------------------------------------------------
void compute_schedule(void) {

  t_sched_start = RTIMER_NOW();
  leds_on(LEDS_ALL);
  // store information about the last schedule
  old_sched = sched;
  // clear the content of the last schedule
  memset((uint8_t *)&sched.slot, 0, sizeof(sched.slot));
#if DYNAMIC_PERIOD
  data_ipi = 1;
  data_cnt = 0;
#if MINIMIZE_LATENCY
  min_next_ready = PERIOD_MAX;
#endif /* MINIMIZE_LATENCY */
#endif /* DYNAMIC_PERIOD */

  ack_snc = 0;
  first_snc = 0;
  // number of slots assigned in the schedule
  uint8_t n_slots_assigned = 0;
  // embed the node id of the host
  sched.host_id = node_id;

  if (stream_ack_idx) {
    // there's at least one stream ack to send: assign the first slot to the host itself
    snc_tmp[n_slots_assigned] = 0;
    snc[n_slots_assigned] = 0;
    ack_snc = 1;
    first_snc = 1;
    n_slots_assigned++;
  }
  // current stream being processed
  stream_info *curr_stream;
  for (curr_stream = list_head(streams_list); curr_stream != NULL; ) {

#if REMOVE_NODES
    if (curr_stream->n_cons_missed & 0x80) {
      curr_stream->n_cons_missed &= 0x7f;
      curr_stream->n_cons_missed++;
    }

    if (curr_stream->n_cons_missed > N_CONS_MISSED_MAX) {
      // too many consecutive slots without reception: delete this stream
      stream_info *stream_to_remove = curr_stream;
      curr_stream = curr_stream->next;
      del_stream(stream_to_remove);
    } else {
#endif /* REMOVE_NODES */

#if DYNAMIC_PERIOD
#if MINIMIZE_LATENCY
      if (curr_stream->next_ready - time < min_next_ready) {
        min_next_ready = curr_stream->next_ready - time;
      }
#else
      curr_gcd = gcd(data_ipi, curr_stream->ipi);
      uint16_t k1 = curr_stream->ipi / curr_gcd;
      uint16_t k2 = data_ipi / curr_gcd;
      data_cnt = data_cnt * k1 + k2;
      data_ipi = data_ipi * k1;
#endif /* MINIMIZE_LATENCY */
#endif /* DYNAMIC_PERIOD */
      curr_stream = curr_stream->next;
    }
  }

#if DYNAMIC_PERIOD
  t_period_start = RTIMER_NOW();
  change_period();
  t_period_stop = RTIMER_NOW();
#endif /* DYNAMIC_PERIOD */

  // increment time by the current period
  time += (period * TIME_SCALE);

#if JOINING_NODES
#if DYNAMIC_FREE_SLOTS
  uint8_t n_free = 1;
  if (IS_LESS_EQ_THAN_TIME(last_free_reqs_time + T_LAST_FREE_REQS) &&
      (!IS_LESS_EQ_THAN_TIME(last_free_slot_time + T_LAST_FREE_SLOT))) {
    // the last stream request during a free slot was received more than T_LAST_FREE_REQS ago
    // and we scheduled a free slot less than T_LAST_FREE_SLOT ago:
    // do not schedule any free slot
    n_free = 0;
  }
  if (n_free) {
    last_free_slot_time = (uint16_t)time;
  }
  N_SLOTS = SET_N_FREE(n_free);
#else
  N_SLOTS = SET_N_FREE(1);
#endif /* DYNAMIC_FREE_SLOTS */
#endif /* JOINING_NODES */
  if (n_streams == 0) {
    // no streams to process
    goto set_schedule;
  }

  // random initial position in the list
  uint16_t rand_init_pos = 0; //random_rand() % n_streams;
  uint16_t i;
  curr_stream = list_head(streams_list);
  // make curr_stream point to the random initial position
  for (i = 0; i < rand_init_pos; i++) {
    curr_stream = curr_stream->next;
  }
  // initial stream being processed
  stream_info *init_stream = curr_stream;
  do {
    // compute how many (new) slots have to be assigned for this stream
    if (IS_LESS_EQ_THAN_TIME(curr_stream->next_ready)) {
      uint16_t n_new_slots = ((uint16_t)time - curr_stream->next_ready) / curr_stream->ipi + 1;
      n_slots_to_assign += n_new_slots;
      curr_stream->next_ready += n_new_slots * curr_stream->ipi;
    }

    // assign slots for this stream, if possible
    if ((n_slots_assigned < N_SLOTS_MAX) &&
        IS_LESS_EQ_THAN_TIME(curr_stream->last_assigned + curr_stream->ipi)) {
#if DYNAMIC_PERIOD
      uint16_t to_assign = ((uint16_t)time - curr_stream->last_assigned) / curr_stream->ipi;
      if (saturated) {
        if (curr_stream->next == init_stream || (curr_stream->next == NULL && rand_init_pos == 0)) {
          // last random stream: assign all possible slots
        } else {
          // ensure fairness among source nodes when the bandwidth saturates
          // assigned a number of slots proportional to (1/IPI)
          uint16_t slots_ipi = T_min / curr_stream->ipi;
          if (to_assign > slots_ipi) {
            to_assign = slots_ipi;
            if (to_assign == 0 && curr_stream == init_stream) {
              // first random stream: assign one slot to it, even if it has very long IPI
              to_assign = 1;
            }
          }
        }
      }
#else
      uint16_t to_assign = ((uint16_t)time - curr_stream->last_assigned) / curr_stream->ipi;
#endif /* DYNAMIC_PERIOD */
      if (to_assign > N_SLOTS_MAX - n_slots_assigned) {
        to_assign = N_SLOTS_MAX - n_slots_assigned;
      }
      uint8_t last = n_slots_assigned + to_assign;

      for (; n_slots_assigned < last; n_slots_assigned++) {
        snc_tmp[n_slots_assigned] = curr_stream->node_id;

#if REMOVE_NODES
        streams[n_slots_assigned] = curr_stream;
#endif /* REMOVE_NODES */
      }
//      n_slots_assigned += to_assign;
      n_slots_to_assign -= to_assign;
      curr_stream->last_assigned += to_assign * curr_stream->ipi;
#if REMOVE_NODES
      curr_stream->n_cons_missed |= 0x80;
#endif /* REMOVE_NODES */
    }

    // go to the next stream in the list
    curr_stream = curr_stream->next;
    if (curr_stream == NULL) {
      // end of the list: start again from the head of the list
      curr_stream = list_head(streams_list);
      first_snc = n_slots_assigned;
    }
  } while (curr_stream != init_stream);

set_schedule:
  // embed the number of assigned slots into the schedule

  memcpy(&snc[ack_snc], &snc_tmp[first_snc], (n_slots_assigned - first_snc) * 2);
  memcpy(&snc[ack_snc + n_slots_assigned - first_snc], &snc_tmp[ack_snc], (first_snc - ack_snc) * 2);
  N_SLOTS |= GET_N_SLOTS(n_slots_assigned);

  compress_schedule(GET_N_SLOTS(N_SLOTS));

#if TWO_SCHEDS
  if (period != 1) {
    // this schedule is sent at the end of a round: do not communicate
    PERIOD = period | SET_COMM(0);
    TIME = (uint16_t)time - (period - 1) * TIME_SCALE;
  } else {
    // T = 1: there are no schedules sent at the end of a round, always communicate
    PERIOD = period | SET_COMM(1);
    TIME = (uint16_t)time;
  }
#else
  // always communicate
  PERIOD = GET_PERIOD(period);
  PERIOD |= SET_COMM(1);
#endif /* TWO_SCHEDS */
  t_sched_stop = RTIMER_NOW();
  leds_off(LEDS_ALL);
}

//--------------------------------------------------------------------------------------------------
void scheduler_init(void) {
  // initialize streams member and list
  memb_init(&streams_memb);
  list_init(streams_list);

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

#if JOINING_NODES
  n_streams = 0;
#else
  n_streams = sizeof(nodes) / 2;
  for (idx = 0; idx < n_streams; idx++) {
    stream_info *curr_stream = memb_alloc(&streams_memb);
    curr_stream->node_id = nodes[idx];
    curr_stream->ipi = INIT_IPI;
    curr_stream->last_assigned = (uint16_t)(time + INIT_DELAY * TIME_SCALE - curr_stream->ipi);
    curr_stream->next_ready = (uint16_t)(time + INIT_DELAY * TIME_SCALE);
    list_add(streams_list, curr_stream);
  }
#endif /* JOINING_NODES */

  compute_schedule();
}
