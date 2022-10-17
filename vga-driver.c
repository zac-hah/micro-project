#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "Lib4.h"
#include "TM4C1294NCPDT.h"
  
void Setup_PWM(void);
void Setup_GPIO(void);

// Setting SysClk
#define MINT 96
#define MFRAC 0
#define N 4
#define Q 0
#define PSYSDIV 3
// Used to set memtim0 for flash and eeprom
// These are configured for 100 .. 120 MHz CPU frequency (p277 in the datasheet)
#define FBCHT    0x6
#define EBCHT    0x6
#define FBCE    0
#define EBCE    0
#define FWS    0x5
#define EWS    0x5


void setSysClk(uint32_t sysclk)
{
	SYSCTL->MOSCCTL |= 1<<4;
	SYSCTL->MOSCCTL &= ~(1<<2);
	SYSCTL->MOSCCTL &= ~(1<<3);
	
	// Wait for clock to stabilise
	while(!(SYSCTL->RIS & (1<<8))){
	}
	
	SYSCTL->RSCLKCFG &= ~(0xff << 20);
	SYSCTL->RSCLKCFG |= (0x3 << 20);
	SYSCTL->RSCLKCFG |= (0x3 << 24);
	
	SYSCTL->PLLFREQ0 = MINT;
	SYSCTL->PLLFREQ1 = (Q<<8) | N;
	SYSCTL->PLLFREQ0 |= 1<<23;
	
	SYSCTL->MEMTIM0 = (EBCHT<<22) | (EBCE<<21) | (EWS << 16) | (FBCHT<<6) | FBCE<<5 | FWS;
	SYSCTL->MEMTIM0 |= 1<<4 | 1<<20;
	
	while((SYSCTL->PLLSTAT & 0x01) != 0x01) {
	}
	
	// Set divider
	uint32_t psysdiv = (480 / sysclk)-1;
	SYSCTL->RSCLKCFG |= 1<<31 | 1<<30 | 1<<28 | psysdiv;
}

// Set up PWM ports
void Setup_PWM(void)
{
		// Enable Clock for PWM Module
    SYSCTL->RCGCPWM |= 0x01;
    while((SYSCTL->PRPWM & 0x01) != 0x01 ){
		}
    // Enable and Setup Clock Divider for PWM Timer
    PWM0->CC  = (1 << 8);           // CC[8]:USEPWMDIV
    PWM0->CC &= ~0x7;               // CC[2:0]=000 PWMDIV
    PWM0->CC |= (0x5);              // CC[2:0]=0x5 divider = /64
		// == PWM0 -- HSYNC ==
    // Disable PWM Generator and Setup the Timer counting mode
    PWM0->_0_CTL = 0x00;            // Disable PWM Generator, and set to count down mode
    // Configure LOAD (Period), CMP (Duty), GEN (PWM Mode) values
    PWM0->_0_LOAD = 50;          // Setup the period of the PWM signal
    PWM0->_0_CMPA = 5;     // Setup the initial duty cycle
		PWM0->_0_CMPB = 5;     // Setup the initial duty cycle
    PWM0->_0_GENA = (0x02 << 6) | (0x03 <<2); // ACTCMPAD=ActLow ACTLOAD=ActHigh
		PWM0->_0_GENB = 0x80c; // ACTCMPAD=ActLow ACTLOAD=ActHigh
    // Enable PWM Generator with loadupd synchronised
    PWM0->_0_CTL |= 1<<0 | 1<<3 | 0x3<<10;
		
		// == PWM1 -- VSYNC ==
		PWM0->_1_CTL = 0x00;            // Disable PWM Generator, and set to count down mode
    // 4. Configure LOAD (Period), CMP (Duty), GEN (PWM Mode) values
    PWM0->_1_LOAD = 31058;          // Setup the period of the PWM signal
    PWM0->_1_CMPA = 1397;     // Setup the initial duty cycle
		PWM0->_1_CMPB = 197;     // Setup the initial duty cycle
    PWM0->_1_GENA = (0x02 << 6) | (0x03 <<2); // ACTCMPAD=ActLow ACTLOAD=ActHigh
		PWM0->_1_GENB = 0x80c; // ACTCMPAD=ActLow ACTLOAD=ActHigh
    // 5. Enable PWM Generator
    PWM0->_1_CTL |= 1<<0 | 1<<3 | 0x3<<10;           // Enable PWM Generator
		
		__disable_irq();
		// disable interrupts
		PWM0->INTEN = 0;
		
		NVIC->IP[13] = 1;  // top level priority
		NVIC->ISER[0] |= (1<<PWM0_1_IRQn);  // enable interrupt for irqn
		
		// |13:cmpb down|12:cmpb up|11:cmpa down|10:cmpa up|9:tr on cnt load|8:tr cnt 0|<--adc 7~6 raw-->|5:cmpb down|3:cmpb up|2:cmpa dn|1:load|0:int on 0?|
		PWM0->_1_INTEN = 1<<0;
		// enable
		PWM0->INTEN |= 0xf;  // interrupts for PWM0~PWM3
		__enable_irq();
    PWM0->ENABLE |= 0xf;  // Enable PWM0~PWM3
}

