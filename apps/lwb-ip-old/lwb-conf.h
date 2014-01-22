/*
 * lwb-conf.h
 *
 *  Created on: Jan 19, 2012
 *      Author: fe
 */

#ifndef LWB_CONF_H_
#define LWB_CONF_H_

#if defined TWIST_MULTISINK_20 || defined TWIST_MULTISINK_50 || defined TWIST_MULTISINK_80
#include "twist_multisink.h"
#elif defined COOJA
#include "cooja.h"
#elif defined DEFAULT
#include "default.h"
#elif defined KANSEI_SCALABILITY
#include "kansei_scalability.h"
#elif defined TWIST_DECREASING_POWER
#include "twist_decreasing_power.h"
#elif defined DSN_MOBILITY_PEOPLE
#include "dsn_mobility_people.h"
#elif defined DEBUG
#include "debug.h"
#elif defined FAIRNESS
#include "fairness.h"
#elif defined DSN_ULTRA_LOW_POWER
#include "dsn_ultra_low_power.h"
#elif defined DSN_TRAFFIC_FLUCTUATIONS
#include "dsn_traffic_fluctuations.h"
#elif defined TWIST_BOOTSTRAPPING
#include "twist_bootstrapping.h"
#elif defined TWIST_SCHEDULING
#include "twist_scheduling.h"
#elif defined DSN_INTERFERENCE
#include "dsn_interference.h"
#elif defined DSN_REMOVE_NODES
#include "dsn_remove_nodes.h"
#elif defined CONET_MOBILITY
#include "conet_mobility.h"
#elif defined DSN_BOOTSTRAPPING
#include "dsn_bootstrapping.h"
#else
#define USING_DEFAULT 1
#include "default.h"
#endif

#endif /* LWB_CONF_H_ */
