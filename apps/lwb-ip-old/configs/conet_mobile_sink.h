/*
 * conet_mobile_sink.h
 *
 *  Created on: Feb 19, 2012
 *      Author: fe
 */

#ifndef CONET_MOBILE_SINK_H_
#define CONET_MOBILE_SINK_H_

// configuration values for experiments with mobile sink on the Conet testbed
#define N_NODES_MAX  500
#define COOJA        0
#define RF_CHANNEL   26
#define TINYOS       0
#define LWB_DEBUG    0
#define TX_POWER     31
#define PRINT_PERIOD 0
#define EXP_LENGTH   3300   // 55 minutes
#define INIT_IPI     30
#define INIT_PERIOD  1
#define INIT_DELAY   300    // 5 minutes
#define HOSTS        {200}
#define SINKS        {}     // all nodes are sinks
#define SOURCES      {}     // all nodes but the host are sources
#define MOBILE       1

#endif /* CONET_MOBILE_SINK_H_ */
