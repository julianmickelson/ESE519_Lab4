/*
 * ESE 519 Lab 4
 * Julian Mickelson
 * Myles Cai
 */ 


#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <util/delay.h>
#include <math.h>
#include <time.h>
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

// 0 is one-player (accel mode), 1 is two-player
#define GAME_MODE 0

// initialize the touch screen coordinates
uint8_t Xt;
uint8_t Yt;

// initialize the display coordinates
uint8_t Xd;
uint8_t Yd;

// initialize score variables
char scoreL = '0';
char scoreR = '0';

// initialize the L and R paddle coordinates
uint8_t paddleL = HEIGHT/2 - PADDLE_LENGTH/2;
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

// function to process accelerometer
void pongBot();

// function to check if someone has scored
int checkScore();

// function to read touchscreen X coordinate
int readX();

// function to read touchscreen Y coordinate
int readY();

// function to process accelerometer
int accfilt();

const int test = 5;

int main(void)
{
	// init rand/time
	srand(time(NULL));
	
	// init print
	uart_init();

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
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // prescale /128
	ADMUX |= (1 << REFS0); // AREF = AVcc
	_delay_ms(50);
	
	// accel initialization
	DIDR0 |= (1 << ADC5D); // disable digital input

	while (1) {
		
		// touchscreen mode
		if (GAME_MODE) {
			// check touchscreen and update paddle
			checkInput();
		} 
		
		// accelerometer + pong bot mode
		else {
			// computer update paddle
			pongBot();

			// update user paddle based on 
			paddleL += accfilt();
			
			// make sure paddle is in bounds
			// paddle is too low
			if (paddleL + PADDLE_LENGTH > HEIGHT - 1) {
				paddleL = HEIGHT - PADDLE_LENGTH;
			}
					
			// paddle is too high
			if (paddleL < 2) {
				paddleL = 2;
			}
		}
		
		// run game play cycle
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
		
		// start ball in random direction and speed
		double randVal1 = (float)rand()/RAND_MAX;
		if (randVal1 < 0.25) {
			ballVX = -2;
		}
		else if (randVal1 < 0.5) {
			ballVX = -1;
		}
		else if (randVal1 < 0.75) {
			ballVX = 1;
		} else {
			ballVX = 2;
		}
		
		double randVal2 = (float)rand()/RAND_MAX;
		if (randVal2 < 0.2) {
			ballVY = -2;
		}
		else if (randVal2 < 0.4) {
			ballVX = -1;
		}
		else if (randVal2 < 0.6) {
			ballVY = 0;
		}
		else if (randVal2 < 0.8) {
			ballVY = 1;
		} else {
			ballVY = 2;
		}
		
		// reset flag
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
	
	// check for ceiling  or floor collision
	if ((ballY - BALL_RADIUS < 3) || (ballY + BALL_RADIUS > HEIGHT - 1)) {
		// reverse ball y direction
		ballVY = -ballVY;
		
		// make noise
		beepTone();
	}
	
	// make sure ball isn't going too fast
	if (ballVX < -2) {
		ballVX = -2;
	}
	
	if (ballVX > 2) {
		ballVX = 2;
	}
	
	if (ballVY < -2) {
		ballVY = -2;
	}
	
	if (ballVY > 2) {
		ballVY = 2;
	}
	
	
	
	// check if someone scored
	if (checkScore()) {
		// make noise
		beepTone();
		
		// reset new round flag
		newRound = 1;
	}
	
	// delay graphics refresh
	_delay_ms(100);
}

// returns 1 if there is a collision with a paddle, 0 if not
int checkCollisions() {
	
	// ball is right of left paddle
	if (ballX - BALL_RADIUS > 4) {

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
	if (ballX - BALL_RADIUS < 3) {
		scoreR ++;

		if (scoreR == '9'){
			
			// set screen red
			PORTD &= ~(1 << 7);
			PORTB |= (1 << 0);
			PORTB |= (1 << 2);
			
			draw();
			
			_delay_ms(10000);
			
			// reset score
			scoreR = '0';
			scoreL = '0';
			
			// return screen to white
			PORTD &= ~(1 << 7);
			PORTB &= ~(1 << 0);
			PORTB &= ~(1 << 2);
		}
		
		return 1;
	}
	
	// left player score on right wall
	else if (ballX + BALL_RADIUS > WIDTH - 2) {
		scoreL ++;

		if (scoreL == '9'){
			
			// set screen purple
			PORTD |= (1 << 7);
			PORTB |= (1 << 0); 
			PORTB &= ~(1 << 2); 
			
			draw();
			
			_delay_ms(10000);
			
			// reset score
			scoreR = '0';
			scoreL = '0';
			
			// return screen to white
			PORTD &= ~(1 << 7);
			PORTB &= ~(1 << 0);
			PORTB &= ~(1 << 2);
			
		}
		return 1;
	}
	
	// no score
	else {
		return 0;
	}

}

// initialize function to play sounds
void beepTone() {
	// using B3 output (not pwm?)
	DDRB |= (1 << 3);
	
	for (int i = 0; i < 10; i++){
		PORTB |= (1 << 3);
		_delay_ms(5);
		PORTB &= ~(1 << 3);
		_delay_ms(5);
	}	
}

void checkInput() {
	
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

	DDRC &= ~(1 << 3);
	PORTC &= ~(1 << 3); // Y-/C3 to input and disable pull-up

	DIDR0 |= (1 << ADC3D); // disable digital input

	ADMUX |= (1 << MUX0) | (1 << MUX1); // select ADC3 / Y-
	ADCSRA |= (1 << ADEN); // enable system

	ADCSRA |= (1 << ADSC); // start

	while(bit_is_clear(ADCSRA,ADIF)); // stall until conversion is finished

	int xcoord = ADC; // store ADC value as y coordinate
	ADCSRA &= ~(1 << ADEN); // disable system

	// step 3: Ys to digital and Y+ low and Y- high

	DDRC |= (1 << 1); // Y+/C1 to output
	PORTC &= ~(1 << 1); // Y+/C1 to low

	DDRC |= (1 << 3); // Y-/C3 to output
	PORTC |= (1 << 3); // Y-/C3 to high

	// step 4:

	DDRC &= ~(1 << 2);
	PORTC &= ~(1 << 2); // X+/C2 to input and disable pull-up

	DDRC &= ~(1 << 0);
	PORTC &= ~(1 << 0); // X-/C0 to input and disable pull-up

	DIDR0 |= (1 << ADC0D); // disable digital input

	ADMUX &= ~(1 << MUX0);
	ADMUX &= ~(1 << MUX1); // X-/C0 ADC0
	ADCSRA |= (1 << ADEN); // enable system
	DIDR0 |= (1 << ADC0D); // disable digital input

	ADCSRA |= (1 << ADSC); // start.

	while(bit_is_clear(ADCSRA,ADIF)); // stall

	int ycoord = ADC; // store ADC val
	ADCSRA &= ~(1 << ADEN); // disable system
	
	// shifting from xcoord & ycoord to Xd and Yd
	// for x: mappin from 150-850 to 1 - 128
	// for y: mappin from 90 - 900 to 1 - 64
	
	printf("xd = %d     yd = %d\n", xcoord, ycoord);

	if (ycoord < 730) { // check that it's even being touched

		Xd = (930 - (xcoord - 70))/(float)930 * WIDTH;
		Yd = (ycoord - 110)/(float)730 * HEIGHT;

		// left player
		if (Xd < WIDTH/4) {
			// tap top half of screen
			if (Yd < HEIGHT/2) {
				paddleL -= 2;
			}
		
			// tap bottom half of screen
			else {
				paddleL += 2;
			}
		}
	
		// right player
		else if (Xd > 0.75*WIDTH) {
			// tap top half of screen
			if (Yd < HEIGHT/2) {
				paddleR -= 2;
			}
			
			// tap bottom half of screen
			else {
				paddleR += 2;
			}
		}
	
		// make sure paddle is in bounds
		// paddle is too low
		if (paddleL + PADDLE_LENGTH > HEIGHT - 1) {
			paddleL = HEIGHT - PADDLE_LENGTH;
		}
				
		// paddle is too high
		if (paddleL < 2) {
			paddleL = 2;
		}
		
		// paddle is too low
		if (paddleR + PADDLE_LENGTH > HEIGHT - 1) {
			paddleR = HEIGHT - PADDLE_LENGTH;
		}
				
		// paddle is too high
		if (paddleR < 2) {
			paddleR = 2;
		}
	}
}

void pongBot(void) {
	
	int botSpeed = 1;
	
	if (ballX > WIDTH/2) {
		// paddle is higher than ball
		if (paddleR + (PADDLE_LENGTH/2) < ballY) {
			// move paddle down
			paddleR += botSpeed;
		}
	
		// paddle is lower than ball
		else if (paddleR + (PADDLE_LENGTH/2) > ballY) {
			// move paddle up
			paddleR -= botSpeed;
		}
	
		// make sure paddle is in bounds
		// paddle is too low
		if (paddleR + PADDLE_LENGTH > HEIGHT - 1) {
			paddleR = HEIGHT - PADDLE_LENGTH;
		}
	
		// paddle is too high
		if (paddleR < 2) {
			paddleR = 2;
		}
	}
}

int accfilt(void){
	
	ADCSRA |= (1 << ADEN); // enable system
	ADMUX &= ~(1 << MUX1);
	ADMUX |= (1 << MUX0) | (1 << MUX2); // select ADC5

	int arr[10];
	int sum = 0;
	
	for(int j = 0; j < 10; j++){

		ADCSRA |= (1 << ADSC); // start

		while(bit_is_clear(ADCSRA,ADIF)); // stall

		arr[j] = ADC;

		sum = sum + ADC;

	}

	int avera = sum/10;
	
	if (avera < 305){
		return -1;
	}
	else if (avera > 349){
		return 1;
	}
	else {
		return 0;
	}
}
