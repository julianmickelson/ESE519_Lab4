#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <util/delay.h>
#include "lcd.h"

#define FREQ 16000000
#define BAUD 9600
#define HIGH 1
#define LOW 0
#define BUFFER 1024
#define BLACK 0x000001

char displayChar = 0;

int main(void)
{
	//setting up the gpio for backlight
	DDRD |= 0x1C;
	PORTD &= ~0x1C;
	PORTD |= 0x00;
	
	//lcd initialisation
	lcd_init();
	lcd_command(CMD_DISPLAY_ON);
	lcd_set_brightness(0x18);
	write_buffer(buff);
	_delay_ms(50000);
	clear_buffer(buff);
	
	fillrect(buff,0,0,20,20,displayChar);
	write_buffer(buff);
	_delay_ms(30000);
	drawrect(buff,30,30,20,20,displayChar);
	write_buffer(buff);
	_delay_ms(30000);

	while (1) {
	}
}

