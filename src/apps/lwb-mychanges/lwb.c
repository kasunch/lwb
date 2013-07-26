
#include "lwb.h"

#if USING_DEFAULT
#warning ##################################################################
#warning # LWB configuration not provided or not defined: using 'default' #
#warning ##################################################################
#endif /* DEFAULT_CONF */

static uint16_t idx, slot_idx;

static uint16_t hosts[] = HOSTS;
static uint16_t sinks[] = SINKS;
static uint16_t sources[] = SOURCES;
static uint8_t  is_host, is_sink, is_source = 0;

// For some statistics
static uint16_t n_rcvd[N_NODES_MAX];
static uint16_t n_tot[N_NODES_MAX];
#if LATENCY
static uint16_t lat[N_NODES_MAX];
#endif /* LATENCY */
//static uint16_t dc[N_NODES_MAX];

#if LWB_DEBUG && !COOJA
static uint16_t dc_control[N_NODES_MAX];
static uint8_t  rc[N_NODES_MAX];
#endif /* LWB_DEBUG && !COOJA */

void set_hosts_sinks_sources(void) {
  // initialize arrays for statistics on received packets
  memset(n_rcvd, 0, sizeof(n_rcvd));
  memset(n_tot, 0, sizeof(n_tot));

  for (idx = 0; idx < sizeof(hosts) / 2; idx++) {
    if (node_id == hosts[idx]) {
      is_host = 1;
      break;
    }
  }
  if (sizeof(sinks) == 0) {
    // all nodes are sinks
    is_sink = 1;
  } else {
    for (idx = 0; idx < sizeof(sinks) / 2; idx++) {
      if (node_id == sinks[idx]) {
        is_sink = 1;
        break;
      }
    }
  }
  if (sizeof(sources) == 0) {
    // all nodes but the host are sources
    if (!is_host) {
      is_source = 1;
    }
  } else {
    for (idx = 0; idx < sizeof(sources) / 2; idx++) {
      if (node_id == sources[idx]) {
        is_source = 1;
        break;
      }
    }
  }
}

#if !COOJA
static uint8_t sched_bin;
#endif /* !COOJA */

MEMB(streams_memb, stream_info, N_STREAMS_MAX);
LIST(streams_list);

#if REMOVE_NODES
static stream_info *streams[N_SLOTS_MAX];
#endif /* REMOVE_NODES */


static uint16_t snc[N_SLOTS_MAX];     ///<  Contains the node IDs related to slots. Zero means the slot belongs to host.
static uint16_t snc_tmp[N_SLOTS_MAX];

static uint8_t  first_snc, ack_snc;

#define TIME       sched.time     ///< Current time of the host in seconds.
#define PERIOD     sched.T        ///< Round period (time between two rounds) in current schedule in seonds.
#define N_SLOTS    sched.n_slots  ///< Number of slots in current schedule. Free slots = (n_slots >> 6), Data slots = (n_slots & 0x3F)
#define OLD_PERIOD old_sched.T    ///< Previous round period.

#define PERIOD_RT(T)     ((T) * (uint32_t)RTIMER_SECOND + ((int32_t)((T) * skew) / (int32_t)64))

static struct rtimer rt;
static struct pt pt_sync, pt_rr;

static sched_struct sched;      ///< Current schedule
static sched_struct old_sched;  ///< Old schedules. This may not be the previous one.

//static data_struct data;
static data_header_struct data_header;

static stream_req stream_reqs[N_STREAM_REQS_MAX]; ///< To store piggybacked stream requests
static uint8_t n_stream_reqs;                     ///< Number of stream requests

static rtimer_clock_t t_start = 0;
static uint8_t sched_size;

static uint16_t stream_ack[N_SLOTS_MAX];
static uint8_t stream_ack_idx;

#define STREAM_ACK_LENGTH 2
#define STREAM_ACK_IDX_LENGTH 1

// variables for statistics about schedule reception
static uint8_t  first_schedule_rcvd = 0;
static uint16_t last_time = 0;

#if JOINING_NODES
static uint8_t  joining_state;
static uint8_t  n_joining_trials = 0;
static uint8_t  rounds_to_wait;
static uint16_t t_last_req;
#if DYNAMIC_FREE_SLOTS
static uint16_t last_free_slot_time;
static uint16_t last_free_reqs_time;
#endif /* DYNAMIC_FREE_SLOTS */
#endif /* JOINING_NODES */

static uint16_t last_t_ref_sync, t_ref_sync = 0;
static uint32_t skew = 0;
static uint8_t  period;                       ///< Round period.

