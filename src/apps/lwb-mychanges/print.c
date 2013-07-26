/*
 * print.c
 *
 *  Created on: Aug 3, 2011
 *      Author: fe
 */


#if PRINT_PERIOD
static uint32_t print_cnt = 0;
#endif // PRINT_PERIOD

void print_logs() {
  printf("Buf Stats: %u, %u\n", buf_stats.ui16_tx_overflow, buf_stats.ui16_rx_overflow);

  if (is_host) {
    //printf("last_free_slot %u, last_free_reqs %u, n_free %u\n",
    //       last_free_slot_time, last_free_reqs_time, GET_N_FREE(N_SLOTS));

    stream_info *p_stream;
    for (p_stream = list_head(streams_list); p_stream != NULL; p_stream = p_stream->next) {
      printf("stream: id %u, ipi %u, missed %u\n", p_stream->node_id, p_stream->ipi, p_stream->n_cons_missed);
    }

  } else {
    printf("state : %u, n_stream_reqs : %u\n", joining_state, n_stream_reqs);
    printf("rounds_to_wait : %u\n", rounds_to_wait);
    printf("sche: COMM %u, time %u, T %u, data %u, n_free %u\n",
           GET_COMM(PERIOD),
           TIME,
           GET_PERIOD(PERIOD),
           GET_N_SLOTS(N_SLOTS),
           GET_N_FREE(N_SLOTS));

    uint8_t ui8_slots_for_me = 0;
    for (idx = 0; idx < GET_N_SLOTS(N_SLOTS); idx++) {
      if (snc[idx] == node_id) {
        ui8_slots_for_me++;
      }
    }
    printf("slots for me  %u\n", ui8_slots_for_me);

    for (idx = 0; idx < N_NODES_MAX; idx++) {
      if (n_rcvd[idx]) {
        printf("0 %lu %u %u\n", time - period, idx, n_rcvd[idx]);
      }
    }
  }

  printf("polls : %u\n", ui8_n_polls);

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
      print_logs();
      print_cnt++;
    }
#else
    print_logs();
#endif // PRINT_PERIOD

  }

  PROCESS_END();
}
