/*
 * dsn_bootstrapping.h
 *
 *  Created on: Mar 18, 2012
 *      Author: fe
 */

#ifndef DSN_BOOTSTRAPPING_H_
#define DSN_BOOTSTRAPPING_H_

// configuration values for bootstrapping experiments on DSN
#define N_NODES_MAX  300
#define COOJA        0
#define RF_CHANNEL   26
#define TINYOS       0
#define LWB_DEBUG    0
#define TX_POWER     31
#define PRINT_PERIOD 0
#define EXP_LENGTH   0
#define INIT_IPI     6
#define INIT_PERIOD  1
#define INIT_DELAY   0
#define HOSTS        {200}
#define SINKS        HOSTS  // the host is also the sink
#define SOURCES      {4, 7, 9, 14, 15, 33}

#endif /* DSN_BOOTSTRAPPING_H_ */