#define GET_PERIOD(per) ((per) & 0x7f)        ///< Extract round period. The round period is represented by lest significant 7 bits.
#define SET_COMM(comm) ((comm) << 7)          ///< Set whether data communication follows the schedule or not.
#define GET_COMM(comm) ((comm) >> 7)          ///< Get whether data communication follows the schedule or not.

#define GET_TIME_LOW()   (uint16_t)(time & 0xffff)
#define GET_TIME_HIGH()  (uint16_t)(time >> 16)
#define SET_TIME_LOW(a)  (time = ((uint32_t)GET_TIME_HIGH() << 16) | ((a) & 0xffff))
#define SET_TIME_HIGH(a) (time = ((uint32_t)(a) << 16) | GET_TIME_LOW())

#if CONTROL_DC
static uint32_t en_control, en_rx, en_tx = 0;
#endif /* CONTROL_DC */
static inline void save_energest_values(void) {
#if CONTROL_DC
  // store current Rx and Tx energest values (before Glossy starts)
  en_rx = energest_type_time(ENERGEST_TYPE_LISTEN);
  en_tx = energest_type_time(ENERGEST_TYPE_TRANSMIT);
#endif /* CONTROL_DC */
}
static inline void update_control_dc(void) {
#if CONTROL_DC
  // add the energy spent to the control energy part (after Glossy finishes)
  en_control += (energest_type_time(ENERGEST_TYPE_LISTEN) - en_rx) +
      (energest_type_time(ENERGEST_TYPE_TRANSMIT) - en_tx);
#endif /* CONTROL_DC */
}

static uint32_t time;                           ///< Global time in seconds
static uint8_t  sync_state;                     ///< Synchronization state of the node
static uint16_t n_streams, n_slots_to_assign;
static uint8_t  relay_cnt_cnt;
static uint16_t relay_cnt_sum;

static rtimer_clock_t T_guard;              ///< Guard time that Glossy should be started earlier than the planned time.
                                            ///< This is placed to reduce probability of missing packets due to radio being turned on late.


uint16_t get_node_id_from_schedule(uint8_t *sched, uint16_t pos) {
  uint16_t id;
  uint16_t offset_bits = NODE_ID_BITS * pos;
  memcpy(&id, sched + offset_bits / 8, 2);
  return ((id >> (offset_bits % 8)) & NODE_ID_MASK);
}

static inline void set_node_id_to_schedule(uint8_t *sched, uint16_t pos, uint16_t id) {
  uint16_t tmp;
  uint16_t offset_bits = NODE_ID_BITS * pos;
  memcpy(&tmp, sched + offset_bits / 8, 2);
  tmp = tmp & (~(NODE_ID_MASK << (offset_bits % 8)));
  id = (id & NODE_ID_MASK) << (offset_bits % 8);
  tmp |= id;
  memcpy(sched + offset_bits / 8, &tmp, 2);
}

stream_info* get_stream_from_id(uint16_t id) {
  stream_info *curr_stream;
  for (curr_stream = list_head(streams_list); curr_stream != NULL; curr_stream = curr_stream->next) {
    if (curr_stream->node_id == id) {
      return curr_stream;
    }
  }
  return NULL;
}

#define IS_LESS_EQ_THAN_TIME(a) ((int16_t)(a-(uint16_t)time) <= 0)

#define IS_STREAM_TO_ADD(a) ((a & 0x3) == ADD_STREAM)
#define IS_STREAM_TO_DEL(a) ((a & 0x3) == DEL_STREAM)
#define IS_STREAM_TO_REP(a) ((a & 0x3) == REP_STREAM)
#define GET_STREAM_ID(a)    (a >> 2)
#define SET_STREAM_ID(a)    (a << 2)
#define GET_N_FREE(a)       (a >> 6)
#define SET_N_FREE(a)       (a << 6)
#define GET_N_SLOTS(a)      (a & 0x3f)

#define MAX_BUF_ELEMENT_SIZE 102

#define MAX_TX_BUF_SIZE 500
#define MAX_RX_BUF_SIZE 500

#define MAX_TX_BUF_ELEMENTS (MAX_TX_BUF_SIZE / MAX_BUF_ELEMENT_SIZE)
#define MAX_RX_BUF_ELEMENTS (MAX_TX_BUF_SIZE / MAX_BUF_ELEMENT_SIZE)

typedef struct data_buf {
  struct data_buf* next;
  uint8_t data[MAX_BUF_ELEMENT_SIZE];
  uint8_t ui8_data_size;
} data_buf_t;

// Buffers for TX and RX
MEMB(tx_buffer_memb, data_buf_t, MAX_TX_BUF_ELEMENTS);
LIST(tx_buffer_list);
MEMB(rx_buffer_memb, data_buf_t, MAX_RX_BUF_ELEMENTS);
LIST(rx_buffer_list);

