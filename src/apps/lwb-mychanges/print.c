/*
 * print.c
 *
 *  Created on: Aug 3, 2011
 *      Author: fe
 */

#if MOBILE
static char buf[2];

#define PRINTF_CONET(...) \
    if (is_sink) { \
      putchar(0x0b); \
      memcpy(buf, &node_id, 2); \
      putchar(buf[1]); \
      putchar(buf[0]); \
      putchar(0xff); \
      putchar(0xff); \
      putchar(0x0f); \
      printf(__VA_ARGS__); \
      clock_delay(10000); \
    }
#endif /* MOBILE */

#if PRINT_PERIOD
static uint32_t print_cnt = 0;
#endif /* PRINT_PERIOD */

void print_logs(uint8_t times) {
#if 0
    char buf[2];
    // type: uint8_t
    putchar(0x0b);

    // node id: uint16_t
    memcpy(buf, &node_id, 2);
    putchar(buf[1]);
    putchar(buf[0]);

    // parent node id: uint16_t
    putchar(0xff);
    putchar(0xff);

    // size? appears not to be used. uint32_t
    putchar(0x0f);
#endif /* MOBILE */
#if !LWB_DEBUG
  uint8_t i;
  if (is_sink) {
    if (is_host) {
#ifdef DSN_BOOTSTRAPPING
      for (idx = 0; idx < slots_idx; idx++) {
        printf("%5u %3u %1u\n", slots_time, slots[idx].id, slots[idx].type);
      }
#elif USERINT_INT
      for (idx = 0; idx < N_NODES_MAX; idx++) {
        if (n_tot[idx]) {
          printf("0 %lu %u %u %u %u %lu %lu\n", time - period, idx, n_rcvd[idx], n_rcvd_tot[idx], rc[idx], en_on[idx], en_tot[idx]);
        }
      }
      printf("--------------------------------------\n");
      if (period > 2) {
        stream_info *curr_stream;
        for (curr_stream = list_head(streams_list); curr_stream != NULL; curr_stream = curr_stream->next) {
          printf("1 %lu %u %u %u %u %u\n", time - period,
              curr_stream->node_id, curr_stream->stream_id, curr_stream->ipi,
              curr_stream->last_assigned, curr_stream->next_ready);
        }
        printf("-------------------------------------------------------------------\n");
      }
      return;
#else
      for (idx = 0; idx < N_NODES_MAX; idx++) {
        if (n_tot[idx]) {
          for (i = 0; i < times; i++) {
#if !TINYOS
            printf("%3u ", node_id);
#endif /* !TINYOS */
#if DSN_TRAFFIC_FLUCTUATIONS || DSN_INTERFERENCE || DSN_REMOVE_NODES
            printf("1 %3u %10lu %10lu %5u %5u %5u %5u\n", idx, en_on[idx], en_tot[idx],
                (uint16_t)((10000LLU * en_on[idx]) / en_tot[idx]), n_rcvd[idx],
                (uint16_t)((10000LU * n_rcvd[idx]) / n_tot[idx]), n_tot[idx]);
#elif LATENCY
            if (period > 2 || times > 1) {
              printf("1 %u %u %u %u %u %u\n", idx, dc[idx], n_rcvd[idx],
                  (uint16_t)((10000LU * n_rcvd[idx]) / n_tot[idx]), n_tot[idx],
                  (uint16_t)((100LU * lat[idx]) / n_rcvd[idx]));
            }
#elif MOBILE
            PRINTF_CONET("1 %3u %5u\n", idx, dc[idx]);
            PRINTF_CONET("4 %5u %5u\n", n_rcvd_tot[idx], n_rcvd[idx]);
            PRINTF_CONET("5 %5u %5u\n",
                (uint16_t)((10000LU * n_rcvd[idx]) / n_tot[idx]), n_tot[idx]);
#else
            printf("1 %3u %5u %5u %5u %5u\n", idx, dc[idx], n_rcvd[idx],
                (uint16_t)((10000LU * n_rcvd[idx]) / n_tot[idx]), n_tot[idx]);
#endif
#ifdef KANSEI_SCALABILITY
            clock_delay(10000);
#endif /* KANSEI_SCALABILITY */
          }
        }
      }
#endif /* DSN_BOOTSTRAPPING */
    } else {
      for (idx = 0; idx < N_NODES_MAX; idx++) {
        if (n_rcvd[idx]) {
          for (i = 0; i < times; i++) {
#if !TINYOS
            printf("%3u ", node_id);
#endif /* !TINYOS */
#if DSN_TRAFFIC_FLUCTUATIONS || DSN_INTERFERENCE || DSN_REMOVE_NODES
            printf("2 %3u %10lu %10lu %5u %5u 0 0\n", idx, en_on[idx], en_tot[idx],
                (uint16_t)((10000LLU * en_on[idx]) / en_tot[idx]), n_rcvd[idx]);
#elif MOBILE
            PRINTF_CONET("2 %3u %5u\n", idx, dc[idx]);
            PRINTF_CONET("4 %5u %5u\n", n_rcvd_tot[idx], n_rcvd[idx]);
#elif USERINT_INT
#else
            printf("2 %3u %5u %5u 0 0\n", idx, dc[idx], n_rcvd[idx]);
#endif /* DSN_TRAFFIC_FLUCTUATIONS */
#ifdef KANSEI_SCALABILITY
            clock_delay(10000);
#endif /* KANSEI_SCALABILITY */
          }
        }
      }
    }
  } else {
    if (is_host) {
      for (idx = 0; idx < N_NODES_MAX; idx++) {
        if (n_tot[idx]) {
          for (i = 0; i < times; i++) {
#if !TINYOS
            printf("%3u ", node_id);
#endif /* !TINYOS */
            printf("0 %3u 0 0 0 %5u\n", idx, n_tot[idx]);
#ifdef KANSEI_SCALABILITY
            clock_delay(10000);
#endif /* KANSEI_SCALABILITY */
          }
        }
      }
    }
  }
  // print information about myself
  uint32_t en_on = energest_type_time(ENERGEST_TYPE_LISTEN) + energest_type_time(ENERGEST_TYPE_TRANSMIT);
  uint32_t en_tot = energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM);
  for (i = 0; i < times; i++) {
#if !TINYOS && !defined DSN_BOOTSTRAPPING
    printf("%3u ", node_id);
#endif /* !TINYOS */
#if MOBILE
    PRINTF_CONET("3 %2d 0 %4u\n",
        (relay_cnt_cnt) ? ((relay_cnt_sum * RELAY_CNT_FACTOR) / relay_cnt_cnt) : (RELAY_CNT_INVALID),
        (uint16_t)((10000LLU * en_on) / en_tot)
    );
#else
#ifndef DSN_BOOTSTRAPPING
    printf("3 %3u %3u %5ld %5u %5u\n", node_id,
        (relay_cnt_cnt) ? ((relay_cnt_sum * RELAY_CNT_FACTOR) / relay_cnt_cnt) : (RELAY_CNT_INVALID),
        skew,
#if CONTROL_DC
        (uint16_t)((10000LLU * en_control)  / en_tot),
#else
        0,
#endif /* CONTROL_DC */
        (uint16_t)((10000LLU * en_on) / en_tot)
    );
#endif /* DSN_BOOTSTRAPPING */
    #endif /* MOBILE */
  }
