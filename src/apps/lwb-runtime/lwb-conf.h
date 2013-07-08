/*
 * lwb-conf.h
 *
 *  Created on: Jan 19, 2012
 *      Author: fe
 */

#ifndef LWB_CONF_H_
#define LWB_CONF_H_

#ifdef COOJA
#include "cooja.h"
#elif defined DEFAULT
#include "default.h"
#elif defined DEBUG
#include "debug.h"
#else
#define USING_DEFAULT 1
#include "default.h"
#endif

#endif /* LWB_CONF_H_ */
