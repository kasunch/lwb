/*
 * default.h
 *
 *  Created on: Jan 19, 2012
 *      Author: fe
 */

#ifndef DEFAULT_H_
#define DEFAULT_H_

// default configuration values
#define N_NODES_MAX  500
#define COOJA        0
#define RF_CHANNEL   26
#define TINYOS       0
#define LWB_DEBUG    0
#define TX_POWER     31
#define PRINT_PERIOD 0      // print after each round (if there's time before the next one)
#define EXP_LENGTH   0      // the experiment has unlimited length
#define INIT_IPI     5
#define INIT_PERIOD  1
#define INIT_DELAY   300
#define HOSTS        {51}
#define SINKS        HOSTS  // the host is also the sink
#define SOURCES      {}     // all nodes but the host are sources

#endif /* DEFAULT_H_ */
