#ifndef __LWB_SCHED_COMPRESSOR_H__
#define __LWB_SCHED_COMPRESSOR_H__

#define TEST_LWB_SCHED_COMPRESSOR 0

#if TEST_LWB_SCHED_COMPRESSOR

#include <stdint.h>
#define LWB_SCHED_HEADER_LEN    6

#define LWB_SCHED_MAX_SLOTS      50

typedef struct lwb_sched_info {
    uint16_t ui16_host_id;
    uint16_t ui16_time;
    uint8_t  ui8_n_slots;
    uint8_t  ui8_T;
} lwb_sched_info_t;


typedef struct lwb_schedule {
    lwb_sched_info_t    sched_info;
    uint16_t            ui16arr_slots[LWB_SCHED_MAX_SLOTS];
} lwb_schedule_t;

#else
#include "lwb-common.h"
#endif

/// @brief Compress LWB schedule into byte buffer.
/// @details Schedule information is copied to the buffer from this function.
/// @param p_shed A pointer to the schedule to be compressed
/// @param p_buf A buffer to hold compressed schedule
/// @param ui8_buf_len The size of the buffer
/// @return The size of the compressed schedule. Zero is returned in case of an error
uint8_t lwb_sched_compress(lwb_schedule_t *p_sched, uint8_t* p_buf, uint8_t ui8_buf_len);

/// @brief Decompress LWB schedule from a byte buffer.
/// @details Schedule information has to be already in p_sched.
/// @param p_shed A pointer to the schedule to hold the decompressed schedule.
/// @param p_buf The buffer that holds the compressed schedule.
/// @param ui8_buf_len The size of the buffer compressed schedule
/// @return non-zero if no error occurred.
uint8_t lwb_sched_decompress(lwb_schedule_t *p_sched, uint8_t* p_buf, uint8_t ui8_buf_len);

#endif // __LWB_SHED_COMPRESSOR_H__
