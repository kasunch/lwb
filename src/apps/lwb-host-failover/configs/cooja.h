/*
 * cooja.h
 *
 *  Created on: Jan 19, 2012
 *      Author: fe
 */

#ifndef COOJA_H_
#define COOJA_H_

// configuration values for simulations in Cooja
//#define N_NODES_MAX  300
//#define COOJA        1
//#define RF_CHANNEL   26
//#define TINYOS       0
//#define LWB_DEBUG    0
//#define TX_POWER     15
//#define PRINT_PERIOD 0
//#define EXP_LENGTH   0
//#define INIT_IPI     30
//#define INIT_PERIOD  1
//#define INIT_DELAY   300    // 5 minutes
//#define HOSTS        {200}
//#define SINKS        HOSTS     // all nodes are sinks
//#define SOURCES      {}     // all nodes but the host are sources
//#define MOBILE       1
//#define PERIOD_MAX   5
#define N_NODES_MAX  300
#define COOJA        1
#define RF_CHANNEL   26
#define TINYOS       0
#define LWB_DEBUG    0
#define TX_POWER     31
#define PRINT_PERIOD 0      // print after each round (if there's time before the next one)
#define EXP_LENGTH   0      // the experiment has unlimited length
#define INIT_IPI     30
#define INIT_PERIOD  1
#define INIT_DELAY   300
#define CHANNELS     {26, 15, 25}
#define HOSTS        {29,  4, 12}
#define SINKS        {29,  4, 12, 200}
#define SOURCES      {}
#define HOST_TIMEOUT 120
#define T_REMOVE_H1  {3600, 12600}
#define T_ADD_H1     {5400, 14400}
#define T_REMOVE_H2  {7200}
#define T_ADD_H2     {14400}
#define T_REMOVE_H3  {9000}
#define T_ADD_H3     {10800}

#endif /* COOJA_H_ */
