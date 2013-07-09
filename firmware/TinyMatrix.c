////////////////////////////////////////////////////////////
// TinyCoinMatrix.c                                       //
// copyright 2012 Tim Toner (tigeruppp/at/gmail.com)      //
// licensed under the GNU GPL v2 or newer                 //
//                                                        //
// Port to ATtiny88 by John Duksta (john/at/duksta.org)   //
// Breakout game added by Dennis Brown (dennis.brown/at/gmail) //
////////////////////////////////////////////////////////////

#include <inttypes.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/pgmspace.h>


// 2D LED buffer
#define ROWS		5
#define COLS		7
char bitmap[ROWS][COLS];	
int need_refresh_line;
int need_render_frame;
int button_count;
int ball_x;
int ball_y;
int ball_y_dir;
int ball_x_dir;
int ball_angle;
int move_sleep;
int dead_sleep;
int t_count;
int in_game;
int is_down;
int lives;
int blocks[ROWS][3];
unsigned char current_row;		// current lit row


// globals
int mode;						// current display mode
int t, frame, frame_delay;		// animation timing variables
int b1, last_b1, b2, last_b2;	// button states


// LED refresh interrupt, called 390 times per second
ISR(TIMER0_COMPA_vect)
{
	need_refresh_line = 1;	// flag to display new row of LEDs

	if (++current_row >= ROWS) 
		current_row = 0;

	if ( (t++%frame_delay) == 0 )
		need_render_frame = 1;

}

// attach a piezo to PORTD6 and GND.
void beep()
{
	int i;

	return;
	//	This routine is disabled.
	//	I'm using PORTD6 as a redundant switch input right now.
	//	It's at the bottom corner of the chip opposite GND.
	//	Perfect place to dead-bug a pushbutton.

	for(i=0; i<100; i++)
	{
		PORTD = _BV(0) | _BV(5) | _BV(6);		_delay_us(100);
		PORTD = 0;		_delay_us(100);
	}
	
}


/////////////////////////////////////////////////////////////////////
// ATTENTION:                                                      //
// The following 3 functions serve as the LED "driver".            //
// Tuned for Lite-ON LTP-747C.                                     //
// They match the columns and rows of the LED to the output pins   //
// of the microcontroller.                                         //
// Adjust these functions to match your LED module.                //
/////////////////////////////////////////////////////////////////////

// Set port pins high and low in such order as to turn off the LED.
// This depends on the LED's cathode / anode arrangement etc.  
void reset_led()
{
	// keep pull-up resistors active	
	PORTD = _BV(6) | _BV(7);
		
	// hi/low the port pins to power off LED. (see datasheets)
	PORTA = _BV(0);
	PORTC = _BV(0) | _BV(3) | _BV(4) | _BV(5) | _BV(7);
	PORTD |= _BV(1);
}

// energize row r (call once)
void set_row(int r) 
{ 
	switch(r)
	{
		case 0:	PORTD |= _BV(2);	break;
		case 1:	PORTD |= _BV(0);	break;
		case 2:	PORTA |= _BV(1);	break;
		case 3:	PORTC |= _BV(2);	break;
		case 4:	PORTC |= _BV(1);	break;
	}
}

// energize col c (call once for each lit pixel in column)
void set_column(int c)
{
	switch(c)
	{
		case 0: PORTA &= ~_BV(0);		break;
		case 1: PORTC &= ~_BV(7);		break;
		case 2: PORTD &= ~_BV(1);		break;
		case 3: PORTC &= ~_BV(0);		break;	
		case 4: PORTC &= ~_BV(5);		break;
		case 5: PORTC &= ~_BV(4);		break;
		case 6: PORTC &= ~_BV(3);		break;
	}	
}

/////////////////////////////////////////////////////////////////////
// end of LED specific hardware code                               //
/////////////////////////////////////////////////////////////////////


