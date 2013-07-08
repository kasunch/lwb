/*
 * twist_decreasing_power.h
 *
 *  Created on: Jan 22, 2012
 *      Author: fe
 */

#ifndef TWIST_DECREASING_POWER_H_
#define TWIST_DECREASING_POWER_H_

// configuration values for experiments with decreasing transmit power on Twist
#define N_NODES_MAX  300
#define COOJA        0
#define RF_CHANNEL   26
#define TINYOS       1
#define LWB_DEBUG    0
#define TX_POWER     7
#define PRINT_PERIOD 0
#define EXP_LENGTH   12600  // 3.5 hours
#define INIT_IPI     120
#define INIT_PERIOD  1
#define INIT_DELAY   600    // 10 minutes
#define HOSTS        {12}
#define SINKS        HOSTS  // the host is also the sink
#define SOURCES      {}     // all nodes but the host are sources

#endif /* TWIST_DECREASING_POWER_H_ */
