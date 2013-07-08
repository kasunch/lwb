

#include "test_flocklab_adaptors.h"

PROCESS(test_flocklab_adaptors_process, "Test Flocklab Adaptors");
AUTOSTART_PROCESSES(&test_flocklab_adaptors_process);
PROCESS_THREAD(test_flocklab_adaptors_process, ev, data)
{
	PROCESS_BEGIN();

	static struct etimer et;

	INIT_PIN_USERINT_IN;
	INIT_PIN_GIO2_IN;
	INIT_PIN_ADC0_OUT;
	INIT_PIN_ADC1_OUT;

	while (1) {
		if (PIN_USERINT_IS_SET) {
			SET_PIN_ADC0;
			leds_on(LEDS_GREEN);
		} else {
			UNSET_PIN_ADC0;
			leds_off(LEDS_GREEN);
		}
		if (PIN_GIO2_IS_SET) {
			SET_PIN_ADC1;
			leds_on(LEDS_BLUE);
		} else {
			UNSET_PIN_ADC1;
			leds_off(LEDS_BLUE);
		}
//		SET_PIN_ADC0;
//		etimer_set(&et, CLOCK_SECOND / 10);
//		PROCESS_WAIT_UNTIL(etimer_expired(&et));
//		SET_PIN_ADC1;
//		etimer_set(&et, CLOCK_SECOND / 10);
//		PROCESS_WAIT_UNTIL(etimer_expired(&et));
//		UNSET_PIN_ADC0;
//		etimer_set(&et, CLOCK_SECOND / 10);
//		PROCESS_WAIT_UNTIL(etimer_expired(&et));
//		UNSET_PIN_ADC1;
//		etimer_set(&et, CLOCK_SECOND / 10);
//		PROCESS_WAIT_UNTIL(etimer_expired(&et));
		watchdog_periodic();
	}

	PROCESS_END();
}
