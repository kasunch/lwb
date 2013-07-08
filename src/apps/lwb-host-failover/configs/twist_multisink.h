/*
 * twist_multisink.h
 *
 *  Created on: Jan 19, 2012
 *      Author: fe
 */

#ifndef TWIST_MULTISINK_H_
#define TWIST_MULTISINK_H_

// common configuration values for multi-sink experiments on Twist
#define N_NODES_MAX  500
#define COOJA        0
#define RF_CHANNEL   26
#define TINYOS       1
#define LWB_DEBUG    0
#define TX_POWER     15
#define PRINT_PERIOD 0
#define EXP_LENGTH   12600  // 3.5 hours
#define INIT_IPI     60
#define INIT_PERIOD  30
#define INIT_DELAY   600    // 10 minutes
#define HOSTS        {200}
#define SINKS        {97, 141, 151, 186, 200, 202, 240, 262} // 97 141 151 186 200 202 240 262

// specific configuration values for Twist multi-sink-*
#if TWIST_MULTISINK_20
#define SOURCES      {10, 12, 84, 91, 92, 99, 101, 142, 150, 185, 191, 206, 209, 216, 223, 229, 241, 252}
#elif TWIST_MULTISINK_50
#define SOURCES      {10, 81, 82, 85, 86, 88, 90, 91, 92, 95, 101, 137, 139, 143, 144, 145, 147, 150, 153, 185, 187, 189, 191, 192, 195, 197, 198, 199, 206, 207, 208, 209, 211, 213, 216, 221, 222, 223, 224, 228, 231, 241, 249, 250, 252}
#elif TWIST_MULTISINK_80
#define SOURCES      {10, 12, 15, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 95, 96, 99, 100, 101, 102, 103, 137, 139, 140, 142, 143, 144, 145, 146, 147, 148, 149, 150, 152, 153, 154, 185, 187, 188, 189, 190, 191, 192, 193, 196, 197, 198, 199, 204, 205, 206, 207, 209, 211, 213, 215, 216, 218, 220, 221, 222, 223, 224, 228, 229, 230, 241, 249, 250, 252}
#endif

#endif /* TWIST_MULTISINK_H_ */