#else
#if FAIRNESS
  stream_info *curr_stream;
  if (is_host) {
    for (curr_stream = list_head(streams_list); curr_stream != NULL; curr_stream = curr_stream->next) {
      uint16_t id = curr_stream->node_id;
      printf("%u %u %u\n", (uint16_t)(time / TIME_SCALE), id, n_rcvd[id]);
    }
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
#if JOINING_NODES && DYNAMIC_FREE_SLOTS
    printf("last_free_slot %u, last_free_reqs %u, n_free %u\n",
        last_free_slot_time, last_free_reqs_time, GET_N_FREE(N_SLOTS));
#endif
#if DYNAMIC_PERIOD
    uint32_t T_period = 1000000LU * (t_period_stop - t_period_start) / RTIMER_SECOND;
    printf("period us %6lu\n", T_period);
#endif /* DYNAMIC_PERIOD */
#if COMPRESS
    uint16_t ratio = 0;
    if (GET_N_SLOTS(N_SLOTS)) {
      ratio = (10000LU * (sched_size - SYNC_HEADER_LENGTH)) / (GET_N_SLOTS(N_SLOTS) * 2);
    }
    uint32_t T_compress = 1000000LU * (t_compr_stop - t_compr_start) / RTIMER_SECOND;
    printf("sched size %u, compr ratio %u, ack_snc %u, first_snc %u, us %lu\n",
        sched_size, ratio, ack_snc, first_snc, T_compress);
#if 0
    printf("compr snc_tmp");
    for (idx = 0; idx < GET_N_SLOTS(N_SLOTS); idx++) {
      printf(" %u", snc_tmp[idx]);
    }
    printf("\ncompr snc    ");
    for (idx = 0; idx < GET_N_SLOTS(N_SLOTS); idx++) {
      printf(" %u", snc[idx]);
    }
    printf("\ncompr sc");
    for (idx = 0; idx < sched_size - SYNC_HEADER_LENGTH; idx++) {
      printf(" %02x", sched.slot[idx]);
    }
    printf("\n");
#endif /* GLOSSY_DEBUG */
#endif /* COMPRESS */
#if CONTROL_DC
    printf("control_dc %5u\n", (uint16_t)((10000LLU * en_control) / (energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM))));
