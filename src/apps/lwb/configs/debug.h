/*
 * debug.h
 *
 *  Created on: Jan 19, 2012
 *      Author: fe
 */

#ifndef DEBUG_H_
#define DEBUG_H_

// debug configuration values
#define N_NODES_MAX  500
#define COOJA        0
#define RF_CHANNEL   26
#define TINYOS       1
#define LWB_DEBUG    1
#define TX_POWER     5
#define PRINT_PERIOD 0      // print after each round (if there's time before the next one)
#define EXP_LENGTH   0      // the experiment has unlimited length
#define INIT_IPI     60
#define INIT_PERIOD  1
#define INIT_DELAY   900
#define HOSTS        {283}
#define SINKS        HOSTS     // all nodes are sinks
#define SOURCES      {}     // all nodes but the host are sources
#define LONG_SLOTS   1

#endif /* DEBUG_H_ */