// render and energize the current row, based on bitmap array
void refresh_line()
{
	int c;

	reset_led();
	set_row(current_row);
	
	for (c=0; c<COLS; c++) 
		if (bitmap[current_row][c]) set_column(c);

	/* GARY - Tuesday Sept 26 11:44pm */

	need_refresh_line = 0;
}


// zero out the bitmap array
void clear_bitmap()
{
	int c,r;

	for (c=0; c<COLS; c++)
		for (r=0; r<ROWS; r++)
			bitmap[r][c] = 0 ;
}



/////////////////////////////////////////////////////////////////////
//  static 5x7 graphics / symbols                                  //
/////////////////////////////////////////////////////////////////////

#define CHARS 41
const unsigned char alphabet[CHARS][5] PROGMEM =
{ 
	{ 0x0C, 0x12, 0x24, 0x12, 0x0C },	// heart
	{ 0x7E, 0x09, 0x09, 0x09, 0x7E },	// A
	{ 0x7f, 0x49, 0x49, 0x49, 0x36 },	// B
	{ 0x3e, 0x41, 0x41, 0x41, 0x22 },	// C
	{ 0x7f, 0x41, 0x41, 0x22, 0x1c },	// D
	{ 0x7f, 0x49, 0x49, 0x49, 0x63 },	// E
	{ 0x7f, 0x09, 0x09, 0x09, 0x01 },	// F
	{ 0x3e, 0x41, 0x41, 0x49, 0x7a },	// G
	{ 0x7f, 0x08, 0x08, 0x08, 0x7f },	// H
	{ 0x00, 0x41, 0x7f, 0x41, 0x00 },	// I
	{ 0x20, 0x40, 0x41, 0x3f, 0x01 },	// J
	{ 0x7f, 0x08, 0x14, 0x22, 0x41 },	// K
	{ 0x7f, 0x40, 0x40, 0x40, 0x60 },	// L
	{ 0x7f, 0x02, 0x04, 0x02, 0x7f },	// M
	{ 0x7f, 0x04, 0x08, 0x10, 0x7f },	// N
	{ 0x3e, 0x41, 0x41, 0x41, 0x3e },	// O
	{ 0x7f, 0x09, 0x09, 0x09, 0x06 },	// P
	{ 0x3e, 0x41, 0x51, 0x21, 0x5e },	// Q
	{ 0x7f, 0x09, 0x19, 0x29, 0x46 },	// R
	{ 0x46, 0x49, 0x49, 0x49, 0x31 },	// S
	{ 0x01, 0x01, 0x7f, 0x01, 0x01 },	// T
	{ 0x3f, 0x40, 0x40, 0x40, 0x3f },	// U
	{ 0x1f, 0x20, 0x40, 0x20, 0x1f },	// V
	{ 0x3f, 0x40, 0x30, 0x40, 0x3f },	// W
	{ 0x63, 0x14, 0x08, 0x14, 0x63 }, 	// X
	{ 0x07, 0x08, 0x70, 0x08, 0x07 },	// Y
	{ 0x61, 0x51, 0x49, 0x45, 0x43 },	// Z
	{ 0xFF, 0x41, 0x5D, 0x41, 0xFF },	// psycho 2 (27)
	{ 0x00, 0x3E, 0x22, 0x3E, 0x00 },	// psycho 1 (28)
	{ 0x0E, 0x3B, 0x17, 0x3B, 0x0E },	// skull (29)
	{ 0x0A, 0x00, 0x55, 0x00, 0x0A },	// flower (30)
	{ 0x08, 0x14, 0x2A, 0x14, 0x08 },	// diamond (31)
	{ 0x07, 0x49, 0x71, 0x49, 0x07 },	// cup (32)
	{ 0x22, 0x14, 0x6B, 0x14, 0x22 },	// star2 (33)
	{ 0x36, 0x36, 0x08, 0x36, 0x36 },	// star3 (34)
	{ 0x06, 0x15, 0x69, 0x15, 0x06 },	// nuke (35)
	{ 0x0F, 0x1A, 0x3E, 0x1A, 0x0F },	// fox (36)
	{ 0x6C, 0x1A, 0x6F, 0x1A, 0x6C },	// alien (37)
	{ 0x7D, 0x5A, 0x1E, 0x5A, 0x7D },	// alien (38)
	{ 0x4E, 0x7B, 0x0F, 0x7B, 0x4E },	// alien (39)
	{ 0x3D, 0x66, 0x7C, 0x66, 0x3D }	// alien (40)
};