#endif /* CONTROL_DC */
#if DYNAMIC_PERIOD
    printf("period %u, T_min %u, data_ipi %u, data_cnt %lu, saturated %u\n", period, T_min, data_ipi, data_cnt, saturated);
#endif /* DYNAMIC_PERIOD */
    if (0) {
      stream_info *curr_stream;
      for (curr_stream = list_head(streams_list); curr_stream != NULL; curr_stream = curr_stream->next) {
        uint16_t id = curr_stream->node_id;
        printf("%3u %3u %3u %5u %5u %3u %3u %5u %5u %5u %5u %5u\n",
            id,
            curr_stream->stream_id,
            curr_stream->ipi,
            curr_stream->last_assigned,
            curr_stream->next_ready,
#if REMOVE_NODES
            curr_stream->n_cons_missed >> 7,
            curr_stream->n_cons_missed & 0x7f,
#else
            0,
            0,
#endif /* REMOVE_NODES */
#if LWB_DEBUG && !COOJA
            rc[id],
#if CONTROL_DC
            dc_control[id],
#else
            0,
#endif /* CONTROL_DC */
#else
            0,
            0,
#endif /* LWB_DEBUG && !COOJA */

            dc[id],
            n_tot[id] ? (uint16_t)((10000LU * n_rcvd[id]) / n_tot[id]) : 0,
            n_tot[id]
        );
      }
    }
    printf("n_added %u, n_deleted %u, n_no_space %u, n_replaced %u, n_duplicates %u\n",
        n_added, n_deleted, n_no_space, n_replaced, n_duplicates);
#if GLOSSY_DEBUG
    printf("high_T_irq %u, rx_timeout %u, bad_length %u, bad_header %u, bad_crc %u\n",
        high_T_irq, rx_timeout, bad_length, bad_header, bad_crc);
