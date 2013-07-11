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

static uint8_t pkt_len;
static uint8_t pkt[PKT_LEN_MAX];
static uint8_t pkt_type;
static uint16_t curr_ipi;

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

void prepare_data_packet(uint8_t free_slot) {
  data_header.node_id = node_id;
#if STATIC
  if (relay_cnt_cnt >= AVG_RELAY_CNT_UPDATE / 2) {
    data_header.relay_cnt = (relay_cnt_sum * RELAY_CNT_FACTOR) / relay_cnt_cnt;
  } else {
    data_header.relay_cnt = RELAY_CNT_INVALID;
  }
#endif /* STATIC */
  pkt_type = SET_N_STREAM_REQS(n_stream_reqs);
  if (!free_slot) {
    pkt_type |= DATA;
    data.en_on = energest_type_time(ENERGEST_TYPE_LISTEN) + energest_type_time(ENERGEST_TYPE_TRANSMIT);

    data.en_tot = energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM);

#if LATENCY
    while (IS_LESS_EQ_THAN_TIME(data.gen_time + curr_ipi)) {
      data.gen_time += curr_ipi;
    }
#endif /* LATENCY */


#if LWB_DEBUG && !COOJA
    data.relay_cnt = (relay_cnt_cnt) ?
        ((relay_cnt_sum * RELAY_CNT_FACTOR) / relay_cnt_cnt) : (RELAY_CNT_INVALID);
#if CONTROL_DC
    data.en_control = en_control;
#endif /* CONTROL_DC */
#endif /* LWB_DEBUG && !COOJA */
  }

#if FAIRNESS
  stream_reqs[0].time_info = TIME + stream_reqs[0].ipi;
#endif /* FAIRNESS */
  uint8_t stream_reqs_len = GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH;
  uint8_t data_len = IS_THERE_DATA(pkt_type) * DATA_LENGTH;
  pkt_len = DATA_HEADER_LENGTH + stream_reqs_len + data_len;
  memcpy(pkt, &data_header, DATA_HEADER_LENGTH);
  memcpy(pkt + DATA_HEADER_LENGTH, stream_reqs, stream_reqs_len);
  memcpy(pkt + DATA_HEADER_LENGTH + stream_reqs_len, &data, data_len);
}

//--------------------------------------------------------------------------------------------------
void check_for_stream_requests(void) {

  if (GET_N_STREAM_REQS(pkt_type)) {

#if JOINING_NODES
#if DYNAMIC_FREE_SLOTS
    last_free_reqs_time = (uint16_t)time;
#endif /* DYNAMIC_FREE_SLOTS */
#endif /* JOINING_NODES */

    // check if we already processed stream requests from this node during this round
    uint8_t already_processed = 0;
    for (idx = 0; idx < stream_ack_idx; idx++) {
      if (stream_ack[idx] == data_header.node_id) {
        // if so, do not process them again
        already_processed = 1;
        break;
      }
    }
    if (!already_processed) {
      // cycle through the stream requests and process them
      for (idx = 0; idx < GET_N_STREAM_REQS(pkt_type) && stream_ack_idx < PKT_LEN_MAX - DATA_HEADER_LENGTH; idx++) {
        process_stream_req(data_header.node_id, stream_reqs[idx]);
        // prepare the stream acknowledgment for this node
        stream_ack[stream_ack_idx] = data_header.node_id;
        stream_ack_idx++;
      }
    }

  }

}

//--------------------------------------------------------------------------------------------------

