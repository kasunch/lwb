
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
static uint16_t pkt_rcvd[N_NODES_MAX];
static uint16_t pkt_tot[N_NODES_MAX];
static uint8_t  pkt_buf[N_NODES_MAX];

static uint16_t dc[N_NODES_MAX];
static uint16_t ps[N_NODES_MAX];
static uint16_t Ts[N_NODES_MAX];
static uint16_t Td[N_NODES_MAX];
static uint16_t sched_tot, sched_rcvd = 0;
static uint32_t Td_tot = 0;
static uint16_t Td_cnt = 0;
static uint32_t Ts_tot = 0;
static uint16_t Ts_cnt = 0;

static pkt_info delivered_pkt[N_SLOTS_MAX];

static uint64_t ack, cumulative_ack;
static uint8_t  n_sinks;
static uint16_t curr_sinks[N_SINKS_MAX];

void set_hosts_sinks_sources(void) {
	// initialize arrays for statistics on received packets
	memset(pkt_rcvd, 0, sizeof(pkt_rcvd));
	memset(pkt_tot, 0, sizeof(pkt_tot));

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

MEMB(streams_memb, stream_info, N_STREAMS_MAX);
LIST(streams_list);
MEMB(slots_memb, slots_info, N_SLOTS_MAX);
LIST(slots_list);
MEMB(sinks_memb, sinks_info, N_SINKS_MAX);
LIST(sinks_list);
MEMB(rcvd_memb, rcvd_pkt_info, N_SLOTS_MAX);
LIST(rcvd_list);
MEMB(already_rcvd_memb, rcvd_pkt_info, N_SLOTS_MAX);
LIST(already_rcvd_list);

static uint16_t snc[N_SLOTS_MAX];
static uint8_t  stream_id[N_SLOTS_MAX];
static uint16_t pkt_id[N_SLOTS_MAX];
static uint8_t  s_ack;

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
static uint8_t  last_rcvd_seq_no = 0;
static uint16_t last_time = 0;
static rtimer_clock_t T_guard;

static uint8_t  joining_state;
static uint8_t  n_joining_trials = 0;
static uint8_t  rounds_to_wait;
static uint32_t last_free_slot_time;
static uint32_t last_free_reqs_time;

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

static uint32_t en_control, en_rx, en_tx = 0;
static inline void save_energest_values(void) {
	// store current Rx and Tx energest values (before Glossy starts)
	en_rx = energest_type_time(ENERGEST_TYPE_LISTEN);
	en_tx = energest_type_time(ENERGEST_TYPE_TRANSMIT);
}
static inline void update_control_dc(void) {
	// add the energy spent to the control energy part (after Glossy finishes)
	en_control += (energest_type_time(ENERGEST_TYPE_LISTEN) - en_rx) +
			(energest_type_time(ENERGEST_TYPE_TRANSMIT) - en_tx);
}

static inline void update_Td(void) {
	Td_tot += (energest_type_time(ENERGEST_TYPE_LISTEN) - en_rx) +
			(energest_type_time(ENERGEST_TYPE_TRANSMIT) - en_tx);
	Td_cnt++;
	if (Td_cnt >= DATA_THR) {
		Td_tot /= 2;
		Td_cnt /= 2;
	}
}

static inline void update_Ts(void) {
	Ts_tot += (energest_type_time(ENERGEST_TYPE_LISTEN) - en_rx) +
			(energest_type_time(ENERGEST_TYPE_TRANSMIT) - en_tx) - T_guard;
	Ts_cnt++;
	if (Ts_cnt >= SCHED_THR) {
		Ts_tot /= 2;
		Ts_cnt /= 2;
	}
}

static uint32_t time;
static uint8_t  sync_state;
static uint16_t n_streams, n_slots_to_assign;
static uint8_t  relay_cnt_cnt;
static uint16_t relay_cnt_sum;

stream_info* get_stream_from_id(uint16_t id) {
	stream_info *curr_stream;
	for (curr_stream = list_head(streams_list); curr_stream != NULL; curr_stream = curr_stream->next) {
		if (curr_stream->node_id == id) {
			return curr_stream;
		}
	}
	return NULL;
}

#define IS_LESS_EQ_THAN_TIME(a)      ((int16_t)(a-(uint16_t)time) <= 0)
#define IS_LESS_EQ_THAN_TIME_LONG(a) ((int32_t)(a-time) <= 0)

#define IS_STREAM_TO_ADD(a) ((a & 0x3) == ADD_STREAM)
#define IS_STREAM_TO_DEL(a) ((a & 0x3) == DEL_STREAM)
#define IS_STREAM_TO_REP(a) ((a & 0x3) == REP_STREAM)
#define IS_DATA_SINK(a)     ((a & 0x3) == DATA_SINK)
#define GET_STREAM_ID(a)    (a >> 2)
#define SET_STREAM_ID(a)    (a << 2)
#define GET_N_FREE(a)       (a >> 6)
#define SET_N_FREE(a)       (a << 6)
#define GET_N_SLOTS(a)      (a & 0x3f)

#include "compress.c"
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

	if (is_host) {
		scheduler_init();
		SCHEDULE_L(RTIMER_NOW() - 10, INIT_PERIOD * (uint32_t)RTIMER_SECOND, g_sync);
	} else {
		n_stream_reqs = 0;
		SCHEDULE_L(RTIMER_NOW() - 10, INIT_PERIOD * (uint32_t)(random_rand() % RTIMER_SECOND), g_sync);
	}

	if (INIT_DELAY) {
		// set timer to reset energest values
		etimer_set(&et, (INIT_DELAY - 1) * CLOCK_SECOND);
		PROCESS_YIELD_UNTIL(etimer_expired(&et));

		energest_init();
		ENERGEST_ON(ENERGEST_TYPE_CPU);
		en_control = 0;
		Td_cnt = 0; Td_tot = 0;
		Ts_cnt = 0; Ts_tot = 0;
	}

	PROCESS_END();
}