// renders character c onto the bitmap
void render_character(int c)
{
	int x,y, byte;

	clear_bitmap();

	for (y=0; y<ROWS; y++)
	{
		byte = pgm_read_byte(&(alphabet[c][y]));

		for (x=0; x<COLS; x++)
		{
			if (byte & _BV(0)) bitmap[y][x] = 1;
			byte = byte >> 1;
		}		
	}	
}


/////////////////////////////////////////////////////////////////////
//                                                      animations //
/////////////////////////////////////////////////////////////////////

void render_checkerboard()
{
	int c,r;

	frame_delay = 300;

	// fill the frame buffer with a procedural pattern

	for (c=0; c<COLS; c++)
		for (r=0; r<ROWS; r++)
		{
			bitmap[r][c] = (r + c + frame) % 2 ;
		}
}

void render_rain()
{
	int y;

	frame_delay = 20;
	clear_bitmap();

	// this is a modulus based particle system

	y = frame%19;	
	if (y<COLS) bitmap[0][y] = 1;

	y = frame%11;	
	if (y<COLS) bitmap[2][y] = 1;

	y = frame%17;	
	if (y<COLS) bitmap[4][y] = 1;
}


void render_psycho()
{
	frame_delay = 30;

	// simple 2 frame animation
	
	if (frame%2) render_character(27);
	else render_character(28);
}

void render_heartbeat()
{
	frame_delay = 40;

	// how to sequence frames using case statement

	switch (frame%10)
	{
		case 0: render_character(0);	break;	
		case 1: clear_bitmap();			break;	
		case 2: render_character(0);	break;	
		case 3: clear_bitmap();			break;	
	}
}

void render_fire()
{
	int r;

	frame_delay = 40;
	clear_bitmap();

	// another modulus based particle system

	// fire body
	r = (frame+0)%3;	bitmap[0][6-r] = 1;
	r = (frame+1)%2;	bitmap[1][6-r] = 1;
	r = (frame+0)%2;	bitmap[2][6-r] = 1;
	r = (frame+1)%2;	bitmap[3][6-r] = 1;
	r = (frame+1)%3;	bitmap[4][6-r] = 1;

	r = (frame+1)%5;	bitmap[1][6-r] = 1;
	r = (frame+0)%3;	bitmap[2][6-r] = 1;
	r = (frame+2)%5;	bitmap[3][6-r] = 1;

	r = (frame+4)%4;	bitmap[0][6-r] = 1;
	r = (frame+1)%4;	bitmap[1][6-r] = 1;
	r = (frame+0)%4;	bitmap[2][6-r] = 1;
	r = (frame+3)%4;	bitmap[3][6-r] = 1;
	r = (frame+2)%4;	bitmap[4][6-r] = 1;

	// sparks
	r = (frame+0)%19;	if (r<COLS) bitmap[0][6-r] = 1;
	r = (frame+0)%6;	if (r<COLS) bitmap[1][6-r] = 1;
	r = (frame+0)%7;	if (r<COLS) bitmap[2][6-r] = 1;
	r = (frame+2)%6;	if (r<COLS) bitmap[3][6-r] = 1;
	r = (frame+0)%17;	if (r<COLS) bitmap[4][6-r] = 1;

}

