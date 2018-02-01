#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "common.h"
#include "uart.h"

#define TIMER_OFF {\
TIMSK1 = 0;\
TCNT1 = 0;\
TCCR1B = 0;\
}

#define TIMER_ON {\
TIMSK1 |= (1<<OCIE1A);\
TIFR1 |= (1<<OCF1A);\
TCNT1 = 0;\
TCCR1B = (1<<CS10);\
}

static bool rx_active;
bool uart_data_rdy;

ISR(TIMER1_COMPA_vect)
{
	TCNT1 = 0;
	TIMER_OFF;

	rx_active = false;

	ACTOFF;
}

ISR(USART0_RX_vect)
{
	static unsigned char input_counter = 0;
	unsigned char in = UDR;

	ACTON;
	TIMER_ON;

	if (!rx_active) {
		rx_active = true;
		uart_data_rdy = false;
		input_counter = 0;
	}

	uart_pwm_buffer[input_counter++] = in;

	if (input_counter == PWMCHANNELS) {
		ACTOFF;
		TIMER_OFF;
		rx_active = false;
		uart_data_rdy = true;
	}
}

void uart_init(void)
{
	UBRRL = 8;
	UBRRH = 0;
	// Enable receiver + enable receive interrupt
	UCSRB = (1<<RXEN)|(1<<RXCIE);
	// 8n1
	UCSRC = (1<<UCSZ1)|(1<<UCSZ0);

	// Configure Timer
	TCCR1A = 0;
	TCCR1B = 0; // Timer deactivated

	TCNT1 = 0;
	//OCR1A = 1600; // ~100µS
	OCR1A = 16000; // ~1mS

	// Disable interrupt
	TIMER_OFF;

	sei();
}