void Setup_GPIO(void)
{
    // GPIO Initialization and Configuration
    // Enable Clock to the GPIO Modules (SYSCTL->RCGCGPIO)
    SYSCTL->RCGCGPIO |= (1<<5) | (1<<12) | (1<<9);
    // allow time for clock to stabilize (SYSCTL->PRGPIO)
    while((SYSCTL->PRGPIO & (1<<5)) != (1<<5)){
		}
		
		// port k -- button input
		GPIOK->DIR = 0;  // input
		GPIOK->DEN |= 0xf;  // enable PA0~PA3
		GPIOK->PDR |= 0xf;
		
		// VGA output
		GPION->DIR |= 1<<2 | 1<<0;
		GPION->DEN |= 1<<2 | 1<<0;
		
    // Set Port Control Register for each Port (GPIO->PCTL, check the PCTL table)
		GPIOF_AHB->PCTL |= 0x6666;
    // Set Alternate Function Select bits for each Port (GPIO->AFSEL  0=regular I/O, 1=PCTL peripheral)
    GPIOF_AHB->AFSEL |= 0xf;
		// Set Output pins for each Port (Direction of the Pins: GPIO->DIR  0=input, 1=output)
    GPIOF_AHB->DIR |= 0xf;
		GPIOF_AHB->DR12R |= 0xf;
		GPIOF_AHB->DR8R |= 0xf;
		GPIOF_AHB->DR4R |= 0xf;
		// Set PUR bits (internal Pull-Up Resistor), PDR (Pull-Down Resistor), ODR (Open Drain) for each Port (0: disable, 1=enable)
		// Set Digital ENable register on all GPIO pins (GPIO->DEN 0=disable, 1=enable)
		GPIOF_AHB->DEN |= 0xf;
}

// Pika
/*bool img[12][9] = {
	{0,0,1,1,1,1,1,0,0},
	{0,0,0,0,1,0,1,0,0},
	{0,0,0,0,0,1,0,0,0},
	{0,0,0,0,0,0,0,0,0},
	{0,0,1,1,0,1,0,0,0},
	{0,0,0,0,0,0,0,0,0},
	{0,0,1,1,1,1,1,0,0},
	{0,0,0,1,1,0,0,0,0},
	{0,0,1,1,0,1,0,0,0},
	{0,0,0,0,0,0,0,0,0},
	{0,0,1,1,0,0,0,0,0},
	{0,0,1,1,1,0,0,0,0},
};*/

// smiley face
bool img[12][9] = {
	{0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,1,0,0},
	{0,0,0,1,0,0,0,0,0},
	{0,0,1,0,0,0,0,0,0},
	{0,0,1,0,0,0,0,0,0},
	{0,0,0,1,0,0,0,0,0},
	{0,0,0,0,0,0,1,0,0},
	{0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0},
};

// snake game
char board[12][9] = {
	{0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0},
	{0,0,0,0,'r',0,0,0,0},
	{0,0,0,0,'r',0,0,0,0},
	{0,0,0,0,'r',0,0,0,0},
	{0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0},
};

// Horiz. pos. , Vert. pos. from top left, 0-indexed
int head[2] = {6, 4};
int tail[2] = {4, 4};

void go_up()
{
	// PA0 -- Up
	// set the last head to this direction, and the next pixel as well
	board[head[0]][head[1]] = 'u';
	board[head[0]][head[1]-1] = 'u';
	head[1] -= 1;
}