#endif /* GLOSSY_DEBUG */
  } else {
    if (node_id == 189) {
    printf("time %5lu, period %3u, skew %ld, sync_state %1u, otg %2u\n", time, period, skew, sync_state, rt.overflows_to_go);
    uint32_t en_on = energest_type_time(ENERGEST_TYPE_LISTEN) + energest_type_time(ENERGEST_TYPE_TRANSMIT);
    uint32_t en_tot = energest_type_time(ENERGEST_TYPE_CPU) + energest_type_time(ENERGEST_TYPE_LPM);
    printf("comm %1u, dc %5u, en_on %lu, en_tot %lu\n", GET_COMM(OLD_PERIOD), (uint16_t)((10000LLU * en_on) / en_tot), en_on, en_tot);
#if CONTROL_DC
    printf("control_dc %5u\n", (uint16_t)((10000LLU * en_control) / en_tot));
#endif /* CONTROL_DC */
#if COMPRESS
    uint32_t T_uncompress = 1000000LU * (t_uncompr_stop - t_uncompr_start) / RTIMER_SECOND;
    printf("uncompr d_bits %u, l_bits %u, us %lu\n", old_sched.slot[2] / 8, old_sched.slot[2] % 8, T_uncompress);
#endif /* COMPRESS */
#if JOINING_NODES
    if (joining_state != JOINED) {
      printf("joining_state %1u, n_joining_trials %3u, rounds_to_wait %3u\n",
          joining_state, n_joining_trials, rounds_to_wait);
    }
#endif /* JOINING_NODES */
#if !COOJA
    char s[8] = "";
    itoa(sched_bin, s, 2);
    printf("%08s\n", s);
#endif /* !COOJA */
#if COMPRESS
#if 0
    uint32_t T_uncompress = 1000000LU * (t_uncompr_stop - t_uncompr_start) / RTIMER_SECOND;
    printf("uncompr d_bits %u, l_bits %u, us %lu snc", old_sched.slot[2] / 8, old_sched.slot[2] % 8, T_uncompress);
    for (idx = 0; idx < GET_N_SLOTS(N_SLOTS); idx++) {
      printf(" %u", snc[idx]);
    }
    printf("\nuncompr sc");
    for (idx = 0; idx < sched_size - SYNC_HEADER_LENGTH; idx++) {
      printf(" %02x", old_sched.slot[idx]);
    }
    printf("\n");
#endif /* GLOSSY_DEBUG */
#endif /* COMPRESS */
#if STATIC
    printf("relay_cnt_cnt %u, relay_cnt_sum %u\n", relay_cnt_cnt, relay_cnt_sum);
#endif /* STATIC */
#if GLOSSY_DEBUG
    printf("high_T_irq %u, rx_timeout %u, bad_length %u, bad_header %u, bad_crc %u\n",
        high_T_irq, rx_timeout, bad_length, bad_header, bad_crc);
#endif /* GLOSSY_DEBUG */
//    for (idx = 0; idx < N_NODES_MAX; idx++) {
//      if (n_rcvd[idx]) {
//          printf("%3u %5u\n", idx, n_rcvd[idx]);
//      }
//    }
    }
  }
  }
#endif /* FAIRNESS */
#endif /* !LWB_DEBUG */
}

#if FLASH
  void print_flash_logs(uint8_t times) {
    uint8_t i;
    uint32_t curr_idx;
    if (is_host) {
      for (i = 0; i < times; i++) {
        for (curr_idx = 0; curr_idx < flash_idx; curr_idx++) {
          xmem_pread(&flash_dy, sizeof(flash_dy), FLASH_OFFSET + curr_idx * sizeof(flash_dy));
          printf("0 %u %u %u %u\n", flash_dy.time, flash_dy.node_id, flash_dy.n_rcvd, flash_dy.n_tot);
          clock_delay(10000);
        }
      }
    } else {
      for (i = 0; i < times; i++) {
        for (curr_idx = 0; curr_idx < flash_idx; curr_idx++) {
          xmem_pread(&flash_dc, sizeof(flash_dc), FLASH_OFFSET + curr_idx * sizeof(flash_dc));
          printf("1 %u %u %lu %lu\n", flash_dc.time, node_id, flash_dc.en_on, flash_dc.en_tot);
          clock_delay(10000);
        }
      }
    }
  }
#endif /* FLASH */

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
#if FLASH
      print_flash_logs(1);
#else
      print_logs(1);
#endif /* FLASH */
#else
      // wait a few random seconds to avoid nodes printing all at the same time
      clock_time_t T_wait = random_rand() % (180 * CLOCK_SECOND);
      clock_time_t now = clock_time();
      while (clock_time() < now + T_wait) {
        watchdog_periodic();
      }
      TACCTL1 = 0;
      // print the logs several times at the end of the experiment
#if FLASH
      print_flash_logs(3);
#else
      print_logs(10);
#endif /* FLASH */
#endif /* COOJA */
    }
#endif /* EXP_LENGTH */

  }

  PROCESS_END();
}
