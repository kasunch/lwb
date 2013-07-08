
#include "contiki.h"
#include "contiki-conf.h"
#include "dev/leds.h"
#include "watchdog.h"

#define SET_PIN(a,b)          do { P##a##OUT |=  BV(b); } while (0)
#define UNSET_PIN(a,b)        do { P##a##OUT &= ~BV(b); } while (0)
#define INIT_PIN_IN(a,b)      do { P##a##SEL &= ~BV(b); P##a##DIR &= ~BV(b); } while (0)
#define INIT_PIN_OUT(a,b)     do { P##a##SEL &= ~BV(b); P##a##DIR |=  BV(b); } while (0)
#define PIN_IS_SET(a,b)       (    P##a##IN  &   BV(b))

// UserINT (P2.7)
#define SET_PIN_USERINT      SET_PIN(2,7)
#define UNSET_PIN_USERINT    UNSET_PIN(2,7)
#define INIT_PIN_USERINT_IN  INIT_PIN_IN(2,7)
#define INIT_PIN_USERINT_OUT INIT_PIN_OUT(2,7)
#define PIN_USERINT_IS_SET   PIN_IS_SET(2,7)

// GIO2 (P2.3)
#define SET_PIN_GIO2         SET_PIN(2,3)
#define UNSET_PIN_GIO2       UNSET_PIN(2,3)
#define INIT_PIN_GIO2_IN     INIT_PIN_IN(2,3)
#define INIT_PIN_GIO2_OUT    INIT_PIN_OUT(2,3)
#define PIN_GIO2_IS_SET      PIN_IS_SET(2,3)

// ADC0 (P6.0)
#define SET_PIN_ADC0         SET_PIN(6,0)
#define UNSET_PIN_ADC0       UNSET_PIN(6,0)
#define INIT_PIN_ADC0_IN     INIT_PIN_IN(6,0)
#define INIT_PIN_ADC0_OUT    INIT_PIN_OUT(6,0)
#define PIN_ADC0_IS_SET      PIN_IS_SET(6,0)

// ADC1 (P6.1)
#define SET_PIN_ADC1         SET_PIN(6,1)
#define UNSET_PIN_ADC1       UNSET_PIN(6,1)
#define INIT_PIN_ADC1_IN     INIT_PIN_IN(6,1)
#define INIT_PIN_ADC1_OUT    INIT_PIN_OUT(6,1)
#define PIN_ADC1_IS_SET      PIN_IS_SET(6,1)

// ADC2 (P6.2) -> LED3
#define SET_PIN_ADC2         SET_PIN(6,2)
#define UNSET_PIN_ADC2       UNSET_PIN(6,2)
#define INIT_PIN_ADC2_IN     INIT_PIN_IN(6,2)
#define INIT_PIN_ADC2_OUT    INIT_PIN_OUT(6,2)
#define PIN_ADC2_IS_SET      PIN_IS_SET(6,2)

// ADC6 (P6.6) -> LED2
#define SET_PIN_ADC6         SET_PIN(6,6)
#define UNSET_PIN_ADC6       UNSET_PIN(6,6)
#define INIT_PIN_ADC6_IN     INIT_PIN_IN(6,6)
#define INIT_PIN_ADC6_OUT    INIT_PIN_OUT(6,6)
#define PIN_ADC6_IS_SET      PIN_IS_SET(6,6)

// ADC7 (P6.7) -> LED1
#define SET_PIN_ADC7         SET_PIN(6,7)
#define UNSET_PIN_ADC7       UNSET_PIN(6,7)
#define INIT_PIN_ADC7_IN     INIT_PIN_IN(6,7)
#define INIT_PIN_ADC7_OUT    INIT_PIN_OUT(6,7)
#define PIN_ADC7_IS_SET      PIN_IS_SET(6,7)
