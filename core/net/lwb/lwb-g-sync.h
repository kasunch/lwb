#ifndef __LWB_G_SYNC_H__
#define __LWB_G_SYNC_H__

/// @file lwb-g-sync.h
/// @brief Header file for Glossy synchronization.

#include "contiki.h"
#include "lwb-common.h"

/// @brief Proto thread for Glossy synchronization and sending schedule at host.
PT_THREAD(lwb_g_sync_host(struct rtimer *t, lwb_context_t *p_context));

/// @brief Proto thread for Glossy synchronization and receiving schedule at source.
PT_THREAD(lwb_g_sync_source(struct rtimer *t, lwb_context_t *p_context));

/// @brief Initialize Glossy synchronization.
void lwb_g_sync_init();

#endif // __LWB_G_SYNC_H__
