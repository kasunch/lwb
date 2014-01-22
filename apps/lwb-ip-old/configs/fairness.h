/*
 * debug.h
 *
 *  Created on: Jan 19, 2012
 *      Author: fe
 */

#ifndef FAIRNESS_H_
#define FAIRNESS_H_

// configuration values for fairness experiments
#define COOJA        0
#define RF_CHANNEL   26
#define TINYOS       0
#define LWB_DEBUG    1
#define TX_POWER     31
#define PRINT_PERIOD 0      // print after each round (if there's time before the next one)
#define EXP_LENGTH   0      // the experiment has unlimited length
#define INIT_IPI     4
#define INIT_PERIOD  1
#define INIT_DELAY   60
#define HOSTS        {200}
#define SINKS        HOSTS
#define SOURCES      {4, 7, 8, 9, 12, 14, 15, 29, 33}
#define FAIRNESS     1
#define TIME_SCALE   16     // set the time unit for the IPI to 1/16 seconds

#endif /* FAIRNESS_H_ */
