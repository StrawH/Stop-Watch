/*
 * main.c
 *
 *  Created on: Jan 8, 2022
 *      Author: Omar
 */
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// declare 6 variables for 6 digits stop watch
unsigned char time[6] = { 0, 0, 0, 0, 0, 0 };

#define seconds_0 time[0]		// first digit in seconds
#define seconds_1 time[1]		// second digit in seconds
#define minutes_0 time[2]		// first digit in minutes
#define minutes_1 time[3]		// second digit in minutes
#define hours_0 time[4]			// first digit in hours
#define hours_1 time[5]			// second digit in hours

// configure timer1 , make an interrupt every compare match at 1s .
void timer1_init();

// configure interrupt0 to reset stop watch time once external interrupt occurred
void INT0_Init();

// configure interrupt1 to pause stop watch time once external interrupt occurred
void INT1_Init();

// configure interrupt2 to reset stop watch time once external interrupt occurred
void INT2_Init();

// confger 7-segment to display numbers
void seven_segment_display(int num);

int main(void) {
	DDRB |= (1 << 0);

	INT1_Init();           // Enable and configure external INT1
	INT0_Init();           // Enable external INT0
	INT2_Init();           // Enable and configure external INT2

	timer1_init();		// enable timer1 Interrupt

	/* configure first four pins of PORTC as output pins for 7-segment decoder A B C D */
	DDRC |= 0x0F;

	/* configure first six pins of PORTA as output pins for 7-segment enable 1 2 3 4 5 6 */
	DDRA = 0x3F;

	/* configure Interrupt pins as output */
	DDRD &= ~(1 << PD2);  // Configure pin PD0 in PORTD as interrupt0 input pin
	DDRD &= ~(1 << PD3); //configure pin PD3 as interrupt1 input
	DDRB &= ~(1 << PB2); //configure pin PB3 as interrupt2 input

	/* initialize the 7-segment with value 0 by clear the first four bits in PORTC */
	PORTC &= 0xF0;

	/* initialize the 7-segment enable pins with 1 in PORTA */
	PORTA = 0x3F;

	/* activate Interrupt pins pull up resistors to make interrupt at falling edges */
	PORTD |= (1 << PD2); // activate internal pull up for INT0
	PORTB |= (1 << PB2); // activate internal pull up for INT2

	/* initialize variables to switch between 7-segment digits */
	unsigned char segment = 5; // segment digit number 0 to 5 , initialized with 5 to start from the right segment digit
	unsigned char counter = 0; // stop watch digits seconds, minutes and hours 0 to 5

	/* start of Super Loop */
	for (;;) {

		/* ***************** display time on 7-segment ****************** */

		/* when reach last segment back for the first one */
		if (counter > 5) {
			counter = 0;		// timer array
			segment = 5;		// digit number
		}

		/* swap between segment and display target number in each segment super fast */
		PORTA = ~(1 << segment);		// swap between segment by putting 0 and enable pin
		seven_segment_display(time[counter]);	// display target number
		_delay_ms(5);					 // small delay to be able to display and switch between segments

		/* change targeted segment and time digit */
		segment--;		// targeted  segment
		counter++;		// time digit number

	}/* end of super loop*/

	return 0;
}

void seven_segment_display(int num) {

	/* display number from from 0 to 9 on PORTC
	 * mask number with port to get only required bits */
	PORTC = (PORTC & 0xF0) | (num & 0x0F);
}

void timer1_init() {

	/* timer required operating conditions :
	 * Frequency = 1MH
	 * prescaler = 1024
	 * every compare match with 1000 the timer counts 1S and makes
	 * an interrupt */

	// set up timer with prescaler = 1024 and CTC mode
	TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);

	// initialize counter
	TCNT1 = 0;

	// initialize compare value
	OCR1A = 1000;

	// enable compare interrupt
	TIMSK |= (1 << OCIE1A);

	// enable global interrupts
	SREG |= (1 << 7);           // Enable global interrupts in MC.
}

ISR( TIMER1_COMPA_vect) {
	// toggle led here for  interrupt check occurrence every 1S
	PORTB ^= (1 << PB0);

	/* increase stop watch digit by digit when every digit reaches its maximum value */
	// first seconds digit from 0 to 9 , increase seaconds every interrupt (every 1S)
	seconds_0++;
	if (seconds_0> 9) {

		// increase second seconds digit by one
		seconds_1++;

		// reset seconds first digit
		seconds_0 = 0;
	}

	if (seconds_1> 5) {

		// increase minutes first digit by one
		minutes_0++;

		// reset seconds second digit
		seconds_1 = 0;
	}
	if (minutes_0> 5) {

		// reset minutes first digit
		minutes_0 = 0;

		// increase second minutes digit by one
		minutes_1++;
	}
	if (minutes_1> 5) {

		// reset minutes second digit
		minutes_1 = 0;

		// increase first hours digit by one
		hours_0++;
	}
	if (hours_0> 5) {

		// reset hours first digit
		hours_0 = 0;

		// increase second hours digit by one
		hours_1++;
	}
	if (hours_1> 9) {

		// reset hours digit maximum value 9
		hours_1 = 0;
	}
}

/* External INT0 enable and configuration function */
void INT0_Init(void) {

	SREG &= ~(1 << 7);                   // Disable interrupts by clearing I-bit
	DDRD &= (~(1 << PD2));               // Configure INT0/PD2 as input pin
	GICR |= (1 << INT0);                 // Enable external interrupt pin INT0
	MCUCR |= (1 << ISC01);   			// Trigger INT0 with the falling edge
	SREG |= (1 << 7);                    // Enable interrupts by setting I-bit
}

/* External INT0 Interrupt Service Routine */
ISR( INT0_vect) {

	/*  reset stop watch time once external interrupt occurred */
	unsigned char i = 0;
	for (i = 0; i <= 5; i++) {
		time[i] = 0;
	}
}

/* External INT1 enable and configuration function */
void INT1_Init(void) {

	SREG &= ~(1 << 7);      // Disable interrupts by clearing I-bit
	GICR |= (1 << INT1);    // Enable external interrupt pin INT1
	MCUCR = (1 << ISC11) | (1 << ISC10);	// Trigger INT1 with the rising edge
	SREG |= (1 << 7);       // Enable interrupts by setting I-bit
}

ISR(INT1_vect) {

	/* pause stop watch counting by disabling timer 1 clock */
	TCCR1B &= ~(1 << CS10);
	TCCR1B &= ~(1 << CS11);
	TCCR1B &= ~(1 << CS12);
}

/* External INT2 enable and configuration function */
void INT2_Init(void) {

	SREG &= ~(1 << 7);      // Disable interrupts by clearing I-bit
	GICR |= (1 << INT2);    // Enable external interrupt pin INT1
//	MCUCSR &= ~(1 << ISC11);	// Trigger INT2 with the falling edge
	SREG |= (1 << 7);       // Enable interrupts by setting I-bit
}

ISR(INT2_vect) {

	/* resume stop watch time by reactivate timer 1 clock with prescaler 1024 */
	TCCR1B |= (1 << CS12) | (1 << CS10);
}
