/*
 * compress.c
 *
 *  Created on: Dec 27, 2011
 *      Author: fe
 */

#define GET_D_BITS()       (sched.slot[2] / 8)
#define GET_L_BITS()       (sched.slot[2] % 8)
#define SET_D_L_BITS(d, l) (sched.slot[2] = (d * 8) | (l % 8))
#define COMPR_SLOT(a)      (sched.slot[3 + a])

static rtimer_clock_t t_compr_start, t_compr_stop, t_uncompr_start, t_uncompr_stop;

static inline uint8_t n_bits(uint16_t a) {
	for (idx = 15; idx > 0; idx--) {
		if (a & (1 << idx)) {
			return idx + 1;
		}
	}
	return idx + 1;
}

static inline void set_compr_sched(uint8_t pos, uint8_t run_bits, uint32_t run_info) {
	uint32_t tmp;
	uint16_t offset_bits = run_bits * pos;
	tmp = COMPR_SLOT(offset_bits / 8) | ((uint32_t)COMPR_SLOT(offset_bits / 8 + 1) << 8) | ((uint32_t)COMPR_SLOT(offset_bits / 8 + 2) << 16) | ((uint32_t)COMPR_SLOT(offset_bits / 8 + 3) << 24);
	tmp |= (run_info << (offset_bits % 8));
	COMPR_SLOT(offset_bits / 8)     = tmp;
	COMPR_SLOT(offset_bits / 8 + 1) = tmp >> 8;
	COMPR_SLOT(offset_bits / 8 + 2) = tmp >> 16;
	COMPR_SLOT(offset_bits / 8 + 3) = tmp >> 24;
}

static inline uint32_t get_compr_sched(uint16_t pos, uint8_t run_bits) {
	uint32_t tmp;
	uint16_t offset_bits = run_bits * pos;
	tmp = COMPR_SLOT(offset_bits / 8) | ((uint32_t)COMPR_SLOT(offset_bits / 8 + 1) << 8) | ((uint32_t)COMPR_SLOT(offset_bits / 8 + 2) << 16) | ((uint32_t)COMPR_SLOT(offset_bits / 8 + 3) << 24);
	return ((tmp >> (offset_bits % 8)) & ((1 << run_bits) - 1));
}

static inline void set_compr_stream_id(uint8_t tot_bytes, uint8_t pos, uint8_t bits, uint8_t id) {
	uint16_t tmp;
	uint16_t offset_bits = bits * pos;
	tmp = sched.slot[tot_bytes + offset_bits / 8] | ((uint16_t)sched.slot[tot_bytes + offset_bits / 8 + 1] << 8);
	tmp |= (id << (offset_bits % 8));
	sched.slot[tot_bytes + offset_bits / 8]     = tmp;
	sched.slot[tot_bytes + offset_bits / 8 + 1] = tmp >> 8;
}

static inline uint8_t get_compr_stream_id(uint8_t tot_bytes, uint8_t pos, uint8_t bits) {
	uint16_t tmp;
	uint16_t offset_bits = bits * pos;
	tmp = sched.slot[tot_bytes + offset_bits / 8] | ((uint16_t)sched.slot[tot_bytes + offset_bits / 8 + 1] << 8);
	return (uint8_t)((tmp >> (offset_bits % 8)) & ((1 << bits) - 1));
}

static inline void set_compr_pkt_id(uint8_t tot_bytes, uint8_t pos, uint8_t bits, uint16_t id) {
	uint32_t tmp;
	uint16_t offset_bits = bits * pos;
	tmp = sched.slot[tot_bytes + offset_bits / 8] | ((uint32_t)sched.slot[tot_bytes + offset_bits / 8 + 1] << 8) | ((uint32_t)sched.slot[tot_bytes + offset_bits / 8 + 2] << 16) | ((uint32_t)sched.slot[tot_bytes + offset_bits / 8 + 3] << 24);
	tmp |= (id << (offset_bits % 8));
	sched.slot[tot_bytes + offset_bits / 8]     = tmp;
	sched.slot[tot_bytes + offset_bits / 8 + 1] = tmp >> 8;
	sched.slot[tot_bytes + offset_bits / 8 + 2] = tmp >> 16;
	sched.slot[tot_bytes + offset_bits / 8 + 3] = tmp >> 24;
}

