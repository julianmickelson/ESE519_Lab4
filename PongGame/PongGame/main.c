#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <util/delay.h>
#include <math.h>
#include "uart.h"
#include "lcd.h"

#define F_CPU 16000000UL
#define FREQ 16000000
#define BAUD 9600
#define HIGH 1
#define LOW 0
#define WIDTH 128
#define HEIGHT 64
#define BUFFER (WIDTH*HEIGHT)
#define BLACK 0x000001
#define PADDLE_LENGTH 10
#define BALL_RADIUS 2
#define X_MINUS 0
#define X_PLUS 2
#define Y_MINUS 1
#define Y_PLUS 3

// random array of initial velocities
uint8_t randArr[6] = {-3, -2, -1, 1, 2, 3};
uint8_t seed = 22;

// initialize the touch screen coordinates
uint8_t Xt;
uint8_t Yt;

// initialize the display coordinates
uint8_t Xd = 12;
uint8_t Yd = 5;

// initialize score variables
char scoreL = '0';
char scoreR = '0';

// initialize the L and R paddle coordinates
uint8_t paddleL = 4 + HEIGHT/2 - PADDLE_LENGTH/2;
uint8_t paddleR = HEIGHT/2 - PADDLE_LENGTH/2;

// initialize the ball positions
int ballX = WIDTH/2;
int ballY = HEIGHT/2;

// initialize the ball velocities
int ballVX;
int ballVY;

// initialize flag for start of new round
uint8_t newRound = 1;

// function to play sounds
void beepTone();

// function to check touchscreen and update paddle 
void checkInput();

// function to update game parameters 
void update();

// function to draw graphics 
void draw();

// function to check horizontal and vertical collisions of the ball
int checkCollisions();

// function to check if someone has scored
int checkScore();

// function to read touchscreen X coordinate
int readX();

// function to read touchscreen Y coordinate
int readY();

int main(void)
{
	// setting up the gpio for backlight
	DDRD |= 0x80;
	PORTD &= ~0x80;
	PORTD |= 0x00;
	
	DDRB |= 0x05;
	PORTB &= ~0x05;
	PORTB |= 0x00;
	
	// lcd initialization
	lcd_init();
	lcd_command(CMD_DISPLAY_ON);
	lcd_set_brightness(0x18);
	write_buffer(buff);
	_delay_ms(50000);
	clear_buffer(buff);
	
	// ADC initialization
	uart_init();
	//sei();
	//ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // prescale /128
	//ADCSRA |= (1 << ADIE); // enable interrupt

	while (1) {
		
		// check touchscreen and update paddle
		checkInput();
		
		// run gameplay cycle
		update();
		
		// update graphics
		draw();

	}
	
}

// draw graphics
void draw() {
	
	clear_buffer(buff);
	
	// bounding rectangle
	drawrect(buff, 1, 1, WIDTH, HEIGHT, BLACK);
	
	// half court dashed line (with space for ball start point)
	for (int i = 0; i < (WIDTH - 2)/2; i++) {
		drawline(buff, WIDTH/2, 2 + i*4, WIDTH/2, 3 + i*4, BLACK);
	}

	for (int i = 0; i < 8; i++) {
		clearpixel(buff, WIDTH/2, HEIGHT/2 - 4 + i);
	}
	
	// scoreboard
	drawchar(buff, WIDTH/2 - 10, 0, scoreL);
	drawchar(buff, WIDTH/2 + 6, 0, scoreR);
	
	// paddles
	fillrect(buff, 3, paddleL, 2, PADDLE_LENGTH , BLACK);
	fillrect(buff, WIDTH - 3, paddleR, 2, PADDLE_LENGTH, BLACK);
	
	// ball
	fillcircle(buff, ballX, ballY, BALL_RADIUS, BLACK);

	// update screen
	write_buffer(buff);
}

// update gameplay cycle
void update() {
	
	// check if start of a round
	if (newRound) {	
		// reset ball position
		ballX = WIDTH/2;
		ballY = HEIGHT/2;
		
		srand(seed);
		
		// start ball in random direction and speed
		while(!ballVX) {
			ballVX = -2 + (int)((double)rand() / ((double)RAND_MAX + 1) * 5);
		}
			ballVY = -2 + (int)((double)rand() / ((double)RAND_MAX + 1) * 5);
		
		newRound = 0;
	}
	
		
	// update ball position
	ballX += ballVX;
	ballY += ballVY;
	
	// check for paddle collision
	if (checkCollisions()) {
			
			// find where on paddle ball hit
			int hitPos;
			
			// left paddle collision
			if (ballVX < 0) {
				hitPos = (paddleL + (PADDLE_LENGTH/2)) - ballY;
			} 
			
			// right paddle collision
			else {
				hitPos = (paddleR + (PADDLE_LENGTH/2)) - ballY;
			}
			
			// change ball y angle based on where on paddle it hit
			ballVY -= hitPos/2;
			
			// reverse ball x direction
			ballVX = -ballVX;
			
			// make noise
			beepTone();
	}
	
	// check for ceiling collision
	if ((ballY - BALL_RADIUS < 3) || (ballY + BALL_RADIUS > HEIGHT - 1)) {
		// reverse ball y direction
		ballVY = -ballVY;
		
		// make noise
		beepTone();
	}
	
	// check if someone scored
	if (checkScore()) {
		// make noise
		beepTone();
		
		// change screen color
		
		// reset new round flag
		newRound = 1;
	}
	
	// delay graphics refresh
	_delay_ms(100);
}