void render_game()
{

	frame_delay = 120;
	clear_bitmap();
	
	t_count++;
	if (t_count >= 1000) t_count = 0;
	
		
	// Draw paddle
	if (button_count == 0) {
		bitmap[0][6] = 1;
		bitmap[1][6] = 1;
	}
	else if (button_count == 1) {
		bitmap[1][6] = 1;
		bitmap[2][6] = 1;
	}
	else if (button_count == 2) {
		bitmap[2][6] = 1;
		bitmap[3][6] = 1;
	}
	else if (button_count == 3) {
		bitmap[3][6] = 1;
		bitmap[4][6] = 1;
	}

	//Draw blocks
	int b_x = 0;
	int b_y = 0;
	
	for (b_y = 0; b_y < 3; b_y++) {
		for (b_x = 0; b_x < ROWS; b_x++) {
			bitmap[b_x][b_y] = blocks[b_x][b_y];	
		}
	}

	//Draw Ball
	if (move_sleep <= 0) {
		if (ball_x_dir > 0) {
			ball_x++;
			if (ball_x > 4) {
				ball_x_dir = -1;
				ball_x = 3;
			}
		}
		else {
			ball_x--;
			if (ball_x < 0) {
				ball_x_dir = 1;
				ball_x = 1;
			}
		}
		
		
		if (ball_y_dir > 0) {
			//Detect colisions
			ball_y--;
			
			// Hit the roof
			if (ball_y <= 0) {
				ball_y_dir *= -1;
				if (rand()%2+1 == 0) {
				//if (t_count%7 == 0) {
						ball_x_dir *= -1;
				}
				//ball_y = 0;
			}
			
			//Detect block colisions
			if (bitmap[ball_x][ball_y-1] == 1) {
				blocks[ball_x][ball_y-1] = 0;
				ball_y_dir = -1;
				//if (t_count%2 == 0) {
				if (rand()%2+1 == 0) {
					ball_x_dir = ball_x_dir * -1;
				}
			}
			else if ((bitmap[ball_x-1][ball_y-1] == 1) && (ball_x_dir == -1)) {
				blocks[ball_x-1][ball_y-1] = 0;
				ball_y_dir = -1;
				ball_x_dir = ball_x_dir * -1;
			}
			else if ((bitmap[ball_x+1][ball_y-1] == 1) && (ball_x_dir == 1)) {
				blocks[ball_x+1][ball_y-1] = 0;
				ball_y_dir = -1;
				ball_x_dir = ball_x_dir * -1;
			}			
			else if ((bitmap[ball_x+1][0] == 1) && (ball_x_dir == 1) && (ball_y == 0)) {
				blocks[ball_x+1][0] = 0;
				ball_y_dir = -1;
				ball_x_dir = ball_x_dir * -1;
			}	
			else if ((bitmap[ball_x-1][0] == 1) && (ball_x_dir == 1) && (ball_y == 0)) {
				blocks[ball_x-1][0] = 0;
				ball_y_dir = -1;
				ball_x_dir = ball_x_dir * -1;
			}	
			else if ((bitmap[ball_x-1][ball_y-1] == 1) && (ball_x_dir == 1) && (ball_x == 4)) {
				blocks[ball_x-1][ball_y-1] = 0;
				ball_y_dir = -1;
				ball_x_dir = ball_x_dir * -1;
			}
			else if ((bitmap[1][1] == 1) && (ball_x_dir == -1) && (ball_x == 0) && (ball_y == 2)) {
				blocks[1][1] = 0;
				ball_y_dir = -1;
				ball_x_dir = ball_x_dir * -1;
			}
			if ((bitmap[3][0] == 1) && (ball_x == 2) && (ball_y == 0)) {
				blocks[3][0] = 0;
				ball_y_dir = 1;
				ball_x_dir = ball_x_dir * -1;
			}
		}
		
		else {
			ball_y++;
			if (ball_y < COLS-3) {
				//Detect block colisions
				if (bitmap[ball_x][ball_y+1] == 1) {
					blocks[ball_x][ball_y+1] = 0;
					ball_y_dir = 1;
					ball_x_dir = ball_x_dir * -1;
				}
				else if ((bitmap[ball_x-1][ball_y+1] == 1) && (ball_x_dir == -1)) {
					blocks[ball_x-1][ball_y+1] = 0;
					ball_y_dir = 1;
					ball_x_dir = ball_x_dir * -1;
				}
				else if ((bitmap[ball_x+1][ball_y+1] == 1) && (ball_x_dir == 1)) {
					blocks[ball_x+1][ball_y+1] = 0;
					ball_y_dir = 1;
					ball_x_dir = ball_x_dir * -1;
				}
				else if ((bitmap[ball_x+1][0] == 1) && (ball_x_dir == 1) && (ball_y == 0)) {
					blocks[ball_x+1][0] = 0;
					ball_y_dir = -1;
					ball_x_dir = ball_x_dir * -1;
				}	
				else if ((bitmap[ball_x-1][0] == 1) && (ball_x_dir == 1) && (ball_y == 0)) {
					blocks[ball_x-1][0] = 0;
					ball_y_dir = -1;
					ball_x_dir = ball_x_dir * -1;
				}	

			}
				
			if (ball_y == COLS-2) {
				if (bitmap[ball_x][COLS-1] == 1) {
					//ball_y--;
					ball_y_dir = 1;
					//if (t_count%13 == 0) {
					if (rand()%2+1 == 0) {
						ball_x_dir *= -1;
					}

				}
			}

			
			// DEATH
			if (ball_y == COLS-1) {
				dead_sleep = 8;
				lives--;
			}
			
		}
		move_sleep = 2;
		
	}
	else {
		move_sleep--;
	}
	int is_done = 0;
	for (b_y = 0; b_y < 3; b_y++) {
		for (b_x = 0; b_x < ROWS; b_x++) {
			is_done += blocks[b_x][b_y];	
		}
	}
	if (is_done == 0) {
		dead_sleep = 10;
		lives = 0;
	}
	
	if (dead_sleep <= 0)
	{
		bitmap[ball_x][ball_y] = 1;
	}
	else {
			ball_x = ball_x + 1;
			if (ball_x > 4) ball_x = 0;
			ball_x_dir *= -1;
			ball_y = 2;
			
			ball_y_dir = -1;
			//Init blocks
			int b_x = 0;
			int b_y = 0;
			
			if (lives == 0) {
				// Board reset
				for (b_y = 0; b_y < 3; b_y++) {
					for (b_x = 0; b_x < ROWS; b_x++) {
						blocks[b_x][b_y] = 1;	
					}
				}
			
				//Reset lives
				lives = 2;
			}
			
		dead_sleep--;
	}

	//if (in_game >= 50) {
	//	render_game(); break;
	//}

}

