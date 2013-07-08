/*
 * default.h
 *
 *  Created on: Jan 19, 2012
 *      Author: fe
 */

#ifndef DSN_HOST_FAILOVER_H_
#define DSN_HOST_FAILOVER_H_

// configuration values for host failover experiments
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
#define INIT_DELAY   300
#define CHANNELS     {26, 15, 25}
#define HOSTS        {29,  4, 12}
#define SINKS        {29,  4, 12, 200}
#define SOURCES      {}
#define HOST_TIMEOUT 120
#define T_REMOVE_H1  {1800, 6300}
#define T_ADD_H1     {2700, 7200}
#define T_REMOVE_H2  {3600}
#define T_ADD_H2     {7200}
#define T_REMOVE_H3  {4500}
#define T_ADD_H3     {5400}

#endif /* DSN_HOST_FAILOVER_H_ */
