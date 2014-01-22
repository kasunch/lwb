/*
 * g_sync.c
 *
 *  Created on: Aug 3, 2011
 *      Author: fe
 */

//--------------------------------------------------------------------------------------------------
static inline void estimate_skew(void) {
  uint16_t time_diff = (TIME - last_time) / TIME_SCALE;
  int16_t skew_tmp = T_REF - last_t_ref_sync + RTIMER_SECOND * (time_diff % 2);
  // FIXME: "dirty hack" to temporary fix a Glossy bug that rarely occurs in disconnected networks
  if (skew_tmp < 500 && TIME != last_time) {
    skew = (int32_t)(64 * (int32_t)skew_tmp) / (int32_t)time_diff;
    // store the last values
    last_time = TIME;
    last_t_ref_sync = T_REF;
    t_ref_sync = T_REF;
  } else {
    sync_state = BOOTSTRAP;
    SET_UNSYNCED();
  }
}

//--------------------------------------------------------------------------------------------------
static inline void update_relay_cnt(void) {
#if AVG_RELAY_CNT_UPDATE < 2
  relay_cnt_sum = get_relay_cnt();
  relay_cnt_cnt = 1;
#else
  relay_cnt_sum += get_relay_cnt();
  relay_cnt_cnt++;

  if (relay_cnt_cnt >= AVG_RELAY_CNT_UPDATE) {
    relay_cnt_sum /= 2;
    relay_cnt_cnt /= 2;
  }
#endif /* AVG_RELAY_CNT_UPDATE */
}

//--------------------------------------------------------------------------------------------------
static inline void update_sched_stats(void) {
  if (first_schedule_rcvd) {
#if !COOJA
    // sched_bin: 1 bit [r-7, r-6, ..., r-1, r]
    sched_bin <<= 1;
    sched_bin |= IS_SYNCED();
#endif /* !COOJA */
  } else {
    if (sync_state == SYNCED) {
      first_schedule_rcvd = 1;
    }
  }
}

//--------------------------------------------------------------------------------------------------
static inline void compute_new_sync_state(void) {
  if (IS_SYNCED()) {
    // schedule received and t_ref updated
    period = GET_PERIOD(PERIOD);
    if (TIME < last_time) {
      // Current time of the host is less than the last reported time.
      // 16-bit timer overflow: increment by 1 the 16 MSB of time
      SET_TIME_HIGH(GET_TIME_HIGH() + 1);
    }
    // set the 16 LSB of time
    SET_TIME_LOW(TIME);
    if (sync_state == BOOTSTRAP) {
      if (GET_COMM(PERIOD)) {
        // exit from bootstrap state
        sync_state = QUASI_SYNCED;
        T_guard = T_GUARD_3;
        last_time = TIME;
        last_t_ref_sync = T_REF;
        t_ref_sync = T_REF;
      }
    } else {
      if (sync_state == QUASI_SYNCED && !GET_COMM(PERIOD)) {
        // We've received a schedule at the end of a round.
        // no immediate communication: remain in this state and don't synchronize
      } else {
        if (GET_COMM(PERIOD)) {
          // We've received a schedule at the beginning of a round.
          // immediate communication: go to synced and estimate skew
          sync_state = SYNCED;
          T_guard = T_GUARD;
          estimate_skew();
        } else {
          // no immediate communication: go to already_synced
          sync_state = ALREADY_SYNCED;
          T_guard = T_GUARD;
        }
      }
    }
    sched_size = get_data_len();
    update_relay_cnt();

  } else {
    // t_ref not updated
    if (sync_state == UNSYNCED_3 || sync_state == QUASI_SYNCED) {
      // go back to bootstrap
      sync_state = BOOTSTRAP;
      skew = 0;
    } else {
      // compute new state based on the current state
      if (sync_state == SYNCED || sync_state == ALREADY_SYNCED) {
        sync_state = UNSYNCED_1;
        T_guard = T_GUARD_1;
      } else {
        if (sync_state == UNSYNCED_1) {
          sync_state = UNSYNCED_2;
          T_guard = T_GUARD_2;
        } else {
          if (sync_state == UNSYNCED_2) {
            sync_state = UNSYNCED_3;
            T_guard = T_GUARD_3;
          }
        }
      }
      // compute time and period based on the old schedule
      if (period == 1) {
        // always communicate
        TIME += (1 * TIME_SCALE);
        PERIOD = 1 | SET_COMM(1);
      } else {
        PERIOD = period | SET_COMM(~GET_COMM(OLD_PERIOD));
        if (GET_COMM(PERIOD)) {
          TIME += (period - 1) * TIME_SCALE;
        } else {
          TIME += (1 * TIME_SCALE);
        }
      }
      if (GET_COMM(PERIOD)) {
        // artificially update t_ref based on the last sync point
        uint16_t time_diff = (TIME - last_time) / TIME_SCALE;
        rtimer_clock_t new_t_ref = last_t_ref_sync + (int32_t)(time_diff * skew) / (int32_t)64 + RTIMER_SECOND * (time_diff % 2);
        set_t_ref_l(new_t_ref);
        t_ref_sync = new_t_ref;
        if (sync_state == UNSYNCED_1) {
          // retrieve the old schedule
          memcpy(&sched.slot, &old_sched.slot, sizeof(sched.slot));
        }
      }
    }
  }
  SYNC_LEDS();
}

