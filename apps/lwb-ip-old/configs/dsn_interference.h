/*
 * dsn_interference.h
 *
 *  Created on: Feb 17, 2012
 *      Author: fe
 */

#ifndef DSN_INTERFERENCE_H_
#define DSN_INTERFERENCE_H_

// configuration values for interference experiments on DSN
#define N_NODES_MAX  300
#define COOJA        0
#define RF_CHANNEL   20
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

#endif /* DSN_INTERFERENCE_H_ */
