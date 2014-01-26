/*
 * kansei_scalability.h
 *
 *  Created on: Jan 20, 2012
 *      Author: fe
 */

#ifndef KANSEI_SCALABILITY_H_
#define KANSEI_SCALABILITY_H_

// configuration values for scalability experiments on Kansei
#define N_NODES_MAX  500
#define COOJA        0
#define RF_CHANNEL   26
#define TINYOS       1
#define LWB_DEBUG    0
#define TX_POWER     5
#define PRINT_PERIOD 13500  // 3.75 hours
#define EXP_LENGTH   13500  // 3.75 hours
#define INIT_IPI     10
#define INIT_PERIOD  1
#define INIT_DELAY   1800   // 0.5 hours
#define HOSTS        {284}
#define SINKS        HOSTS  // the host is also the sink
#define SOURCES      {}     // all nodes but the host are sources

#endif /* KANSEI_SCALABILITY_H_ */