//--------------------------------------------------------------------------------------------------
static inline void compute_new_joining_state(void) {
  if (sync_state == SYNCED) {
    /// @note Following part is not needed.
    /// We send stream add requests bead on the demand. If no demand, not stream add request.
    /// The number of rounds to wait can be set just after sending stream add request.
//    if (joining_state == NOT_JOINED) {
//      if (is_source && n_stream_reqs == 0) {
//        n_stream_reqs = 1;
//        stream_reqs[0].ipi = INIT_IPI;
//        stream_reqs[0].req_type = SET_STREAM_ID(1) | (ADD_STREAM & 0x3);
//        stream_reqs[0].time_info = TIME;
//        t_last_req = 0;
//      }
//      joining_state = JOINING;
//      // try as soon as sync_state is SYNCED
//      rounds_to_wait = 0;
//    } else if (joining_state == JUST_TRIED) {
//      // joining did not succeed at the last trial (collision)
//      rounds_to_wait = random_rand() % (1 << (n_joining_trials % 4));
//      n_joining_trials++;
//      joining_state = JOINING;
//    }
  } else {
    if (sync_state == BOOTSTRAP) {
      // set as NOT_JOINED if bootstrapping:
      // after exiting bootstrapping it will try to join again,
      // if no slots are received meanwhile
      joining_state = NOT_JOINED;
    }
  }
}

static rtimer_clock_t t_no_comm;

