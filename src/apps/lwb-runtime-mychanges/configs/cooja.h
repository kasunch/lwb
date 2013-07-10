/*
 * cooja.h
 *
 *  Created on: Jan 19, 2012
 *      Author: fe
 */

#ifndef COOJA_H_
#define COOJA_H_

// configuration values for simulations in Cooja
#define N_NODES_MAX  300
#define COOJA        1
#define RF_CHANNEL   26
#define TINYOS       0
#define LWB_DEBUG    0
#define TX_POWER     7
#define PRINT_PERIOD 0
#define EXP_LENGTH   0
#define INIT_IPI     2
#define INIT_PERIOD  1
#define INIT_DELAY   300    // 5 minutes
#define HOSTS        {200}
#define SINKS        {200, 3, 201}    // all nodes are sinks
#define SOURCES      {}     // all nodes but the hosts and the sinks are sources

#endif /* COOJA_H_ */
