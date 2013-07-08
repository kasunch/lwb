#include "../test_flocklab_adaptors/test_flocklab_adaptors.h"

PROCESS(blink_flocklab_adaptors_process, "Blink Flocklab Adaptors");
AUTOSTART_PROCESSES(&blink_flocklab_adaptors_process);
PROCESS_THREAD(blink_flocklab_adaptors_process, ev, data)
{
	PROCESS_BEGIN();

	static struct etimer et;

	INIT_PIN_ADC2_OUT; // LED3 - BLUE
	INIT_PIN_ADC6_OUT; // LED2 - GREEN
	INIT_PIN_ADC7_OUT; // LED1 - RED


	/* Let the LEDs of the FlockLab adaports blink similar
	 * to TinyOS Blink, i.e., count from 0 to 7 by
	 *  - toggle LED1 every 250 ms
	 *  - toggle LED2 every 500 ms
	 *  - toggle LED3 every 1000 ms
	 */

	while (1) {
		etimer_set(&et, CLOCK_SECOND / 4);
		PROCESS_WAIT_UNTIL(etimer_expired(&et));
		SET_PIN_ADC7;   // LED1
		etimer_set(&et, CLOCK_SECOND / 4);
		PROCESS_WAIT_UNTIL(etimer_expired(&et));
		UNSET_PIN_ADC7; // LED1
		SET_PIN_ADC6;   // LED2
		etimer_set(&et, CLOCK_SECOND / 4);
		PROCESS_WAIT_UNTIL(etimer_expired(&et));
		SET_PIN_ADC7;   // LED1
		etimer_set(&et, CLOCK_SECOND / 4);
		PROCESS_WAIT_UNTIL(etimer_expired(&et));
		UNSET_PIN_ADC7; // LED1
		UNSET_PIN_ADC6; // LED2
		SET_PIN_ADC2;   // LED3
		etimer_set(&et, CLOCK_SECOND / 4);
		PROCESS_WAIT_UNTIL(etimer_expired(&et));
		SET_PIN_ADC7;   // LED1
		etimer_set(&et, CLOCK_SECOND / 4);
		PROCESS_WAIT_UNTIL(etimer_expired(&et));
		UNSET_PIN_ADC7; // LED1
		SET_PIN_ADC6;   // LED2
		etimer_set(&et, CLOCK_SECOND / 4);
		PROCESS_WAIT_UNTIL(etimer_expired(&et));
		SET_PIN_ADC7;   // LED1
		etimer_set(&et, CLOCK_SECOND / 4);
		PROCESS_WAIT_UNTIL(etimer_expired(&et));
		UNSET_PIN_ADC7; // LED1
		UNSET_PIN_ADC6; // LED2
		UNSET_PIN_ADC2; // LED3
	}

	PROCESS_END();
}
