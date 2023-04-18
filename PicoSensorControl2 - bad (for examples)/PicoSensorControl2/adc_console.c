#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

// Constants for GPIO pins
const uint LED_PIN = PICO_DEFAULT_LED_PIN;   // On-board LED pin
const uint PING_PIN = 30;                     // GPIO pin for PING sensor
const uint IR_PIN = 29;                       // GPIO pin for IR sensor

// Constants for ADC settings
const uint ADC_RESOLUTION_BITS = 12;         // ADC resolution in bits
const float ADC_REFERENCE_VOLTAGE = 3.3f;     // ADC reference voltage in volts

int main() {
	// Initialize standard I/O
	stdio_init_all();

	// Initialize ADC
	adc_init();
	adc_gpio_init(IR_PIN);
	adc_select_input(0); // Select ADC input 0 (GPIO26)

	// Set LED pin as output
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	while (1) {
		// Read PING sensor
		gpio_put(PING_PIN, 1);
		sleep_us(10); // Pulse trigger for 10 us
		gpio_put(PING_PIN, 0);
		while (!gpio_get(PING_PIN)) {} // Wait for echo to start
		uint32_t start = time_us_32();
		while (gpio_get(PING_PIN)) {} // Wait for echo to end
		uint32_t end = time_us_32();
		uint32_t pulse_duration = end - start;
		float distance = pulse_duration * 0.0343f / 2; // Calculate distance in cm
		printf("PING Distance: %.2f cm\n", distance);

		// Read IR sensor
		uint16_t adc_value = adc_read();
		float voltage = (float)adc_value * ADC_REFERENCE_VOLTAGE / (1 << ADC_RESOLUTION_BITS);
		printf("IR Voltage: %.2f V\n", voltage);

		// Toggle LED
		gpio_xor_mask(1u << LED_PIN);

		sleep_ms(1000); // Delay for 1 second
	}

	return 0;
}
