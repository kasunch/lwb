/*
 * twist_bootstrapping.h
 *
 *  Created on: Feb 9, 2012
 *      Author: fe
 */

#ifndef TWIST_BOOTSTRAPPING_H_
#define TWIST_BOOTSTRAPPING_H_

// configuration values for bootstrapping experiments on Twist
#define N_NODES_MAX  300
#define COOJA        0
#define RF_CHANNEL   26
#define TINYOS       1
#define LWB_DEBUG    0
#define TX_POWER     15
#define PRINT_PERIOD 2700   // 45 minutes
#define EXP_LENGTH   2700   // 45 minutes
#define INIT_IPI     60
#define INIT_PERIOD  1
#define INIT_DELAY   0
#define HOSTS        {12}
#define SINKS        HOSTS  // the host is also the sink
#define SOURCES      {}     // all nodes but the host are sources
#define FLASH        1      // store logs to the flash

#endif /* TWIST_BOOTSTRAPPING_H_ */
