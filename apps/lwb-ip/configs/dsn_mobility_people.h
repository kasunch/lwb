/*
 * dsn_mobility_people.h
 *
 *  Created on: Jan 23, 2012
 *      Author: fe
 */

#ifndef DSN_MOBILITY_PEOPLE_H_
#define DSN_MOBILITY_PEOPLE_H_

#define N_NODES_MAX  300
#define COOJA        0
#define RF_CHANNEL   26
#define TINYOS       0
#define LWB_DEBUG    0
#define TX_POWER     31
#define PRINT_PERIOD 0
#define EXP_LENGTH   0
#define LOW_IPI      1
#define HIGH_IPI     300
#define INIT_IPI     HIGH_IPI
#define INIT_PERIOD  1
#define INIT_DELAY   60
#define HOSTS        {200}
#define SINKS        {}     // all nodes are sinks
#define SOURCES      {}     // all nodes but the host are sources
#define AVG_RELAY_CNT_UPDATE 2
#define RELAY_CNT_FACTOR 16
#define USERINT_INT  1
#define USERINT_NODE 105
#define IS_MOBILE_NODE (node_id > 100)
#define ENERGEST_DIV 16

#endif /* DSN_MOBILITY_PEOPLE_H_ */