/*
 * g_rr.c
 *
 *  Created on: Aug 3, 2011
 *      Author: fe
 */

//#define PKT_LEN_MAX (DATA_HEADER_LENGTH + N_STREAM_REQS_MAX * STREAM_REQ_LENGTH + DATA_LENGTH)
#define PKT_LEN_MAX 102

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

//--------------------------------------------------------------------------------------------------
void prepare_data_packet() {

//  data_header.node_id = node_id;
//  pkt_type = SET_N_STREAM_REQS(n_stream_reqs);
//  pkt_type |= DATA;
//
//  uint8_t stream_reqs_len = GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH;
//  uint8_t data_len = IS_THERE_DATA(pkt_type) * DATA_LENGTH;
//  pkt_len = DATA_HEADER_LENGTH + stream_reqs_len + data_len;
//  memcpy(pkt, &data_header, DATA_HEADER_LENGTH);
//  memcpy(pkt + DATA_HEADER_LENGTH, stream_reqs, stream_reqs_len);
//  memcpy(pkt + DATA_HEADER_LENGTH + stream_reqs_len, &data, data_len);

  // At the moment, we do not send stream request packets along with data packets.
  data_header.node_id = node_id;
  pkt_type = DATA;
  memcpy(pkt, &data_header, DATA_HEADER_LENGTH);
  pkt_len = DATA_HEADER_LENGTH;

  data_buf_t* p_buf = list_head(tx_buffer_list);
  if (p_buf) {
    ui8_tx_buf_element_count--;
    memcpy(pkt + DATA_HEADER_LENGTH, p_buf->data, p_buf->ui8_data_size);
    pkt_len += p_buf->ui8_data_size;
    list_remove(tx_buffer_list, p_buf);
    memb_free(&tx_buffer_memb, p_buf);
  } else{
    // We don't have data to send. So only the data_header will be send (data length = 0)
  }
}

//--------------------------------------------------------------------------------------------------
void prepare_stream_reqs_packet() {

  pkt_type = SET_N_STREAM_REQS(n_stream_reqs); // set number of stream requests
  uint8_t stream_reqs_len = GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH;

  data_header.node_id = node_id;
  pkt_len = DATA_HEADER_LENGTH + stream_reqs_len;
  memcpy(pkt, &data_header, DATA_HEADER_LENGTH);
  memcpy(pkt + DATA_HEADER_LENGTH, stream_reqs, stream_reqs_len); // Copy the stream requests
}

//--------------------------------------------------------------------------------------------------
void inline receive_data_packet() {
  uint8_t offest = DATA_HEADER_LENGTH + (GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH);
  uint8_t len = get_data_len() - offest;

  if (len == 0) {
    // We don't have data in the packet. Skipping.
    return;
  }

  data_buf_t* p_buf = memb_alloc(&rx_buffer_memb);
  if (p_buf) {
    memcpy(p_buf->data, pkt + offest, len);
    p_buf->ui8_data_size = len;
    list_add(rx_buffer_list, p_buf);
    ui8_rx_buf_element_count++;
    process_poll(&lwb_init);

  } else {
    buf_stats.ui16_rx_overflow++;
  }
}

