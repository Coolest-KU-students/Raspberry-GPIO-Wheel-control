#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

// Constants for GPIO pins
const uint LED_PIN = PICO_DEFAULT_LED_PIN;    // On-board LED pin
const uint PING_PIN_1 = 22;                   // GPIO pin for PING sensor
const uint PING_PIN_2 = 21;
const uint IR_PIN_1 = 0;                      // GPIO26 pin for IR sensor
const uint IR_PIN_2 = 1;					  // GPIO27
const uint IR_PIN_3 = 2;					  // GPIO28
// Constants for ADC settings
const uint ADC_RESOLUTION_BITS = 12;          // ADC resolution in bits
const float ADC_REFERENCE_VOLTAGE = 3.3f;     // ADC reference voltage in volts



int main() {	
	uint32_t IRValues[3] = {0 , 0 , 0};
	float USValues[2] = {0.0, 0.0};
	// Initialize standard I/O
	stdio_init_all();

	// Initialize ADC
	adc_init();
	adc_gpio_init(IR_PIN_1);
	adc_gpio_init(IR_PIN_2);
	adc_gpio_init(IR_PIN_3);

	
	gpio_init(PING_PIN_1);
	gpio_init(PING_PIN_2);

	gpio_set_dir(LED_PIN, GPIO_OUT);
	while(true){
		for(int i = 0; i < 3; i++){
			adc_select_input(i); // Select ADC input 0 (GPIO26)
			uint16_t adc_value = adc_read();
			IRValues[i] = adc_value;
		}

		for(int i = 0; i< 2; i++){
			uint CURRENT_PIN = i == 0 ? PING_PIN_1 : PING_PIN_2;

			gpio_set_dir(CURRENT_PIN, GPIO_OUT);
			gpio_put(CURRENT_PIN, 1);
			sleep_us(10); // Pulse trigger for 10 us
			gpio_put(CURRENT_PIN, 0);
			gpio_set_dir(CURRENT_PIN, GPIO_IN);
			uint32_t waitingTime = time_us_32();

			while (!gpio_get(CURRENT_PIN) 
					&& time_us_32() - waitingTime < 100000) //if it doesn't respond in 100 milisecond, skip
				{} // Wait for echo to start
		
			uint32_t start = time_us_32();
			if(start - waitingTime > 100000){
				USValues[i] = 0;
				continue; //next loop
			}
			while (gpio_get(CURRENT_PIN)) {} // Wait for echo to end
			uint32_t end = time_us_32();
			uint32_t pulse_duration = end - start;
			float distance = pulse_duration * 0.0343f / 2; // Calculate distance in cm
			USValues[i] = distance;
		}
	}
	
	printf("\n");
	printf("%i",IRValues[0]);
	printf(";");
	printf("%.2f",USValues[0]);
	printf(";");
	printf("%i",IRValues[1]);
	printf(";");
	printf("%.2f",USValues[1]);
	printf(";");
	printf("%i",IRValues[2]);
	printf(";");

	/*while (true) {
		//printf("Sending pulse\n");
		// Read PING sensor
		gpio_put(PING_PIN, 1);
		sleep_us(10); // Pulse trigger for 10 us
		gpio_put(PING_PIN, 0);
		gpio_set_dir(PING_PIN, GPIO_IN);
		while (!gpio_get(PING_PIN)) {printf("Awaiting pulse");} // Wait for echo to start
		uint32_t start = time_us_32();
		while (gpio_get(PING_PIN)) {} // Wait for echo to end
		uint32_t end = time_us_32();
		uint32_t pulse_duration = end - start;
		gpio_set_dir(PING_PIN, GPIO_OUT);
		float distance = pulse_duration * 0.0343f / 2; // Calculate distance in cm
		printf("PING Distance: %.2f cm\n", distance);
		
		// Read IR sensor
		uint16_t adc_value = adc_read();
	//	float voltage = (float)adc_value * ADC_REFERENCE_VOLTAGE / (1 << ADC_RESOLUTION_BITS);
	//	float distance = 13.0 * pow(voltage, -1.10); //chatgpt formula
	//	printf("IR Sensor returned %i \n", adc_value);
	//	printf("IR Voltage: %.2f V\n", voltage);
	//	printf("IR distance: %.2f V\n", distance);
		// Toggle LED
		gpio_xor_mask(1u << LED_PIN);

		sleep_ms(5000); // Delay for 1 second
	}*/


	return 0;
}
