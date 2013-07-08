/*
 * dsn_traffic_fluctuations.h
 *
 *  Created on: Feb 3, 2012
 *      Author: fe
 */

#ifndef DSN_TRAFFIC_FLUCTUATIONS_H_
#define DSN_TRAFFIC_FLUCTUATIONS_H_

// configuration values for traffic fluctuations experiments on DSN
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
#define PEAK_NODES   {1, 5, 6, 11, 16, 18, 20, 21, 22, 23, 24, 25, 26, 30}
#define T_CHANGE_IPI {3600, 4500, 6300, 7200, 9000}
#define IPIs         {   5,   60,    2,   60}

#endif /* DSN_TRAFFIC_FLUCTUATIONS_H_ */
