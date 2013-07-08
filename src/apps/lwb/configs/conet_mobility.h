/*
 * conet_mobile_sink.h
 *
 *  Created on: Feb 19, 2012
 *      Author: fe
 */

#ifndef CONET_MOBILITY_H_
#define CONET_MOBILITY_H_

// configuration values for mobility experiments with on the CONET testbed
#define N_NODES_MAX  300
#define COOJA        0
#define RF_CHANNEL   26
#define TINYOS       1
#define LWB_DEBUG    0
#define TX_POWER     3
#define PRINT_PERIOD 0
#define EXP_LENGTH   0
#define INIT_IPI     4
#define INIT_PERIOD  1
#define INIT_DELAY   300    // 5 minutes
#define HOSTS        {12}
#define SINKS        HOSTS  // all nodes are sinks
#define SOURCES      {11, 13, 14, 15}     // all nodes but the host are sources
#define MOBILE       1
#define AVG_RELAY_CNT_UPDATE 1
#define RELAY_CNT_FACTOR 1
//#define PERIOD_MAX   5
#endif /* CONET_MOBILITY_H_ */
