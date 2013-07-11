
#include "lwb.h"

#if USING_DEFAULT
#warning ##################################################################
#warning # LWB configuration not provided or not defined: using 'default' #
#warning ##################################################################
#endif /* DEFAULT_CONF */

#if !JOINING_NODES
#if COOJA
static const uint16_t nodes[] = {105, 106};
#else
#if TINYOS_SERIAL_FRAMES
static uint16_t nodes[] = {103,104,105,106,109,110,111,112,115,116,119,120,121,122,125,126,127,128,131,132,
    133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,
    158,159,160,161,162,163,164,167,168,169,170,171,172,173,174,179,180,182,183,184,185,186,187,188,189,
    195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,
    220,221,222,223,224,225,226,227,228,229,230,232,233,234,235,236,237,238,239,240,241,242,243,244,245,
    246,247,248,249,250,251,252,253,254,255,256,257,258,259,260,263,264,265,266,267,268,269,270,271,272,
    273,274,275,276,280,281,282,283,285,286,287,288,289,290,291,292,293,294,295,296,297,298,299,301,302,
    303,304,305,306,308,309,310,311,313,314,315,316,317,318,320,321,322,323,324,327,328,329,330,331,332,
    333,334,335,336,337,338,339,340,343,344,345,346,347,348,350,351,352,353,355,356,357,358,359,360,361,
    362,364,365,366,367,368,373,374,375,376,377,378,379,380,381,382,383,384,389,390,391,392,393,394,395,
    396,397,398,399,400,405,406,408,409,410,411,412,413,414,415,416,423,424,425,426,427,428,429,430,431,
    432,439,440,441,442,443,444,445,446,447,448,455,456,457,458,459,460,471,472,473,474,475,476};
#else
static const uint16_t nodes[] = {
    101, 102, 103, 104, 105, 106, 107, 108, 109};
//    1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
//    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
//    50, 51};
#endif /* COOJA */
#endif /* TINYOS_SERIAL_FRAMES */
#endif /* !JOINING_NODES */

static uint16_t idx, slot_idx;

static uint16_t hosts[] = HOSTS;
static uint16_t sinks[] = SINKS;
static uint16_t sources[] = SOURCES;
static uint8_t  is_host, is_sink, is_source = 0;
static uint16_t n_rcvd[N_NODES_MAX];
static uint16_t n_tot[N_NODES_MAX];
#if LATENCY
static uint16_t lat[N_NODES_MAX];
#endif /* LATENCY */
#if MOBILE || USERINT_INT
static uint16_t n_rcvd_tot[N_NODES_MAX];
static uint16_t my_n_rcvd_tot, my_n_rcvd_tot_last_round;
#endif /* MOBILE */
#if DSN_TRAFFIC_FLUCTUATIONS || DSN_INTERFERENCE || DSN_REMOVE_NODES || USERINT_INT
static uint32_t en_on[N_NODES_MAX];
static uint32_t en_tot[N_NODES_MAX];
static uint8_t  rc[N_NODES_MAX];
#else
static uint16_t dc[N_NODES_MAX];
#endif /* DSN_TRAFFIC_FLUCTUATIONS */
#if LWB_DEBUG && !COOJA
static uint16_t dc_control[N_NODES_MAX];
static uint8_t  rc[N_NODES_MAX];
#endif /* LWB_DEBUG && !COOJA */
#ifdef DSN_BOOTSTRAPPING
typedef struct {
  uint16_t id;
  uint8_t type;
} slot_type;
static slot_type slots[100];
static uint8_t slots_idx;
static uint16_t slots_time;
#endif /* DSN_BOOTSTRAPPING */

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

#if COMPRESS
static uint16_t snc[N_SLOTS_MAX], snc_tmp[N_SLOTS_MAX];
static uint8_t  first_snc, ack_snc;
#endif /* COMPRESS */

#define TIME       sched.time
#define PERIOD     sched.T
#define N_SLOTS    sched.n_slots
#define OLD_PERIOD old_sched.T

#define PERIOD_RT(T)     ((T) * (uint32_t)RTIMER_SECOND + ((int32_t)((T) * skew) / (int32_t)64))

static struct rtimer rt;
static struct pt pt_sync, pt_rr;
static sched_struct sched, old_sched;
static data_struct data;
static data_header_struct data_header;
static stream_req stream_reqs[N_STREAM_REQS_MAX];
static uint8_t n_stream_reqs;

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
static uint8_t  period;
#define GET_PERIOD(per) ((per) & 0x7f)
#define SET_COMM(comm) ((comm) << 7)
#define GET_COMM(comm) ((comm) >> 7)

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

