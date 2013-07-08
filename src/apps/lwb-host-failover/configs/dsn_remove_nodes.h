/*
 * dsn_remove_nodes.h
 *
 *  Created on: Feb 15, 2012
 *      Author: fe
 */

#ifndef DSN_REMOVE_NODES_H_
#define DSN_REMOVE_NODES_H_

// configuration values for removing nodes experiments on DSN
#define N_NODES_MAX  300
#define COOJA        0
#define RF_CHANNEL   26
#define TINYOS       0
#define LWB_DEBUG    0
#define TX_POWER     31
#define PRINT_PERIOD 0      // print after each round (if there's time before the next one)
#define EXP_LENGTH   0      // the experiment has unlimited length
#define INIT_IPI     60
#define INIT_PERIOD  1
#define INIT_DELAY   300    // 5 minutes
#define HOSTS        {200}
#define SINKS        HOSTS  // the host is also the sink
#define SOURCES      {}     // all nodes but the host are sources
#define NODES_TO_REMOVE {3, 27, 28, 34, 56, 59, 60, 61}
#define T_REMOVE     {1800, 4500}
#define T_ADD        {2700, 5400}

#endif /* DSN_REMOVE_NODES_H_ */
