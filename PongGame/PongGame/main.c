#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <util/delay.h>
#include "uart.h"
#include "lcd.h"

#define F_CPU 16000000UL
#define FREQ 16000000
#define BAUD 9600
#define HIGH 1
#define LOW 0
#define BUFFER 1024
#define BLACK 0x000001
#define X_MINUS 0
#define X_PLUS 2
#define Y_MINUS 1
#define Y_PLUS 3

// initialize the touch screen coordinates
uint8_t Xt;
uint8_t Yt;

// initialize the display coordinates
uint8_t Xd;
uint8_t Yd;

volatile int flag;

char displayChar = 0;

int main(void)
{
	// setting up the gpio for backlight
	DDRD |= 0x1C;
	PORTD &= ~0x1C;
	PORTD |= 0x00;
	
	// lcd initialization
	lcd_init();
	lcd_command(CMD_DISPLAY_ON);
	lcd_set_brightness(0x18);
	write_buffer(buff);
	_delay_ms(10000);
	clear_buffer(buff);
	
	// ADC initialization
	uart_init();
	sei();
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // prescale /128
	ADCSRA |= (1 << ADIE); // enable interrupt
	
	uint8_t msg[] = "hello Myles";
	uint8_t* pmsg = msg; 
	
	drawcircle(buff,30,30,20,0);
	write_buffer(buff);

	while (1) {
		// using port c, 0 - 3
		// C0 -> X-
		// C1 -> Y+
		// C2 -> X+
		// C3 -> Y-
		
		// step 1: set Xs digital - set X- high and X+ low
		
		DDRC |= (1 << 0) | (1 << 2); // Xs output
		PORTC |= (1 << 0); // X-/C0 high
		PORTC &= ~(1 << 2); // X+/C2 low
		
		// step 2: set Y- and Y+ to analog input and read Y-
		
		DDRC &= ~(1 << 1);
		PORTC &= ~(1 << 1); // Y+/C1 to input and disable pull-up
		
		DIDR0 |= (1 << ADC3D); // disable digital input
		
		ADMUX |= (1 << MUX0) | (1 << MUX1); // select ADC3 / Y-
		ADCSRA |= (1 << ADEN); // enable system
		
		ADCSRA |= (1 << ADSC); // start
		
		while(bit_is_clear(ADCSRA,ADIF)); // stall until conversion is finished
		
		int ycoord = ADC; // store ADC value as y coordinate
		ADCSRA &= ~(1 << ADEN); // disable system
		
		printf("%s", "ycoord = ");
		printf("%d",ycoord);
		printf("\n");
		
		// step 3: Ys to digital and Y+ low and Y- high
		
		DDRC |= (1 << 1); // Y+/C1 to output
		PORTC &= ~(1 << 1); // Y+/C1 to low
		
		DDRC |= (1 << 3); // Y-/C3 to output
		PORTC |= (1 << 3); // Y-/C3 to high
		
		// step 4:
		
		DDRC &= ~(1 << 2);
		PORTC &= ~(1 << 2); // X+/C2 to input and disable pull-up
		
		DIDR0 |= (1 << ADC0D); // disable digital input
		
		ADMUX &= ~(1 << MUX0);
		ADMUX &= ~(1 << MUX1); // X-/C0 ADC0
		ADCSRA |= (1 << ADEN); // enable system
		DIDR0 |= (1 << ADC0D); // disable digital input
		
		ADCSRA |= (1 << ADSC); // start.
		
		while(bit_is_clear(ADCSRA,ADIF)); // stall
		
		int xcoord = ADC; // store ADC val
		ADCSRA &= ~(1 << ADEN); // disable system
		
		printf("%s", "xcoord = ");
		printf("%d",xcoord);
		printf("\n");

	}
	
}

ISR(ADC_vect) {

}

