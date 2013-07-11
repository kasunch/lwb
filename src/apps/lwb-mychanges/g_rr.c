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
#if DSN_REMOVE_NODES
    data.en_tot = energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM) - en_on_to_remove;
#else
    data.en_tot = energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM);
#endif /* DSN_REMOVE_NODES */
#if USERINT_INT
    data.low_ipi = low_ipi;
    data.relay_cnt = (relay_cnt_cnt) ? ((relay_cnt_sum * RELAY_CNT_FACTOR) / relay_cnt_cnt) : (RELAY_CNT_INVALID);
#endif /* USERINT_INT */
#if LATENCY
    while (IS_LESS_EQ_THAN_TIME(data.gen_time + curr_ipi)) {
      data.gen_time += curr_ipi;
    }
#endif /* LATENCY */
#if MOBILE || USERINT_INT
    data.n_rcvd_tot = my_n_rcvd_tot_last_round;
#endif /* MOBILE */

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

PT_THREAD(g_rr_host(struct rtimer *t, void *ptr)) {
  PT_BEGIN(&pt_rr);

  while (1) {
#ifdef DSN_BOOTSTRAPPING
    memset(&slots, 0, sizeof(slots));
    slots_idx = 0;
    slots_time = time;
#endif /* DSN_BOOTSTRAPPING */
    for (slot_idx = 0; slot_idx < GET_N_SLOTS(N_SLOTS); slot_idx++) {
      leds_off(LEDS_ALL);
      SCHEDULE(T_REF, T_SYNC_ON + 3 * T_GAP + slot_idx * (T_RR_ON + T_GAP), g_rr_host);
      PT_YIELD(&pt_rr);

       leds_on(slot_idx + 1);
#if COMPRESS
      if (snc[slot_idx] == 0) {
#else
      if (get_node_id_from_schedule(sched.slot, slot_idx + 1) == node_id) {
#endif /* COMPRESS */
#ifdef DSN_BOOTSTRAPPING
        slots[slots_idx].id = stream_ack[0];
        slots[slots_idx].type = 0;
        slots_idx++;
#endif /* DSN_BOOTSTRAPPING */
        data_header.node_id = node_id;
        pkt_type = STREAM_ACK;
        uint8_t stream_ack_len = stream_ack_idx * STREAM_ACK_LENGTH;
        pkt_len = DATA_HEADER_LENGTH + STREAM_ACK_IDX_LENGTH + stream_ack_len;
        memcpy(pkt, &data_header, DATA_HEADER_LENGTH);
        memcpy(pkt + DATA_HEADER_LENGTH, &stream_ack_idx, STREAM_ACK_IDX_LENGTH);
        memcpy(pkt + DATA_HEADER_LENGTH + STREAM_ACK_IDX_LENGTH, stream_ack, stream_ack_len);
        save_energest_values();
        glossy_start((uint8_t *)pkt, pkt_len, GLOSSY_INITIATOR, GLOSSY_NO_SYNC, N_RR, pkt_type,
            T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 2 * T_GAP, (rtimer_callback_t)g_rr_host, &rt, NULL);
      } else {
        glossy_start((uint8_t *)pkt, 0, GLOSSY_RECEIVER, GLOSSY_NO_SYNC, N_RR, 0,
            T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 2 * T_GAP, (rtimer_callback_t)g_rr_host, &rt, NULL);
      }
      PT_YIELD(&pt_rr);

      uint8_t rcvd = !!glossy_stop();
#if COMPRESS
      if (snc[slot_idx] == 0) {
#else
      if (get_node_id_from_schedule(sched.slot, slot_idx + 1) == node_id) {
#endif /* COMPRESS */
        update_control_dc();
        memset(stream_ack, 0, STREAM_ACK_LENGTH);
        stream_ack_idx = 0;
      } else {
#if COMPRESS
        uint16_t id = snc[slot_idx];
#else
        uint16_t id = get_node_id_from_schedule(sched.slot, slot_idx + 1);
#endif /* COMPRESS */
        n_tot[id]++;
#ifdef DSN_BOOTSTRAPPING
        slots[slots_idx].id = id;
        slots[slots_idx].type = 1;
        slots_idx++;
#endif /* DSN_BOOTSTRAPPING */

        // get the data header
        memcpy(&data_header, pkt, DATA_HEADER_LENGTH);
        pkt_type = get_header();

        if (rcvd && (id == data_header.node_id)) {
          if (is_sink) {
            n_rcvd[id]++;
#if MOBILE || USERINT_INT
            my_n_rcvd_tot++;
#endif /* MOBILE */
          }
#if REMOVE_NODES
#if COMPRESS
          streams[((first_snc + slot_idx - 2 * ack_snc) % (GET_N_SLOTS(N_SLOTS) - ack_snc)) + ack_snc]->n_cons_missed = 0;
#else
          streams[slot_idx + 1]->n_cons_missed = 0;
#endif /* COMPRESS */
#endif /* REMOVE_NODES */
          memcpy(stream_reqs, pkt + DATA_HEADER_LENGTH, GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH);
          check_for_stream_requests();
          if (IS_THERE_DATA(pkt_type) && is_sink) {
            memcpy(&data, pkt + DATA_HEADER_LENGTH + GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH, IS_THERE_DATA(pkt_type) * DATA_LENGTH);
#if DSN_TRAFFIC_FLUCTUATIONS || DSN_INTERFERENCE || DSN_REMOVE_NODES || USERINT_INT
            en_on[id] = data.en_on;
            en_tot[id] = data.en_tot;
            rc[id] = data.relay_cnt;
#else
            dc[id] = (uint16_t)((10000LLU * data.en_on) / data.en_tot);
#endif /* DSN_TRAFFIC_FLUCTUATIONS */
#if LATENCY
            lat[id] += (TIME - data.gen_time);
#endif /* LATENCY */
#if MOBILE || USERINT_INT
            n_rcvd_tot[id] = data.n_rcvd_tot;
#endif /* MOBILE */
#if LWB_DEBUG && !COOJA
            rc[id] = data.relay_cnt;
#if CONTROL_DC
            dc_control[id] = (uint16_t)((10000LLU * data.en_control) / data.en_tot);
#endif /* CONTROL_DC */
#endif /* LWB_DEBUG && !COOJA */
          }
        } else {
        }
#if FLASH
        flash_dy.node_id = id;
        flash_dy.time = time;
        flash_dy.n_rcvd = n_rcvd[id];
        flash_dy.n_tot = n_tot[id];
        xmem_pwrite(&flash_dy, sizeof(flash_dy), FLASH_OFFSET + flash_idx * sizeof(flash_dy));
        flash_idx++;
#endif /* FLASH */
      }
    }
#if JOINING_NODES
    for (; slot_idx < GET_N_SLOTS(N_SLOTS) + GET_N_FREE(N_SLOTS); slot_idx++) {
      leds_off(LEDS_ALL);
      SCHEDULE(T_REF, T_SYNC_ON + 3 * T_GAP + slot_idx * (T_RR_ON + T_GAP), g_rr_host);
      PT_YIELD(&pt_rr);

      leds_on(slot_idx + 1);
      save_energest_values();
      glossy_start((uint8_t *)pkt, 0, GLOSSY_RECEIVER, GLOSSY_NO_SYNC, N_RR, 0,
          T_REF + T_SYNC_ON + (slot_idx) * (T_RR_ON + T_GAP) + 3 * T_GAP + T_FREE_ON, (rtimer_callback_t)g_rr_host, &rt, NULL);
      PT_YIELD(&pt_rr);

      if (glossy_stop() && ((get_data_len() - DATA_HEADER_LENGTH) % STREAM_REQ_LENGTH == 0)) {
        // get the data header
        memcpy(&data_header, pkt, DATA_HEADER_LENGTH);
        pkt_type = get_header();
        memcpy(stream_reqs, pkt + DATA_HEADER_LENGTH, GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH);
        check_for_stream_requests();
#ifdef DSN_BOOTSTRAPPING
        slots[slots_idx].id = data_header.node_id;
        slots[slots_idx].type = 2;
        slots_idx++;
#endif /* DSN_BOOTSTRAPPING */
      }
#ifdef DSN_BOOTSTRAPPING
      else {
        slots[slots_idx].id = 0;
        slots[slots_idx].type = 2;
        slots_idx++;
      }
#endif /* DSN_BOOTSTRAPPING */
      update_control_dc();
    }
#endif /* JOINING_NODES */
#if (LWB_DEBUG && (FAIRNESS || COOJA)) || KANSEI_SCALABILITY
    process_poll(&print);
#endif /* LWB_DEBUG && (FAIRNESS || COOJA) */
#if FLASH
    flash_dy.node_id = 0;
    flash_dy.time = time;
    flash_dy.n_rcvd = 0;
    flash_dy.n_tot = 0;
    xmem_pwrite(&flash_dy, sizeof(flash_dy), FLASH_OFFSET + flash_idx * sizeof(flash_dy));
    flash_idx++;
#endif /* FLASH */
    compute_schedule();
#ifdef DSN_BOOTSTRAPPING
    if (period == 1) {
      process_poll(&print);
    }
#endif /* DSN_BOOTSTRAPPING */
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
    for (slot_idx = 0; slot_idx < GET_N_SLOTS(N_SLOTS); slot_idx++) {
      leds_off(LEDS_ALL);
      SCHEDULE(T_REF, T_SYNC_ON + 3 * T_GAP + slot_idx * (T_RR_ON + T_GAP), g_rr_source);
      PT_YIELD(&pt_rr);

      leds_on(slot_idx + 1);
#if COMPRESS
      if (snc[slot_idx] == node_id) {
#else
      if (get_node_id_from_schedule(sched.slot, slot_idx + 1) == node_id) {
#endif /* COMPRESS */
        prepare_data_packet(0);
        glossy_start((uint8_t *)pkt, pkt_len, GLOSSY_INITIATOR, GLOSSY_NO_SYNC, N_RR, pkt_type,
            T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 2 * T_GAP, (rtimer_callback_t)g_rr_source, &rt, NULL);
//#if JOINING_NODES
//          joining_state = JOINED;
//#endif /* JOINING_NODES */
        PT_YIELD(&pt_rr);

        glossy_stop();
      } else {
        uint8_t participate;
#if STATIC
#if COMPRESS
        stream_info *curr_stream = get_stream_from_id(snc[slot_idx]);
#else
        stream_info *curr_stream = get_stream_from_id(get_node_id_from_schedule(sched.slot, slot_idx + 1));
#endif /* COMPRESS */
        if (curr_stream == NULL) {
          participate = 1;
        } else {
          uint8_t remote_relay_cnt = curr_stream->stream_id;
          if ((remote_relay_cnt == 0xff) || (relay_cnt_cnt <= AVG_RELAY_CNT_UPDATE / 2)) {
            participate = 1;
          } else {
            uint8_t local_relay_cnt = (relay_cnt_sum * RELAY_CNT_FACTOR) / relay_cnt_cnt;
            if (local_relay_cnt < remote_relay_cnt) {
              participate = 1;
            } else {
              if (local_relay_cnt >= remote_relay_cnt + RELAY_CNT_FACTOR) {
                participate = 0;
              } else {
                if ((random_rand() % RELAY_CNT_FACTOR) > (local_relay_cnt - remote_relay_cnt)) {
                  participate = 1;
                } else {
                  participate = 0;
                }
              }
            }
          }
        }
#else
          participate = 1;
#endif /* STATIC */
        if (participate) {
          save_energest_values();
          glossy_start((uint8_t *)pkt, 0, GLOSSY_RECEIVER, GLOSSY_NO_SYNC, N_RR, 0,
              T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 2 * T_GAP, (rtimer_callback_t)g_rr_source, &rt, NULL);
          PT_YIELD(&pt_rr);

          if (glossy_stop()) {
            // get the data header
            memcpy(&data_header, pkt, DATA_HEADER_LENGTH);
            pkt_type = get_header();
            if (IS_STREAM_ACK(pkt_type)) {
              update_control_dc();
              memcpy(&stream_ack_idx, pkt + DATA_HEADER_LENGTH, STREAM_ACK_IDX_LENGTH);
              memcpy(stream_ack, pkt + DATA_HEADER_LENGTH + STREAM_ACK_IDX_LENGTH, stream_ack_idx * STREAM_ACK_LENGTH);
              for (idx = 0; idx < stream_ack_idx; idx++) {
#if JOINING_NODES
                if (stream_ack[idx] == node_id) {
                  n_stream_reqs--;
                  if (n_stream_reqs == 0) {
                    joining_state = JOINED;
                  }
                  curr_ipi = stream_reqs[0].ipi;
#if LATENCY
                  data.gen_time = stream_reqs[0].time_info - curr_ipi;
#endif /* LATENCY */
                }
#endif /* JOINING_NODES */
              }
            } else {
              uint16_t id = data_header.node_id;
#if COMPRESS
              if (id == snc[slot_idx]) {
#else
              if (id == get_node_id_from_schedule(sched.slot, slot_idx + 1)) {
#endif /* COMPRESS */
                if (is_sink) {
                  // packet correctly received from a source node
                  n_rcvd[id]++;
#if MOBILE || USERINT_INT
                  my_n_rcvd_tot++;
#endif /* MOBILE */
                }
                if (IS_THERE_DATA(pkt_type) && is_sink) {
                  memcpy(&data, pkt + DATA_HEADER_LENGTH + GET_N_STREAM_REQS(pkt_type) * STREAM_REQ_LENGTH, IS_THERE_DATA(pkt_type) * DATA_LENGTH);
#if DSN_TRAFFIC_FLUCTUATIONS || DSN_INTERFERENCE || DSN_REMOVE_NODES || USERINT_INT
                  en_on[id] = data.en_on;
                  en_tot[id] = data.en_tot;
#if USERINT_INT
                  if (IS_MOBILE_NODE && id == USERINT_NODE) {
                    if (data.low_ipi != low_ipi && n_stream_reqs == 0) {
                      if (low_ipi) {
                        stream_reqs[n_stream_reqs].req_type = SET_STREAM_ID(2) | (DEL_STREAM & 0x3);
                        low_ipi = 0;
                      } else {
                        stream_reqs[n_stream_reqs].ipi = LOW_IPI;
                        stream_reqs[n_stream_reqs].req_type = SET_STREAM_ID(2) | (REP_STREAM & 0x3);
                        low_ipi = 1;
                      }
                      leds_toggle(LEDS_BLUE);
                      stream_reqs[n_stream_reqs].time_info = TIME;
                      n_stream_reqs++;
#if JOINING_NODES
                      joining_state = NOT_JOINED;
#endif /* JOINING_NODES */
                    }
                  }
#endif /* USERINT_INT */
#else
                  dc[id] = (uint16_t)((10000LLU * data.en_on) / data.en_tot);
#endif /* DSN_TRAFFIC_FLUCTUATIONS */
#if LWB_DEBUG && !COOJA
                  rc[id] = data.relay_cnt;
#if CONTROL_DC
                  dc_control[id] = (uint16_t)((10000LLU * data.en_control) / data.en_tot);
#endif /* CONTROL_DC */
#endif /* LWB_DEBUG && !COOJA */
#if MOBILE || USERINT_INT
                  n_rcvd_tot[id] = data.n_rcvd_tot;
#endif /* MOBILE */
                }
#if STATIC
                stream_info *curr_stream = get_stream_from_id(id);
                if (curr_stream == NULL) {
                  add_relay_cnt(id, data_header.relay_cnt);
                } else {
                  curr_stream->stream_id = data_header.relay_cnt;
                }
#endif /* STATIC */
              }
            }
          }
        }
      }
    }
#if JOINING_NODES
    for (; slot_idx < GET_N_SLOTS(N_SLOTS) + GET_N_FREE(N_SLOTS); slot_idx++) {
      leds_off(LEDS_ALL);
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
          glossy_start((uint8_t *)pkt, pkt_len, GLOSSY_INITIATOR, GLOSSY_NO_SYNC, N_RR, pkt_type,
              T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 2 * T_GAP, (rtimer_callback_t)g_rr_source, &rt, NULL);
        } else {
          // keep waiting, decrease the number of rounds to wait
          rounds_to_wait--;
          save_energest_values();
          glossy_start((uint8_t *)pkt, 0, GLOSSY_RECEIVER, GLOSSY_NO_SYNC, N_RR, 0,
              T_REF + T_SYNC_ON + (slot_idx + 1) * (T_RR_ON + T_GAP) + 2 * T_GAP, (rtimer_callback_t)g_rr_source, &rt, NULL);
        }
      } else {
        // already joined or not SYNCED
        save_energest_values();
        glossy_start((uint8_t *)pkt, 0, GLOSSY_RECEIVER, GLOSSY_NO_SYNC, N_RR, 0,
            T_REF + T_SYNC_ON + (slot_idx) * (T_RR_ON + T_GAP) + 3 * T_GAP + T_FREE_ON, (rtimer_callback_t)g_rr_source, &rt, NULL);
      }
      PT_YIELD(&pt_rr);

      glossy_stop();
      update_control_dc();
    }
#endif /* JOINING_NODES */
#if LWB_DEBUG
    if (slot_idx < 30) {
      process_poll(&print);
    }
#endif /* LWB_DEBUG */
//#if MOBILE
//    process_poll(&print);
//#endif /* MOBILE */
#if !DSN_REMOVE_NODES
    // schedule g_sync in 1 second
    SCHEDULE(T_REF, PERIOD_RT(1) - T_guard, g_sync);
    SYNC_LEDS();
#endif /* !DSN_REMOVE_NODES */

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

#if MOBILE || USERINT_INT
    my_n_rcvd_tot_last_round = my_n_rcvd_tot;
#endif /* MOBILE */
    // copy the current schedule to old_sched
    old_sched = sched;
    // erase the schedule before the receiving the next one
    memset((uint8_t *)&sched.slot, 0, sizeof(sched.slot));

#if DSN_TRAFFIC_FLUCTUATIONS
    if (is_peak_node) {
      for (idx = 0; idx < sizeof(ipis) / 2; idx++) {
        if (time >= t_change_ipi[idx] && time < t_change_ipi[idx+1]) {
          if (curr_ipi != ipis[idx]) {
            n_stream_reqs = 1;
            stream_reqs[0].req_type = SET_STREAM_ID(1) | (REP_STREAM & 0x3);
#if JOINING_NODES
            joining_state = NOT_JOINED;
#endif /* JOINING_NODES */
            stream_reqs[0].ipi = ipis[idx];
            stream_reqs[0].time_info = t_change_ipi[idx];
          }
          break;
        }
      }
    }
#endif /* DSN_TRAFFIC_FLUCTUATIONS */
#if DSN_REMOVE_NODES
    if (is_node_to_remove) {
      for (idx = 0; idx < sizeof(t_remove) / 2; idx++) {
        if (time >= t_remove[idx] && time < t_add[idx]) {
          leds_off(LEDS_ALL);
          sync_state = BOOTSTRAP;
          skew = 0;
          n_stream_reqs = 1;
          stream_reqs[0].req_type = SET_STREAM_ID(1) | (REP_STREAM & 0x3);
#if JOINING_NODES
          joining_state = NOT_JOINED;
#endif /* JOINING_NODES */
          stream_reqs[0].ipi = INIT_IPI;
          stream_reqs[0].time_info = t_add[idx];
          uint32_t T_sleep = (t_add[idx] - t_remove[idx]) + (random_rand() % period);
          SCHEDULE_L(T_REF, PERIOD_RT(T_sleep), g_sync);
          en_on_to_remove += T_sleep << 15;
          goto yield;
        }
      }
    }
    // schedule g_sync in 1 second
    SCHEDULE(T_REF, PERIOD_RT(1) - T_guard, g_sync);
    SYNC_LEDS();
yield:
#endif /* DSN_REMOVE_NODES */

    PT_YIELD(&pt_rr);
  }

  PT_END(&pt_rr);
}

