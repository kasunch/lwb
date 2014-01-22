/*
 * compress.c
 *
 *  Created on: Dec 27, 2011
 *      Author: fe
 */

#define FIRST              *((uint16_t *)sched.slot)
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

void compress_schedule(uint8_t n_slots) {
//  for (idx = 0; idx < GET_N_SLOTS(N_SLOTS); idx++) {
//    *((uint16_t *)sched.slot + idx) = snc[idx];
//  }
//  sched_size = SYNC_HEADER_LENGTH + GET_N_SLOTS(N_SLOTS) * 2;

  t_compr_start = RTIMER_NOW();

  if (n_slots > 0) {
    FIRST = snc[0];
  }
  if (n_slots < 2) {
    SET_D_L_BITS(0, 0);
    sched_size = SYNC_HEADER_LENGTH + n_slots * 2;
    t_compr_stop = RTIMER_NOW();
    return;
  }

  uint8_t  n_runs = 0;
  uint16_t d[n_slots - 1];
  d[n_runs] = snc[1] - snc[0];
  uint16_t d_max = d[n_runs];
  uint8_t  l[n_slots - 1];
  l[n_runs] = 0;
  uint8_t  l_max = l[n_runs];

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
  sched_size = SYNC_HEADER_LENGTH + 3 + (tot_bits / 8) + ((tot_bits % 8) ? 1 : 0);

  t_compr_stop = RTIMER_NOW();

//  printf("n_slots %u, n_runs %u, run_bits %u, sched_size %u\n", n_slots, n_runs, run_bits, sched_size);
//  for (idx = 0; idx < n_runs; idx++) {
//    printf("%u %u\n", d[idx], l[idx]);
//  }

}

void uncompress_schedule(void) {
//  for (idx = 0; idx < GET_N_SLOTS(N_SLOTS); idx++) {
//    snc[idx] = *((uint16_t *)sched.slot + idx);
//  }

  t_uncompr_start = RTIMER_NOW();

  uint8_t n_slots = GET_N_SLOTS(N_SLOTS);
  if (n_slots > 0) {
    snc[0] = FIRST;
  }
  if (n_slots < 2) {
    t_uncompr_stop = RTIMER_NOW();
    return;
  }

  uint8_t d_bits = GET_D_BITS();
  uint8_t l_bits = GET_L_BITS();
  uint8_t run_bits = d_bits + l_bits;

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
  }

  t_uncompr_stop = RTIMER_NOW();
}