PT_THREAD(g_rr_host(struct rtimer *t, void *ptr)) {
  PT_BEGIN(&pt_rr);

  while (1) {

    for (slot_idx = 0; slot_idx < GET_N_SLOTS(N_SLOTS); slot_idx++) {

      leds_off(LEDS_ALL);
      SCHEDULE(T_REF, T_SYNC_ON + 3 * T_GAP + slot_idx * (T_RR_ON + T_GAP), g_rr_host);
      PT_YIELD(&pt_rr);

       leds_on(slot_idx + 1);

      if (snc[slot_idx] == 0) {
        // We have stream ack to send
        data_header.node_id = node_id;
        pkt_type = STREAM_ACK;
        uint8_t stream_ack_len = stream_ack_idx * STREAM_ACK_LENGTH;
        pkt_len = DATA_HEADER_LENGTH + STREAM_ACK_IDX_LENGTH + stream_ack_len;
        memcpy(pkt, &data_header, DATA_HEADER_LENGTH);
        memcpy(pkt + DATA_HEADER_LENGTH, &stream_ack_idx, STREAM_ACK_IDX_LENGTH);
        memcpy(pkt + DATA_HEADER_LENGTH + STREAM_ACK_IDX_LENGTH, stream_ack, stream_ack_len);
        save_energest_values();

        glossy_start((uint8_t *)pkt,
                     pkt_len,
                     GLOSSY_INITIATOR,
                     GLOSSY_NO_SYNC,
                     N_RR,
                     pkt_type,
                     T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 2 * T_GAP,
                     (rtimer_callback_t)g_rr_host, &rt, NULL);
      } else {

        glossy_start((uint8_t *)pkt,
                     0,
                     GLOSSY_RECEIVER,
                     GLOSSY_NO_SYNC,
                     N_RR,
                     0,
                     T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 2 * T_GAP,
                     (rtimer_callback_t)g_rr_host, &rt, NULL);
      }

      PT_YIELD(&pt_rr);

      uint8_t rcvd = !!glossy_stop();

      if (snc[slot_idx] == 0) {
        // we just sent a stream ack
        update_control_dc();
        memset(stream_ack, 0, STREAM_ACK_LENGTH);
        stream_ack_idx = 0;

      } else {
        // Probably, this packet is a data packet
        uint16_t id = snc[slot_idx];
        n_tot[id]++;
        // get the data header
        memcpy(&data_header, pkt, DATA_HEADER_LENGTH);
        pkt_type = get_header();

        if (rcvd && (id == data_header.node_id)) {
          if (is_sink) {
            n_rcvd[id]++;
          }

          /// @note Kasun: I don't understand this
#if REMOVE_NODES
          streams[((first_snc + slot_idx - 2 * ack_snc) % (GET_N_SLOTS(N_SLOTS) - ack_snc)) + ack_snc]->n_cons_missed = 0;
#endif /* REMOVE_NODES */

          // Stream requests can be piggybacked on data packets
          // So, we copy any available stream requests to stream_reqs.
          memcpy(stream_reqs, pkt + DATA_HEADER_LENGTH, GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH);
          check_for_stream_requests();

          // Check whether we actually have data in the packet
          if (IS_THERE_DATA(pkt_type) && is_sink) {
            // We have data.
            memcpy(&data, pkt + DATA_HEADER_LENGTH + GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH, IS_THERE_DATA(pkt_type) * DATA_LENGTH);

            dc[id] = (uint16_t)((10000LLU * data.en_on) / data.en_tot);

#if LATENCY
            lat[id] += (TIME - data.gen_time);
#endif /* LATENCY */

#if LWB_DEBUG && !COOJA
            rc[id] = data.relay_cnt;
#if CONTROL_DC
            dc_control[id] = (uint16_t)((10000LLU * data.en_control) / data.en_tot);
#endif /* CONTROL_DC */
#endif /* LWB_DEBUG && !COOJA */
          }
        } else {
        }

      }
    }

    // Contention slots. Now, we are receiving stream requests from other nodes.
    for (; slot_idx < GET_N_SLOTS(N_SLOTS) + GET_N_FREE(N_SLOTS); slot_idx++) {

      leds_off(LEDS_ALL);
      SCHEDULE(T_REF, T_SYNC_ON + 3 * T_GAP + slot_idx * (T_RR_ON + T_GAP), g_rr_host);
      PT_YIELD(&pt_rr);

      leds_on(slot_idx + 1);
      save_energest_values();
      glossy_start((uint8_t *)pkt,
                   0,
                   GLOSSY_RECEIVER,
                   GLOSSY_NO_SYNC,
                   N_RR,
                   0,
                   T_REF + T_SYNC_ON + (slot_idx) * (T_RR_ON + T_GAP) + 3 * T_GAP + T_FREE_ON,
                   (rtimer_callback_t)g_rr_host, &rt, NULL);
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

#if (LWB_DEBUG && (FAIRNESS || COOJA))
    process_poll(&print);
#endif /* LWB_DEBUG && (FAIRNESS || COOJA) */

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


//--------------------------------------------------------------------------------------------------
PT_THREAD(g_rr_source(struct rtimer *t, void *ptr)) {
  PT_BEGIN(&pt_rr);

  while (1) {

    // Iterate through all the data slots in the round.
    for (slot_idx = 0; slot_idx < GET_N_SLOTS(N_SLOTS); slot_idx++) {

      leds_off(LEDS_ALL);
      SCHEDULE(T_REF, T_SYNC_ON + 3 * T_GAP + slot_idx * (T_RR_ON + T_GAP), g_rr_source);
      PT_YIELD(&pt_rr);

      leds_on(slot_idx + 1);

      if (snc[slot_idx] == node_id) {
        // This is our slot. So we start Glossy as an initiator.
        prepare_data_packet(0);
        glossy_start((uint8_t *)pkt,
                     pkt_len,
                     GLOSSY_INITIATOR,
                     GLOSSY_NO_SYNC,
                     N_RR,
                     pkt_type,
                     T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 2 * T_GAP,
                     (rtimer_callback_t)g_rr_source, &rt, NULL);

        PT_YIELD(&pt_rr);
        glossy_stop();
      } else {
        // This is not our slot. We just participate to the Glossy flooding.
          save_energest_values();
          glossy_start((uint8_t *)pkt,
                       0,
                       GLOSSY_RECEIVER,
                       GLOSSY_NO_SYNC,
                       N_RR,
                       0,
                       T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 2 * T_GAP,
                       (rtimer_callback_t)g_rr_source, &rt, NULL);

          PT_YIELD(&pt_rr);

          if (glossy_stop()) {
            // We successfully received Glossy flooding.
            memcpy(&data_header, pkt, DATA_HEADER_LENGTH);
            pkt_type = get_header();
            if (IS_STREAM_ACK(pkt_type)) {
              // This packet is an stream acknowledgment.
              update_control_dc();
              memcpy(&stream_ack_idx, pkt + DATA_HEADER_LENGTH, STREAM_ACK_IDX_LENGTH);
              memcpy(stream_ack, pkt + DATA_HEADER_LENGTH + STREAM_ACK_IDX_LENGTH, stream_ack_idx * STREAM_ACK_LENGTH);
              for (idx = 0; idx < stream_ack_idx; idx++) {
                if (stream_ack[idx] == node_id) {
                  n_stream_reqs--;
                  if (n_stream_reqs == 0) {
                    // All the stream requests are acknowledged.
                    // Horray, we joined
                    joining_state = JOINED;
                  }
                  curr_ipi = stream_reqs[0].ipi;
                }
              }
            } else if (IS_THERE_DATA(pkt_type)){
              // This is a data packet
              /// @todo Add this data packet to a queue and poll TCPIP process
                uint16_t id = data_header.node_id;
//
//              if (id == snc[slot_idx]) {
//                if (is_sink) {
//                  // packet correctly received from a source node
                n_rcvd[id]++;
//
//                }
//                if (IS_THERE_DATA(pkt_type) && is_sink) {
//                  memcpy(&data, pkt + DATA_HEADER_LENGTH + GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH, IS_THERE_DATA(pkt_type) * DATA_LENGTH);
//
//                  dc[id] = (uint16_t)((10000LLU * data.en_on) / data.en_tot);
//
//                }
//              }
            }
          } else {
            // We haven't received Glossy flooding.
          }
      }
    }
    // We've iterated through all the data/ack slots. Now it is time for contention slots
    for (; slot_idx < GET_N_SLOTS(N_SLOTS) + GET_N_FREE(N_SLOTS); slot_idx++) {
      leds_off(LEDS_ALL);
      // schedule Glossy for next slot.
      SCHEDULE(T_REF, T_SYNC_ON + 3 * T_GAP + slot_idx * (T_RR_ON + T_GAP), g_rr_source);
      PT_YIELD(&pt_rr);

      leds_on(slot_idx + 1);
      if (joining_state == JOINING && (sync_state == SYNCED || sync_state == QUASI_SYNCED) && is_source) {
        n_stream_reqs = 1;
        if (rounds_to_wait == 0) {
          // try to join
          joining_state = JUST_TRIED;
          prepare_data_packet(1);
          save_energest_values();
          glossy_start((uint8_t *)pkt,
                       pkt_len,
                       GLOSSY_INITIATOR,
                       GLOSSY_NO_SYNC,
                       N_RR,
                       pkt_type,
                       T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 2 * T_GAP,
                       (rtimer_callback_t)g_rr_source, &rt, NULL);
        } else {
          // keep waiting, decrease the number of rounds to wait
          rounds_to_wait--;
          save_energest_values();
          glossy_start((uint8_t *)pkt,
                       0,
                       GLOSSY_RECEIVER,
                       GLOSSY_NO_SYNC,
                       N_RR,
                       0,
                       T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 2 * T_GAP,
                       (rtimer_callback_t)g_rr_source, &rt, NULL);
        }
      } else {
        // already joined or not SYNCED
        save_energest_values();
        glossy_start((uint8_t *)pkt,
                     0,
                     GLOSSY_RECEIVER,
                     GLOSSY_NO_SYNC,
                     N_RR,
                     0,
                     T_REF + T_SYNC_ON + (slot_idx) * (T_RR_ON + T_GAP) + 3 * T_GAP + T_FREE_ON,
                     (rtimer_callback_t)g_rr_source, &rt, NULL);
      }
      PT_YIELD(&pt_rr);

      glossy_stop();
      update_control_dc();
    }
#if LWB_DEBUG
    if (slot_idx < 30) {
      process_poll(&print);
    }
#endif /* LWB_DEBUG */

    // schedule g_sync in 1 second
    SCHEDULE(T_REF, PERIOD_RT(1) - T_guard, g_sync);
    SYNC_LEDS();

#if FAIRNESS
    if (is_source && time >= (uint32_t)INIT_DELAY * TIME_SCALE) {
      if ((TIME - t_last_req) / TIME_SCALE >= 300) {
        // send a new request every 5 minutes
        n_stream_reqs = 1;
        stream_reqs[0].req_type = SET_STREAM_ID(1) | (REP_STREAM & 0x3);
#if JOINING_NODES
        joining_state = NOT_JOINED;
#endif /* JOINING_NODES */
        if ((time >= 2400LU * TIME_SCALE) && (node_id == 33)) {
          stream_reqs[0].ipi = 1;
        } else {
          if ((time >= 4200LU * TIME_SCALE) && (node_id == 12 || node_id == 14 || node_id == 15 || node_id == 29)) {
            stream_reqs[0].ipi = 1;
          } else {
            if (time >= 6000LU * TIME_SCALE) {
              stream_reqs[0].ipi = 1;
            } else {
              stream_reqs[0].ipi = INIT_IPI;
            }
          }
        }
        t_last_req = TIME;
      }
    }
#endif /* FAIRNESS */

    // copy the current schedule to old_sched
    old_sched = sched;
    // erase the schedule before the receiving the next one
    memset((uint8_t *)&sched.slot, 0, sizeof(sched.slot));

    PT_YIELD(&pt_rr);
  }

  PT_END(&pt_rr);
}

