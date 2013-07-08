/*
 * twist_scheduling.h
 *
 *  Created on: Feb 16, 2012
 *      Author: fe
 */

#ifndef TWIST_SCHEDULING_H_
#define TWIST_SCHEDULING_H_

// configuration values for scheduling experiments on Twist
#define N_NODES_MAX  300
#define COOJA        0
#define RF_CHANNEL   26
#define TINYOS       1
#define LWB_DEBUG    0
#define TX_POWER     15
#define PRINT_PERIOD 0
#define EXP_LENGTH   9900  // 2.75 hours
#define INIT_IPI     60
#define INIT_PERIOD  1
#define INIT_DELAY   600    // 10 minutes
#define HOSTS        {12}
#define SINKS        HOSTS  // the host is also the sink
#define SOURCES      {}     // all nodes but the host are sources
#define LATENCY      1
#define MINIMIZE_LATENCY 0
#define PERIOD_MAX 2
#endif /* TWIST_SCHEDULING_H_ */
