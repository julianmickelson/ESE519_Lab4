ESE 519 Lab 4

Julian Mickelson
Myles Cai

lcd.c **********************************

API to draw to lcd
- set pixel
- clear pixel
- drawstring
- drawline
- fillrect
- drawrect
- drawcircle
- fill circle

main.c ********************************

Pong game
- 2 game modes: 
	single player using an accelermoter to move the paddle against an AI
	two player using touch screen
	** Set game mode by changing GAME_MODE

- Gameplay Cycle 
	checks touchscreen or accelerometer to update paddle position
		if single player, AI paddle position is also updated
	update ball position based on velocity
	check for collision
	calculate new velocity for next update
	redraw the screen

- Gameplay
	ball starts with random velocity and direction
	ball velocity is faster when bouncing off edge of paddle
	first to 9 wins
		screen lights up when someone gets to 9
	noise whenever someone scores, ball hits paddle, or ball hits court boundarys