static uint32_t time;
static uint8_t  sync_state;
static uint16_t n_streams, n_slots_to_assign;
static uint8_t  relay_cnt_cnt;
static uint16_t relay_cnt_sum;

static rtimer_clock_t T_guard;

#if DSN_TRAFFIC_FLUCTUATIONS
static const uint16_t peak_nodes[] = PEAK_NODES;
static const uint16_t t_change_ipi[] = T_CHANGE_IPI;
static const uint16_t ipis[] = IPIs;
static uint8_t is_peak_node = 0;
#endif /* DSN_TRAFFIC_FLUCTUATIONS */
#if DSN_REMOVE_NODES
static const uint16_t nodes_to_remove[] = NODES_TO_REMOVE;
static const uint16_t t_remove[] = T_REMOVE;
static const uint16_t t_add[] = T_ADD;
static uint8_t is_node_to_remove = 0;
static uint32_t en_on_to_remove = 0;
#endif /* DSN_REMOVE_NODES */
#if USERINT_INT
static uint16_t low_ipi = 0;
#endif /* USERINT_INT */

#if FLASH
static flash_struct_dy   flash_dy;
static flash_struct_dc   flash_dc;
static uint32_t flash_idx = 0;
#endif /* FLASH */

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

#if COMPRESS
#include "compress.c"
#endif /* COMPRESS */
#include "scheduler.c"
#include "g_sync.c"
#include "g_rr.c"
#include "print.c"

PROCESS(lwb_init, "LWB init");
AUTOSTART_PROCESSES(&lwb_init);
PROCESS_THREAD(lwb_init, ev, data)
{
  PROCESS_BEGIN();

  static struct etimer et;

  set_hosts_sinks_sources();

  cc2420_set_tx_power(TX_POWER);
  process_start(&print, NULL);
  process_start(&glossy_process, NULL);

#if DSN_TRAFFIC_FLUCTUATIONS
  for (idx = 0; idx < sizeof(peak_nodes) / 2; idx++) {
    if (node_id == peak_nodes[idx]) {
      is_peak_node = 1;
      break;
    }
  }
#endif /* DSN_TRAFFIC_FLUCTUATIONS */
#if DSN_REMOVE_NODES
  for (idx = 0; idx < sizeof(nodes_to_remove) / 2; idx++) {
    if (node_id == nodes_to_remove[idx]) {
      is_node_to_remove = 1;
      break;
    }
  }
#endif /* DSN_REMOVE_NODES */

#if FLASH && !COOJA
  etimer_set(&et, 2 * CLOCK_SECOND);
  PROCESS_YIELD_UNTIL(etimer_expired(&et));

  xmem_erase(FLASH_SIZE, FLASH_OFFSET);

  etimer_set(&et, 2 * CLOCK_SECOND);
  PROCESS_YIELD_UNTIL(etimer_expired(&et));
  energest_init();
#endif /* FLASH && !COOJA */

  if (is_host) {
    scheduler_init();
    SCHEDULE_L(RTIMER_NOW() - 10, INIT_PERIOD * (uint32_t)RTIMER_SECOND, g_sync);
  } else {
#if JOINING_NODES
//    if (is_source) {
//      n_stream_reqs = 1;
//      stream_reqs[0].ipi = INIT_IPI;
//      stream_reqs[0].req_type = SET_STREAM_ID(1) | (ADD_STREAM & 0x3);
//      stream_reqs[0].time_info = TIME;
//      t_last_req = 0;
//    }
    n_stream_reqs = 0;
#else
    n_stream_reqs = 0;
#endif /* JOINING_NODES */
#if STATIC
    relay_cnt_sum = 0;
    relay_cnt_cnt = 0;
#endif /* STATIC */
    SCHEDULE_L(RTIMER_NOW() - 10, INIT_PERIOD * (uint32_t)(random_rand() % RTIMER_SECOND), g_sync);
  }

#if USERINT_INT
  if (node_id == USERINT_NODE) {
    ENABLE_USERINT_INT();
    low_ipi = 0;
  }
#endif /* USERINT_INT */

  if (INIT_DELAY) {
    // set timer to reset energest values
    etimer_set(&et, INIT_DELAY * CLOCK_SECOND);
    PROCESS_YIELD_UNTIL(etimer_expired(&et));

#if !USERINT_INT
    energest_init();
    ENERGEST_ON(ENERGEST_TYPE_CPU);
#endif /* !USERINT_INT */
#if CONTROL_DC
    en_control = 0;
#endif /* CONTROL_DC */
  }

  PROCESS_END();
}

#if USERINT_INT
interrupt(PORT2_VECTOR) irq_p2(void) {
  ENERGEST_ON(ENERGEST_TYPE_IRQ);
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
  P2IFG = 0;
  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
#endif /* USERINT_INT */
