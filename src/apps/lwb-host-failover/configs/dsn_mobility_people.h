/*
 * dsn_mobility_people.h
 *
 *  Created on: Jan 23, 2012
 *      Author: fe
 */

#ifndef DSN_MOBILITY_PEOPLE_H_
#define DSN_MOBILITY_PEOPLE_H_

#define N_NODES_MAX  500
#define COOJA        0
#define RF_CHANNEL   26
#define TINYOS       0
#define LWB_DEBUG    0
#define TX_POWER     31
#define PRINT_PERIOD 0
#define EXP_LENGTH   0
#define LOW_IPI      10
#define HIGH_IPI     120
#define INIT_IPI     HIGH_IPI
#define INIT_PERIOD  1
#define INIT_DELAY   1800
#define HOSTS        {200}
#define SINKS        HOSTS  // the host is also the sink
#define SOURCES      {}     // all nodes but the host are sources
#define USERINT_INT  1

#endif /* DSN_MOBILITY_PEOPLE_H_ */