// renders the correct image / animation onto the bitmap
#define MODES 45
void render_buffer()
{
	frame++;
	need_render_frame = 0;
	if (in_game >= 5) {
		render_game(); 
	}
	else {
		switch(mode)
		{
			
			case 1:	render_checkerboard();	break;
			case 2:	render_psycho();		break;
			case 3:	render_heartbeat();		break;
			case 4:	render_rain();			break;
			case 5:	render_fire();			break;
			case 6:	render_character(28);	break;
			case 7: render_character(29);	break;
			case 8: render_character(30);	break;
			case 9: render_character(31);	break;
			case 10: render_character(32);	break;
			case 11: render_character(33);	break;
			case 12: render_character(34);	break;
			case 13: render_character(35);	break;
			case 14: render_character(36);	break;
			case 15: render_character(37);	break;
			case 16: render_character(38);	break;
			case 17: render_character(39);	break;
			case 18: render_character(40);	break;
			case 19: render_character(0);	break;
			case 20: render_character(1);	break;
			case 21: render_character(2);	break;
			case 22: render_character(3);	break;
			case 23: render_character(4);	break;
			case 24: render_character(5);	break;
			case 25: render_character(6);	break;
			case 26: render_character(7);	break;
			case 27: render_character(8);	break;
			case 28: render_character(9);	break;
			case 29: render_character(10);	break;
			case 30: render_character(11);	break;
			case 31: render_character(12);	break;
			case 32: render_character(13);	break;
			case 33: render_character(14);	break;
			case 34: render_character(15);	break;
			case 35: render_character(16);	break;
			case 36: render_character(17);	break;
			case 37: render_character(18);	break;
			case 38: render_character(19);	break;
			case 39: render_character(20);	break;
			case 40: render_character(21);	break;
			case 41: render_character(22);	break;
			case 42: render_character(23);	break;
			case 43: render_character(24);	break;
			case 44: render_character(25);	break;
			case 45: render_character(26);	break;
		}
	}
	if (in_game >= 10) {
		in_game = 0;
		render_game(); 
	}

}