static inline uint16_t get_compr_pkt_id(uint8_t tot_bytes, uint8_t pos, uint8_t bits) {
	uint32_t tmp;
	uint16_t offset_bits = bits * pos;
	tmp = sched.slot[tot_bytes + offset_bits / 8] | ((uint32_t)sched.slot[tot_bytes + offset_bits / 8 + 1] << 8) | ((uint32_t)sched.slot[tot_bytes + offset_bits / 8 + 2] << 16) | ((uint32_t)sched.slot[tot_bytes + offset_bits / 8 + 3] << 24);
	return (uint16_t)((tmp >> (offset_bits % 8)) & ((1 << bits) - 1));
}

void compress_schedule(uint8_t n_slots) {
	t_compr_start = RTIMER_NOW();

	uint8_t  tot_bytes = 0;
	uint8_t  n_runs = 0;
	uint16_t d[n_slots - 1];
	d[n_runs] = snc[1] - snc[0];
	uint16_t d_max = d[n_runs];
	uint8_t  l[n_slots - 1];
	l[n_runs] = 0;
	uint8_t  l_max = l[n_runs];
	uint8_t  s_max = 0;
	uint8_t  s_min = 0xff;
	uint16_t p_max = 0;
	uint16_t p_min = 0xffff;

	if (n_slots > 0) {
		sched.slot[0] = snc[0] & 0xff;
		sched.slot[1] = (snc[0] >> 8) & 0xff;
	}
	if (n_slots < 2) {
		SET_D_L_BITS(0, 0);
		tot_bytes = n_slots * 2;
		goto set_stream_id;
	}

	for (idx = 1; idx < n_slots - 1; idx++) {
		if (snc[idx + 1] - snc[idx] == d[n_runs]) {
			l[n_runs]++;
		} else {
			if (l[n_runs] > l_max) {
				l_max = l[n_runs];
			}
			n_runs++;
			d[n_runs] = snc[idx + 1] - snc[idx];
			if (d[n_runs] > d_max) {
				d_max = d[n_runs];
			}
			l[n_runs] = 0;
		}
	}
	if (l[n_runs] > l_max) {
		l_max = l[n_runs];
	}
	n_runs++;

	uint8_t d_bits = n_bits(d_max);
	uint8_t l_bits = n_bits(l_max);
	uint8_t run_bits = d_bits + l_bits;

	for (idx = 0; idx < n_runs; idx++) {
		set_compr_sched(idx, run_bits, ((uint32_t)d[idx] << l_bits) | l[idx]);
	}
	SET_D_L_BITS(d_bits, l_bits);
	uint16_t tot_bits = (uint16_t)n_runs * run_bits;
	tot_bytes = 3 + (tot_bits / 8) + ((tot_bits % 8) ? 1 : 0);

set_stream_id:
	for (idx = 0; idx < n_slots; idx++) {
		if (stream_id[idx] > s_max) {
			s_max = stream_id[idx];
		}
		if (stream_id[idx] < s_min) {
			s_min = stream_id[idx];
		}
		if (pkt_id[idx] > p_max) {
			p_max = pkt_id[idx];
		}
		if (pkt_id[idx] < p_min) {
			p_min = pkt_id[idx];
		}
	}
	if (n_slots) {
		if (s_min == s_max) {
			// all slots are for streams that have the same stream id
			sched.slot[tot_bytes] = (1 << 7) | s_min;
			tot_bytes++;
		} else {
			uint8_t s_bits = n_bits(s_max);
			sched.slot[tot_bytes] = s_bits;
			tot_bytes++;
			for (idx = 0; idx < n_slots; idx++) {
				set_compr_stream_id(tot_bytes, idx, s_bits, stream_id[idx]);
			}
			uint16_t tot_bits = (uint16_t)n_slots * s_bits;
			tot_bytes += (tot_bits / 8) + ((tot_bits % 8) ? 1 : 0);
		}

		if (p_min == p_max) {
			// all slots are for streams that have the same packet id
			sched.slot[tot_bytes] = (1 << 7) | (uint8_t)((p_min >> 8) & 0x7f);
			sched.slot[tot_bytes+1] = (uint8_t)(p_min & 0xff);
			tot_bytes += 2;
		} else {
			uint8_t p_bits = n_bits(p_max-p_min);
			sched.slot[tot_bytes] = p_bits;
			tot_bytes++;
			sched.slot[tot_bytes] = (uint8_t)((p_min >> 8) & 0x7f);
			sched.slot[tot_bytes+1] = (uint8_t)(p_min & 0xff);
			tot_bytes += 2;
			for (idx = 0; idx < n_slots; idx++) {
				set_compr_pkt_id(tot_bytes, idx, p_bits, pkt_id[idx] - p_min);
			}
			uint16_t tot_bits = (uint16_t)n_slots * p_bits;
			tot_bytes += (tot_bits / 8) + ((tot_bits % 8) ? 1 : 0);
		}
	}

	for (idx = 0; idx < n_sinks; idx++) {
		sched.slot[tot_bytes] = curr_sinks[idx] & 0xff;
		sched.slot[tot_bytes+1] = (curr_sinks[idx] >> 8) & 0xff;
		tot_bytes += 2;
	}

	sched_size = SYNC_HEADER_LENGTH + tot_bytes;

	t_compr_stop = RTIMER_NOW();
}