//--------------------------------------------------------------------------------------------------
void inline process_stream_ack() {
  update_control_dc();
  // Copy the length of stream acks.
  memcpy(&stream_ack_idx, pkt + DATA_HEADER_LENGTH, STREAM_ACK_IDX_LENGTH);
  memcpy(stream_ack, pkt + DATA_HEADER_LENGTH + STREAM_ACK_IDX_LENGTH, stream_ack_idx * STREAM_ACK_LENGTH);
  // Need to check whether my stream requests are acknowledged.
  for (idx = 0; idx < stream_ack_idx; idx++) {
    if (stream_ack[idx] == node_id) {
      // The host has acknowledged a stream request.
      n_stream_reqs--;
      if (n_stream_reqs == 0) {
        // All the stream requests are acknowledged.
        joining_state = JOINED;
      } else {
        // we have some stream requests to be acknowledged.
      }
      curr_ipi = stream_reqs[0].ipi;
    } else {
      // The ACK for some other node.
    }
  }
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
    // we allow only one stream request per round from a node.
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

  // The first slot starts after 3 * T_GAP from the end of Glossy phase for schedule.
  // starting time of a slot = T_REF + (3 * T_GAP) + (slot_idx * (T_RR_ON + T_GAP))
  // end time of a slot      = T_REF + T_SYNC_ON + (3 * T_GAP) + (slot_idx * (T_RR_ON + T_GAP)) + T_RR_ON

  while (1) {

    for (slot_idx = 0; slot_idx < GET_N_SLOTS(N_SLOTS); slot_idx++) {

      leds_off(LEDS_ALL);
      SCHEDULE(T_REF, T_SYNC_ON + (3 * T_GAP) + (slot_idx * (T_RR_ON + T_GAP)), g_rr_host);
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
                     T_REF + T_SYNC_ON + (3 * T_GAP) + (slot_idx * (T_RR_ON + T_GAP)) + T_RR_ON,
                     (rtimer_callback_t)g_rr_host,
                     &rt,
                     NULL);
      } else {

        glossy_start((uint8_t *)pkt,
                     0,
                     GLOSSY_RECEIVER,
                     GLOSSY_NO_SYNC,
                     N_RR,
                     0,
                     T_REF + T_SYNC_ON + (3 * T_GAP) + (slot_idx * (T_RR_ON + T_GAP)) + T_RR_ON,
                     (rtimer_callback_t)g_rr_host,
                     &rt,
                     NULL);
      }

      PT_YIELD(&pt_rr);

      uint8_t rcvd = !!glossy_stop();

      if (snc[slot_idx] == 0) {
        // we just sent stream acks
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
            n_rcvd[id]++;

          /// @note Kasun: I don't understand this
#if REMOVE_NODES
          streams[((first_snc + slot_idx - 2 * ack_snc) % (GET_N_SLOTS(N_SLOTS) - ack_snc)) + ack_snc]->n_cons_missed = 0;
#endif /* REMOVE_NODES */

          // Stream requests can be piggybacked on data packets
          // So, we copy any available stream requests to stream_reqs.
          memcpy(stream_reqs, pkt + DATA_HEADER_LENGTH, GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH);
          check_for_stream_requests();

          // Check whether we actually have data in the packet
          if (IS_THERE_DATA(pkt_type)) {
            receive_data_packet();
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
                   T_REF + T_SYNC_ON + (3 * T_GAP) + ((slot_idx) * (T_RR_ON + T_GAP)) + T_FREE_ON,
                   (rtimer_callback_t)g_rr_host,
                   &rt,
                   NULL);
      PT_YIELD(&pt_rr);

      // Check whether we have received a stream request.
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

    // By now, we should received all new stream requests.
    // It is time to compute new schedule.
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

  // The first slot starts after 3 * T_GAP from the end of Glossy phase for schedule.
  // starting time of a slot = T_REF + (3 * T_GAP) + (slot_idx * (T_RR_ON + T_GAP))
  // end time of a slot      = T_REF + T_SYNC_ON + (3 * T_GAP) + (slot_idx * (T_RR_ON + T_GAP)) + T_RR_ON
  while (1) {

    // Iterate through all the data slots in the round.
    for (slot_idx = 0; slot_idx < GET_N_SLOTS(N_SLOTS); slot_idx++) {

      leds_off(LEDS_ALL);
      SCHEDULE(T_REF, T_SYNC_ON + 3 * T_GAP + slot_idx * (T_RR_ON + T_GAP), g_rr_source);
      PT_YIELD(&pt_rr);

      leds_on(slot_idx + 1);

      if (snc[slot_idx] == node_id) {
        // This is our slot. So we start Glossy as an initiator.
        prepare_data_packet();
        glossy_start((uint8_t *)pkt,
                     pkt_len,
                     GLOSSY_INITIATOR,
                     GLOSSY_NO_SYNC,
                     N_RR,
                     pkt_type,
                     T_REF + T_SYNC_ON + (3 * T_GAP) + (slot_idx * (T_RR_ON + T_GAP)) + T_RR_ON,
                     (rtimer_callback_t)g_rr_source,
                     &rt,
                     NULL);

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
                       T_REF + T_SYNC_ON + (3 * T_GAP) + (slot_idx * (T_RR_ON + T_GAP)) + T_RR_ON,
                       (rtimer_callback_t)g_rr_source,
                       &rt,
                       NULL);

          PT_YIELD(&pt_rr);

          if (glossy_stop()) {
            // We successfully received Glossy flooding.
            memcpy(&data_header, pkt, DATA_HEADER_LENGTH);
            uint16_t id = snc[slot_idx];
            pkt_type = get_header();

            if (IS_STREAM_ACK(pkt_type)) {
              // This packet is an stream acknowledgment.
              process_stream_ack();

            } else if (IS_THERE_DATA(pkt_type)){
              // This is a data packet
              n_rcvd[id]++;
              receive_data_packet();

            } else {
              // Unknown packet
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
      SCHEDULE(T_REF, T_SYNC_ON + (3 * T_GAP) + (slot_idx * (T_RR_ON + T_GAP)), g_rr_source);
      PT_YIELD(&pt_rr);

      leds_on(slot_idx + 1);
      if (joining_state == JOINING && (sync_state == SYNCED || sync_state == QUASI_SYNCED) && n_stream_reqs > 0) {
        // We are synchronized and have stream requests to send.
        if (rounds_to_wait == 0) {
          // try to join
          prepare_stream_reqs_packet();
          save_energest_values();
          glossy_start((uint8_t *)pkt,
                       pkt_len,
                       GLOSSY_INITIATOR,
                       GLOSSY_NO_SYNC,
                       N_RR,
                       pkt_type,
                       //T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 2 * T_GAP,
                       T_REF + T_SYNC_ON + (3 * T_GAP) + (slot_idx * (T_RR_ON + T_GAP)) + T_FREE_ON,
                       (rtimer_callback_t)g_rr_source,
                       &rt,
                       NULL);

          joining_state = JUST_TRIED;
          n_joining_trials++;
          rounds_to_wait = random_rand() % (1 << (n_joining_trials % 4));

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
                       //T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 2 * T_GAP,
                       T_REF + T_SYNC_ON + (3 * T_GAP) + (slot_idx * (T_RR_ON + T_GAP)) + T_FREE_ON,
                       (rtimer_callback_t)g_rr_source,
                       &rt,
                       NULL);

        }

      } else {
        // We are not synchronized or we don't have stream requests to send.
        save_energest_values();
        glossy_start((uint8_t *)pkt,
                     0,
                     GLOSSY_RECEIVER,
                     GLOSSY_NO_SYNC,
                     N_RR,
                     0,
                     T_REF + T_SYNC_ON + (3 * T_GAP) + (slot_idx * (T_RR_ON + T_GAP)) + T_FREE_ON,
                     (rtimer_callback_t)g_rr_source,
                     &rt,
                     NULL);
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
    /// @note I don't understand why T_REF is used here.
    ///       T_REF is updated when a schedule is received.
    ///       Does this mean g_sync is scheduled to be run in 1 seconds from the time when the
    ///       schedule is received from the host?
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