// poll the pushbuttons, and record their states.
// increment/decrement 'mode' in response.
void check_inputs()
{
	// button 1 state (PORTD6)
	if ((PIND & _BV(6)) == 0) b1++;	else { b1 = 0; is_down = 0;}

	// button 2 state (PORTD7)
	if ((PIND & _BV(7)) == 0) b2++;	else b2 = 0;

	// rudimentary de-bouncing
	if (b1 == 10) 		{ 
		mode--; 
		button_count--; 
		if (button_count < 0) {
			button_count = 0;
		}
		need_render_frame = 1; 
		is_down = 1;
	}
	if (b2 == 10) 		{ 
		mode++; 
		button_count++;
		if (button_count > 3) {
			button_count = 3;
		}
	 
		need_render_frame = 1; 
		if (is_down == 1) {
			in_game++;
		}
	
	}
	

	// wraparound (optional)
	if (mode > MODES) mode = 1;
	if (mode < 1) mode = MODES;
}



////////////////////////////////////////////////////////////
//                                         initialization //
////////////////////////////////////////////////////////////
void init()
{
	// set output pins
	DDRA = _BV(0) | _BV(1);
	DDRC = _BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4) | _BV(5) | _BV(7);
	DDRD = _BV(0) | _BV(1) | _BV(2);

	// setup Timer/Counter0 for LED refresh
	TCCR0A = _BV(CTC0) | _BV(CS02);	// Set CTC mode (CTC0) and prescaler clk/256 (CS02)
	OCR0A = 40;				// ~ 390 interrupts/sec (by 5 cols = ~78fps)
	TIMSK0 = _BV(OCIE0A);	// Enable T/C 0A

	sei();

	mode = 1;	// Initial display pattern
	button_count = 2;
	ball_x = 3;
	ball_y = 2;
	dead_sleep = 0;
	ball_y_dir = -1;
	ball_x_dir = -1;
	ball_angle = 90;
	t_count = 223;
	in_game = 0;
	lives = 2;
	int x;
	int y;
	for (x = 0; x < 3; x++) {
		for (y = 0; y < COLS; y++) {
			bitmap[x][y] = 1;
		}
	}
	//Init blocks
	int b_x = 0;
	int b_y = 0;
	
	for (b_y = 0; b_y < 3; b_y++) {
		for (b_x = 0; b_x < ROWS; b_x++) {
			blocks[b_x][b_y] = 1;	
		}
	}

}

//////////////////////////////////////////////////////////// 
//                                              main loop // 
//////////////////////////////////////////////////////////// 
void main_loop() 
{ 
	for (;;)
	{
		if (need_render_frame) render_buffer();		
		if (need_refresh_line)	
		{	
			refresh_line();
			check_inputs();
		}
	}	
}




////////////////////////////////////////////////////////////
//                                                  main  //
////////////////////////////////////////////////////////////
int main (void)
{

	init();
	main_loop();
	return (0);
}