static uint8_t ui8_tx_buf_element_count;
static uint8_t ui8_rx_buf_element_count;

static uint8_t ui8_n_polls;

static void (*p_output_fn)(uint8_t*, uint8_t);

typedef struct buffer_stats {
  uint16_t ui16_tx_overflow;
  uint16_t ui16_rx_overflow;
} buffer_stats_t;

static buffer_stats_t buf_stats;

void lwb_input(uint8_t* p_data, uint8_t ui8_len) {
  // Hack to avoid packet input for host node
  /// TODO Remove this.
  if (is_host) {
    return;
  }
  data_buf_t* p_buf = memb_alloc(&tx_buffer_memb);
  if (p_buf) {
    memcpy(p_buf->data, p_data, ui8_len);
    p_buf->ui8_data_size = ui8_len;
    list_add(tx_buffer_list, p_buf);
    ui8_tx_buf_element_count++;
  } else {
    buf_stats.ui16_tx_overflow++;
  }

  if (joining_state == NOT_JOINED) {
    // We are not joined, So its time to make a joining request.
    n_stream_reqs = 1;
    stream_reqs[0].ipi = INIT_IPI;
    stream_reqs[0].req_type = SET_STREAM_ID(1) | (ADD_STREAM & 0x3);
    stream_reqs[0].time_info = INIT_DELAY;
    joining_state = JOINING;
  }
}

void lwb_set_output_function(void (*p_fn)(uint8_t*, uint8_t)) {
  p_output_fn = p_fn;
}

PROCESS(lwb_init, "LWB init");
//AUTOSTART_PROCESSES(&lwb_init);

#if COMPRESS
#include "compress.c"
#endif /* COMPRESS */
#include "scheduler.c"
#include "g_sync.c"
#include "g_rr.c"
#include "print.c"


PROCESS_THREAD(lwb_init, ev, data)
{
  PROCESS_BEGIN();

  static struct etimer et;

  set_hosts_sinks_sources();

  cc2420_set_tx_power(TX_POWER);
  process_start(&print, NULL);
  process_start(&glossy_process, NULL);

  memb_init(&tx_buffer_memb);
  list_init(tx_buffer_list);

  memb_init(&rx_buffer_memb);
  list_init(rx_buffer_list);

  ui8_tx_buf_element_count = 0;
  ui8_rx_buf_element_count = 0;

  if (is_host) {
    scheduler_init();
    SCHEDULE_L(RTIMER_NOW() - 10, INIT_PERIOD * (uint32_t)RTIMER_SECOND, g_sync);
  } else {
    n_stream_reqs = 0;
    SCHEDULE_L(RTIMER_NOW() - 10, INIT_PERIOD * (uint32_t)(random_rand() % RTIMER_SECOND), g_sync);
  }

  if (INIT_DELAY) {
    // set timer to reset energest values
    etimer_set(&et, INIT_DELAY * CLOCK_SECOND);
    PROCESS_YIELD_UNTIL(etimer_expired(&et));

    energest_init();
    ENERGEST_ON(ENERGEST_TYPE_CPU);

#if CONTROL_DC
    en_control = 0;
#endif /* CONTROL_DC */
  }

  while (1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    ui8_n_polls++;
      data_buf_t* p_buf;
      while(ui8_rx_buf_element_count > 0) {
        p_buf = list_head(rx_buffer_list);
        if (p_output_fn) {
          p_output_fn(p_buf->data, p_buf->ui8_data_size);
        }
        list_remove(rx_buffer_list, p_buf);
        memb_free(&rx_buffer_memb, p_buf);
        ui8_rx_buf_element_count--;
      }
  }

  PROCESS_END();
}


PROCESS(lwb_int_test, "LWB interface test");
AUTOSTART_PROCESSES(&lwb_int_test);

void lwb_output(uint8_t* p_data, uint8_t ui8_len) {
  printf("Data; %s, Len: %d\n", p_data, ui8_len);
}

static uint8_t test_counter = 0;
static uint8_t tmp_buffer[128];

PROCESS_THREAD(lwb_int_test, ev, data)
{
  static struct etimer et1;

  PROCESS_BEGIN();

  lwb_set_output_function(&lwb_output);
  process_start(&lwb_init, NULL);


  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et1, CLOCK_SECOND * 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et1));
    int8_t ui8_len = sprintf(tmp_buffer, "This is count %u", test_counter);
    tmp_buffer[ui8_len++] = '\0';
    test_counter++;
    lwb_input(tmp_buffer, ui8_len);
  }

  PROCESS_END();
}
