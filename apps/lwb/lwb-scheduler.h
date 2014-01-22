#ifndef __LWB_SCHEDULAR_H__
#define __LWB_SCHEDULAR_H__

#include "lwb-common.h"

void lwb_sched_init(void);

void lwb_sched_compute_schedule(lwb_schedule_t* p_sched);

void lwb_sched_process_stream_req(uint16_t u16_id, lwb_stream_req_t *p_req);

#endif // __LWB_SCHEDULAR_H__