// returns 1 if there is a collision with a paddle, 0 if not
int checkCollisions() {
	
	// ball is right of left paddle
	if (ballX - BALL_RADIUS > 5) {

		// ball is left of right paddle
		if (ballX + BALL_RADIUS < WIDTH - 4) {
			return 0;
		} 
		
		// ball is over top of right paddle or under bottom right paddle
		else if ((ballY - BALL_RADIUS > paddleR + PADDLE_LENGTH) || (ballY + BALL_RADIUS < paddleR)) {
			return 0;
		}
		
		// collision with right paddle
		else {
			return 1;
		}
		
	} else {
		
		// ball is over top of left paddle or under bottom left paddle
		if ((ballY - BALL_RADIUS > paddleL + PADDLE_LENGTH) || (ballY + BALL_RADIUS < paddleL)) {
			return 0;
		} 
		
		// collision with left paddle
		else {
			return 1;
		}
	}
}

int checkScore() {
	// right player score on left wall
	if (ballX - BALL_RADIUS < 2) {
		scoreR ++;
		return 1;
	}
	
	// left player score on right wall
	else if (ballX + BALL_RADIUS > WIDTH - 1) {
		scoreL ++;
		return 1;
	}
	
	// no score
	else {
		return 0;
	}
}

// initialize function to play sounds
void beepTone() {
	
}

void checkInput() {
	
	//// using port c, 0 - 3
	//// C0 -> X-
	//// C1 -> Y+
	//// C2 -> X+
	//// C3 -> Y-
	//
	//// step 1: set Xs digital - set X- high and X+ low
	//
	//DDRC |= (1 << 0) | (1 << 2); // Xs output
	//PORTC |= (1 << 0); // X-/C0 high
	//PORTC &= ~(1 << 2); // X+/C2 low
	//
	//// step 2: set Y- and Y+ to analog input and read Y-
	//
	//DDRC &= ~(1 << 1);
	//PORTC &= ~(1 << 1); // Y+/C1 to input and disable pull-up
	//
	//DIDR0 |= (1 << ADC3D); // disable digital input
	//
	//ADMUX |= (1 << MUX0) | (1 << MUX1); // select ADC3 / Y-
	//ADCSRA |= (1 << ADEN); // enable system
	//
	//ADCSRA |= (1 << ADSC); // start
	//
	//while(bit_is_clear(ADCSRA,ADIF)); // stall until conversion is finished
	//
	//int ycoord = ADC; // store ADC value as y coordinate
	//ADCSRA &= ~(1 << ADEN); // disable system
	//
	//printf("%s", "ycoord = ");
	//printf("%d",ycoord);
	//printf("\n");
	//
	//// step 3: Ys to digital and Y+ low and Y- high
	//
	//DDRC |= (1 << 1); // Y+/C1 to output
	//PORTC &= ~(1 << 1); // Y+/C1 to low
	//
	//DDRC |= (1 << 3); // Y-/C3 to output
	//PORTC |= (1 << 3); // Y-/C3 to high
	//
	//// step 4:
	//
	//DDRC &= ~(1 << 2);
	//PORTC &= ~(1 << 2); // X+/C2 to input and disable pull-up
	//
	//DIDR0 |= (1 << ADC0D); // disable digital input
	//
	//ADMUX &= ~(1 << MUX0);
	//ADMUX &= ~(1 << MUX1); // X-/C0 ADC0
	//ADCSRA |= (1 << ADEN); // enable system
	//DIDR0 |= (1 << ADC0D); // disable digital input
	//
	//ADCSRA |= (1 << ADSC); // start.
	//
	//while(bit_is_clear(ADCSRA,ADIF)); // stall
	//
	//int xcoord = ADC; // store ADC val
	//ADCSRA &= ~(1 << ADEN); // disable system
	//
	//printf("%s", "xcoord = ");
	//printf("%d",xcoord);
	//printf("\n");
	
	// left player
	if (Xd < WIDTH/4) {
		// tap above paddle
		if (Yd < paddleL) {
			paddleL -= 3;
		}
		
		// tap below paddle
		else if (Yd > paddleL + PADDLE_LENGTH) {
			paddleL += 3;
		}
	}
	
	// right player
	else if (Xd > 0.75*WIDTH) {
		// tap above paddle
		if (Yd < paddleR) {
			paddleR -= 3;
		}
		
		// tap below paddle
		else if (Yd > paddleR + PADDLE_LENGTH) {
			paddleR += 3;
		}
	}
}