void go_down()
{
	// PA1 -- Down
	// set the last head to this direction, and the next pixel as well
	board[head[0]][head[1]] = 'd';
	board[head[0]][head[1]+1] = 'd';
	head[1] += 1;
}

void go_left()
{
	// PA2 -- Left
	// set the last head to this direction, and the next pixel as well
	board[head[0]][head[1]] = 'l';
	board[head[0]-1][head[1]] = 'l';
	head[0] -= 1;
}

void go_right()
{
	// PA3 -- Right
	// set the last head to this direction, and the next pixel as well
	board[head[0]][head[1]] = 'r';
	board[head[0]+1][head[1]] = 'r';
	head[0] += 1;
}

int dbg_c = 0;


int h_p;
int v_p;
int frame_c;

void PWM0_1_Handler(void)
{
	board[0][0] = GPIOK->DATA & 1<<0;
	
	if (PWM0->_1_COUNT < 1) {
		++frame_c;
	}
	if (PWM0->_1_COUNT < 254 ) {
		if (frame_c > 59) {
			frame_c = 0;
			// Wait for line to end
			if ((~GPIOK->DATA & 1<<0) == 1<<0) {
				go_up();
			} else if ((~GPIOK->DATA & 1<<1) == 1<<1) {
				go_down();
			} else if ((~GPIOK->DATA & 1<<2) == 1<<2) {
				go_left();
			} else if ((~ GPIOK->DATA & 1<<3) == 1<<3) {
				go_right();
			} else {
				// set the last head to the head direction, and the next pixel as well
				char headdir = board[head[0]][head[1]];
				switch (headdir) {
					case 'u':
						go_up();
						break;
					case 'd':
						go_down();
						break;
					case 'l':
						go_left();
						break;
					case 'r':
						go_right();
						break;
					default:
						// cry
						break;
				}
			}
			// remove tail
			char lasttail = board[tail[0]][tail[1]];
			board[tail[0]][tail[1]] = 0;
			if (lasttail == 'u') {
				tail[1] -= 1;
			} else if (lasttail == 'd') {
				tail[1] += 1;
			} else if (lasttail == 'l') {
				tail[0] -= 1;
			} else if (lasttail == 'r') {
				tail[0] += 1;
			}
		}
	}
	
	while(PWM0->_0_COUNT > 42) {
		// Wait for line to start, reduces jitter
	}
	// Using wait (instead of PWM COUNT)
	v_p = (int)(PWM0->_1_COUNT - 256) / 3300;
	for (int h_p=0; h_p<12; h_p+=1) {
		if ((PWM0->_1_COUNT > 256) && (PWM0->_1_COUNT < 29952) && (PWM0->_0_COUNT > 8) && (PWM0->_0_COUNT < 43)) {
			GPION->DATA = !board[h_p][v_p]<<2;
			for (int wait=0; wait<147; wait++) {
				__ASM("nop");
			}
		} else {
			GPION->DATA = 0<<2;
		}
	}
}


int main()
{
	frame_c = 0;
	h_p = 0;
	setSysClk(120);	
	Setup_PWM();
	Setup_GPIO();
	serialBegin();
	// Required to reload the PWM timers synchronously
	PWM0->CTL |= 0x3;  // ''''';;;;;\\\oibnrtssssssssssssssssssssssssssssssssssssssssseed
	while(1) {
		while(PWM0->_1_COUNT > 1397) {
			// Wait for line to end
		}
		
		/*if ((GPIOK->DATA & 1<<0) == 1<<0) {
			go_up();
		} else if ((GPIOK->DATA & 1<<1) == 1<<1) {
			go_down();
		} else if ((GPIOK->DATA & 1<<2) == 1<<2) {
			go_left();
		} else if ((GPIOK->DATA & 1<<3) == 1<<3) {
			go_right();
		} else {
			// set the last head to the head direction, and the next pixel as well
			char headdir = board[head[0]][head[1]];
			switch (headdir) {
				case 'u':
					go_up();
					break;
				case 'd':
					go_down();
					break;
				case 'l':
					go_left();
					break;
				case 'r':
					go_right();
					break;
				default:
					// cry
					break;
			}
		}*/
	}
}