void uncompress_schedule(uint8_t data_len) {
	t_uncompr_start = RTIMER_NOW();

	uint8_t tot_bytes;
	uint8_t n_slots = GET_N_SLOTS(N_SLOTS);
	if (n_slots > 0) {
		snc[0] = sched.slot[0] | ((uint16_t)sched.slot[1] << 8);
	}
	if (n_slots < 2) {
		tot_bytes = n_slots * 2;
		goto get_stream_id;
	}

	uint8_t d_bits = GET_D_BITS();
	uint8_t l_bits = GET_L_BITS();
	uint8_t run_bits = d_bits + l_bits;

	uint8_t n_runs = 0;
	slot_idx = 1;
	for (idx = 0; slot_idx < n_slots; idx++) {
		uint32_t run_info = get_compr_sched(idx, run_bits);
		uint16_t d = run_info >> l_bits;
		uint8_t  l = run_info & ((1 << l_bits) - 1);
		uint8_t  i;
		for (i = 0; i < l + 1; i++) {
			snc[slot_idx] = snc[slot_idx - 1] + d;
			slot_idx++;
		}
		n_runs++;
	}
	uint16_t tot_bits = (uint16_t)n_runs * run_bits;
	tot_bytes = 3 + (tot_bits / 8) + ((tot_bits % 8) ? 1 : 0);

get_stream_id:
	if (n_slots) {
		if (sched.slot[tot_bytes] & 0x80) {
			uint8_t s_min = sched.slot[tot_bytes] & 0x7f;
			tot_bytes++;
			for (idx = 0; idx < n_slots; idx++) {
				stream_id[idx] = s_min;
			}
		} else {
			uint8_t s_bits = sched.slot[tot_bytes] & 0x7f;
			tot_bytes++;
			for (idx = 0; idx < n_slots; idx++) {
				stream_id[idx] = get_compr_stream_id(tot_bytes, idx, s_bits);
			}
			uint16_t tot_bits = (uint16_t)n_slots * s_bits;
			tot_bytes += (tot_bits / 8) + ((tot_bits % 8) ? 1 : 0);
		}

		if (sched.slot[tot_bytes] & 0x80) {
			uint16_t p_min = (((uint16_t)sched.slot[tot_bytes] & 0x7f) << 8) | (sched.slot[tot_bytes+1]);
			tot_bytes += 2;
			for (idx = 0; idx < n_slots; idx++) {
				pkt_id[idx] = p_min;
			}
		} else {
			uint8_t p_bits = sched.slot[tot_bytes] & 0x7f;
			tot_bytes++;
			uint16_t p_min = ((uint16_t)sched.slot[tot_bytes] << 8) | (sched.slot[tot_bytes+1]);
			tot_bytes += 2;
			for (idx = 0; idx < n_slots; idx++) {
				pkt_id[idx] = p_min + get_compr_pkt_id(tot_bytes, idx, p_bits);
			}
			uint16_t tot_bits = (uint16_t)n_slots * p_bits;
			tot_bytes += (tot_bits / 8) + ((tot_bits % 8) ? 1 : 0);
		}
	}

	n_sinks = (data_len - (tot_bytes + SYNC_HEADER_LENGTH)) / 2;
	for (idx = 0; idx < n_sinks; idx++) {
		curr_sinks[idx] = sched.slot[tot_bytes] | ((uint16_t)sched.slot[tot_bytes+1] << 8);
		tot_bytes += 2;
	}

	t_uncompr_stop = RTIMER_NOW();
}