//--------------------------------------------------------------------------------------------------
/// @brief This proto-thread does LWB synchronization using schedule from the host node.
///
///       If it is a host node, a Glossy phase is started as an initiator to disseminate the schedule.
///       It should also be noted that the schedule can be disseminated at the end of a round.
///
///       If it is a source node(or other node), a Glossy phase is started as a receiver to receive the schedule.
PT_THREAD(g_sync(struct rtimer *t, void *ptr)) {
  PT_BEGIN(&pt_sync);

  if (is_host) {  // host
    // set as if there was a communication schedule 1 second ago
    t_start = RTIMER_TIME(t) - RTIMER_SECOND;
    t_ref_sync = t_start;
    last_time = TIME - 1 * TIME_SCALE;
    while (1) {
      leds_on(LEDS_GREEN);
      if (GET_COMM(PERIOD)) {
        // This schedule is to be sent at the beginning of the round.
        t_start = RTIMER_TIME(t);
      } else {
        t_no_comm = RTIMER_TIME(t);
      }
      save_energest_values();
      // Disseminate schedule
      glossy_start((uint8_t *)&sched,
                   sched_size,
                   GLOSSY_INITIATOR,
                   GLOSSY_SYNC,
                   N_SYNC,
                   SCHED,
                   RTIMER_TIME(t) + T_SYNC_ON,
                   (rtimer_callback_t)g_sync,
                   &rt,
                   NULL);
      PT_YIELD(&pt_sync);

      leds_off(LEDS_GREEN);
      glossy_stop();
      update_control_dc();

      if (GET_COMM(PERIOD)) {
        // We've just sent the schedule at the beginning of the round.
        if (IS_SYNCED() && RTIMER_CLOCK_LT(t_start, T_REF)) {
          t_ref_sync = T_REF;
          last_time = TIME;
        } else {
          // artificially update t_ref based on the last sync point
          set_t_ref_l(t_ref_sync + RTIMER_SECOND * (((TIME - last_time) / TIME_SCALE) % 2));
        }
        PT_SCHEDULE(g_rr_host(&rt, NULL));

      } else {
        // We've just sent the schedule at the end of the round.
        // We have to send the schedule at the beginning of the next round. So, we schedule
        // g_sync proto-thread to send the schedule again in the next round.
        SCHEDULE_L(t_start, period * (uint32_t)RTIMER_SECOND, g_sync); // L
        // set time to the new value
        TIME = (uint16_t)time;
        PERIOD = period | SET_COMM(1);

      }
      process_poll(&print);
      PT_YIELD(&pt_sync);
    }
  } else { //---------------------------------------------------------
    // source node
    sync_state = BOOTSTRAP;
    joining_state = NOT_JOINED;
    SYNC_LEDS();

    while (1) {
      if (sync_state == BOOTSTRAP) {
        save_energest_values();
        // Start Glossy to receive initial schedule.
        glossy_start((uint8_t *)&sched,
                     0,
                     GLOSSY_RECEIVER,
                     GLOSSY_SYNC,
                     N_SYNC,
                     0,
                     RTIMER_NOW() + T_SYNC_ON,
                     (rtimer_callback_t)g_sync,
                     &rt,
                     NULL);
        PT_YIELD(&pt_sync);

        while (!IS_SYNCED()) {
          // Glossy is not synchronized yet. We have to retry
          glossy_stop();
          update_control_dc();
          energest_flush();
#if PRINT_BOOTSTRAP_MSG
          SCHEDULE(RTIMER_NOW(), T_BOOTSTRAP_GAP, g_sync);
          process_poll(&print);
          PT_YIELD(&pt_sync);
#endif /* PRINT_BOOTSTRAP_MSG */
          /// @note Kasun: I don't know this is necessary or not.
          ///       It seems node get stuck in this loop while trying to receive schedule to
          ///       to exit from bootstrap state.
          // wait 50 millisecond before retrying
          // rtimer_clock_t now = RTIMER_NOW();
          // while (RTIMER_CLOCK_LT(RTIMER_NOW(), now + RTIMER_SECOND / 20));
          save_energest_values();
          glossy_start((uint8_t *)&sched,
                       0,
                       GLOSSY_RECEIVER,
                       GLOSSY_SYNC,
                       N_SYNC,
                       0,
                       RTIMER_NOW() + T_SYNC_ON,
                       (rtimer_callback_t)g_sync,
                       &rt,
                       NULL);
          PT_YIELD(&pt_sync);
        }
      } else {
        save_energest_values();
        // start Glossy to receive schedule.
        glossy_start((uint8_t *)&sched,
                     0,
                     GLOSSY_RECEIVER,
                     GLOSSY_SYNC,
                     N_SYNC,
                     0,
                     RTIMER_TIME(t) + T_SYNC_ON + T_guard,
                     (rtimer_callback_t)g_sync,
                     &rt,
                     NULL);
        PT_YIELD(&pt_sync);
      }

      glossy_stop();
      update_control_dc();
      if (get_rx_cnt() && (get_header() != SCHED)) {
        // The node haasn't received Glossy or the received packet has a wrong header.
        // We treat this as missed/unsynced
        SET_UNSYNCED();
      }

      compute_new_sync_state();
      compute_new_joining_state();
      update_sched_stats();

      if ((sync_state == SYNCED || sync_state == UNSYNCED_1) && GET_COMM(PERIOD)) {
        // communication
        // We've just received the schedule at the beginning of the round.
        uncompress_schedule();
        PT_SCHEDULE(g_rr_source(&rt, NULL));
        process_poll(&print);
        PT_YIELD(&pt_sync);
      } else {
        // copy the current schedule to old_sched
        old_sched = sched;
        // erase the schedule before receiving the next one
        memset((uint8_t *)&sched.slot, 0, sizeof(sched.slot));
        SYNC_LEDS();

        if (sync_state != BOOTSTRAP) {
          if (GET_COMM(PERIOD)) {
            SCHEDULE(t_ref_sync, PERIOD_RT(1) - T_guard, g_sync);
          } else {
            SCHEDULE_L(t_ref_sync, PERIOD_RT(period) - T_guard, g_sync); // L
          }

          process_poll(&print);

          PT_YIELD(&pt_sync);
        } else {
          // we are in BOOTSTRAP state.
          if (IS_SYNCED() && (period > 1)) {
            // we received a schedule which was not meant for synchronization:
            // wake up a bit earlier than the next synchronization schedule
            T_guard = T_GUARD_3;
            SCHEDULE_L(RTIMER_NOW() - 10, PERIOD_RT(period - 1) - T_guard, g_sync); // L
            process_poll(&print);
            PT_YIELD(&pt_sync);
          }

        }
      }
    }
  }

  PT_END(&pt_sync);
}
