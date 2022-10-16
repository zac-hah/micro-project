#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "Lib4.h"
#include "TM4C1294NCPDT.h"
// #include "MyDefines.h"  // Your Definition Header File
  
void Setup_PWM(void);
void Setup_GPIO(void);

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
	
	uint32_t psysdiv = (480 / sysclk)-1;
	SYSCTL->RSCLKCFG |= 1<<31 | 1<<30 | 1<<28 | psysdiv;
}

//------------------------------------------------------------------------------
void Setup_PWM(void)
{
		// 1. Enable Clock for PWM Module
    SYSCTL->RCGCPWM |= 0x01;
    while((SYSCTL->PRPWM & 0x01) != 0x01 ){
		}
    // 2. Enable and Setup Clock Divider for PWM Timer
    PWM0->CC  = (1 << 8);           // CC[8]:USEPWMDIV
    PWM0->CC &= ~0x7;               // CC[2:0]=000 PWMDIV
    PWM0->CC |= (0x5);              // CC[2:0]=0x5 divider = /64
    // 3. Disable PWM Generator and Setup the Timer counting mode
    PWM0->_0_CTL = 0x00;            // Disable PWM Generator, and set to count down mode
    // 4. Configure LOAD (Period), CMP (Duty), GEN (PWM Mode) values
    PWM0->_0_LOAD = 50;          // Setup the period of the PWM signal
    PWM0->_0_CMPA = 5;     // Setup the initial duty cycle
		PWM0->_0_CMPB = 5;     // Setup the initial duty cycle
    PWM0->_0_GENA = (0x02 << 6) | (0x03 <<2); // ACTCMPAD=ActLow ACTLOAD=ActHigh
		PWM0->_0_GENB = 0x80c; // ACTCMPAD=ActLow ACTLOAD=ActHigh
    // 5. Enable PWM Generator
    PWM0->_0_CTL |= 1<<0 | 1<<3 | 0x3<<10;           // Enable PWM Generator, with loadupd synchronised
		
		PWM0->_1_CTL = 0x00;            // Disable PWM Generator, and set to count down mode
    // 4. Configure LOAD (Period), CMP (Duty), GEN (PWM Mode) values
    PWM0->_1_LOAD = 31058;          // Setup the period of the PWM signal
    PWM0->_1_CMPA = 1397;     // Setup the initial duty cycle
		PWM0->_1_CMPB = 197;     // Setup the initial duty cycle
    PWM0->_1_GENA = (0x02 << 6) | (0x03 <<2); // ACTCMPAD=ActLow ACTLOAD=ActHigh
		PWM0->_1_GENB = 0x80c; // ACTCMPAD=ActLow ACTLOAD=ActHigh
    // 5. Enable PWM Generator
    PWM0->_1_CTL |= 1<<0 | 1<<3 | 0x3<<10;           // Enable PWM Generator
		
		/*PWM0->_2_CTL = 0x00;            // Disable PWM Generator, and set to count down mode
    // 4. Configure LOAD (Period), CMP (Duty), GEN (PWM Mode) values
    PWM0->_2_LOAD = 31060;          // Setup the period of the PWM signal
    PWM0->_2_CMPA = 198;     // Setup the initial duty cycle
		PWM0->_2_CMPB = 198;     // Setup the initial duty cycle
    PWM0->_2_GENA = (0x02 << 6) | (0x03 <<2); // ACTCMPAD=ActLow ACTLOAD=ActHigh
		PWM0->_2_GENB = 0x80c; // ACTCMPAD=ActLow ACTLOAD=ActHigh
    // 5. Enable PWM Generator
    PWM0->_2_CTL |= 1<<0 | 1<<3;           // Enable PWM Generator*/
		
		__disable_irq();
		// disable interrupts
		PWM0->INTEN = 0;
		/*GPIOF_AHB->IS |= (1<<2);  // pf2 level sensitive
		GPIOF_AHB->IBE &= ~(1<<2);  // pf2 not both edges
		GPIOF_AHB->IEV &= ~(1<<2);  // low level triggered
		GPIOA_AHB->ICR |= (1<<2);  // clear previous interrupt
		GPIOF_AHB->IM |= (1<<2);  // unmask interrupt*/
		
		NVIC->IP[13] = 1;  // priority
		NVIC->ISER[0] |= (1<<PWM0_1_IRQn);  // enable interrupt for irqn
		
		// |13:cmpb down|12:cmpb up|11:cmpa down|10:cmpa up|9:tr on cnt load|8:tr cnt 0|<--adc 7~6 raw-->|5:cmpb down|3:cmpb up|2:cmpa dn|1:load|0:int on 0?|
		PWM0->_1_INTEN = 1<<0;
		// enable
		PWM0->INTEN |= 0xf;
		__enable_irq();
    // 6. Enable PWM Output
    PWM0->ENABLE |= 0xf;            // Enable PWM0
}

void Setup_GPIO(void)
{
    // GPIO Initialization and Configuration
    // 1. Enable Clock to the GPIO Modules (SYSCTL->RCGCGPIO)
    SYSCTL->RCGCGPIO |= (1<<5) | (1<<12);
    // allow time for clock to stabilize (SYSCTL->PRGPIO)
    while((SYSCTL->PRGPIO & (1<<5)) != (1<<5)){
		}
		
		// port n
		GPION->DIR |= 1<<2;
		GPION->DEN |= 1<<2;
		
    // 4. Set Port Control Register for each Port (GPIO->PCTL, check the PCTL table)
		GPIOF_AHB->PCTL |= 0x6666;
    // 5. Set Alternate Function Select bits for each Port (GPIO->AFSEL  0=regular I/O, 1=PCTL peripheral)
    GPIOF_AHB->AFSEL |= 0xf;
		// 6. Set Output pins for each Port (Direction of the Pins: GPIO->DIR  0=input, 1=output)
    GPIOF_AHB->DIR |= 0xf;
		GPIOF_AHB->DR12R |= 0xf;
		GPIOF_AHB->DR8R |= 0xf;
		GPIOF_AHB->DR4R |= 0xf;
		// 7. Set PUR bits (internal Pull-Up Resistor), PDR (Pull-Down Resistor), ODR (Open Drain) for each Port (0: disable, 1=enable)
    //GPIOF_AHB->ODR = 0;
		//GPIOF_AHB->PDR |= 1<<0;
		// 8. Set Digital ENable register on all GPIO pins (GPIO->DEN 0=disable, 1=enable)
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

int h_p;
int v_p;

void PWM0_1_Handler(void)
{
	while(PWM0->_0_COUNT > 42) {
		// Wait for line to start, reduces jitter
	}
	/*h_p = (int)((PWM0->_0_COUNT - 8) / 3);
	v_p = (int)(PWM0->_1_COUNT - 256) / 3300;
	// In horiz. range if before front porch (12) and after back porch (48.125)
	//bool in_horiz = (PWM0->_0_COUNT > 8) && (PWM0->_0_COUNT < 45);
	// In vert. range if before front porch (247.5) and after back porch (29919)
	//bool in_vert = (PWM0->_1_COUNT > 256) && (PWM0->_1_COUNT < 29880);
	if ((PWM0->_1_COUNT > 256) && (PWM0->_1_COUNT < 29952) && (PWM0->_0_COUNT > 8) && (PWM0->_0_COUNT < 44)) {
		GPION->DATA = img[h_p][v_p]<<2;
	} else {
		GPION->DATA = 0<<2;
	}*/
	
	// Using wait instead of PWM COUNT
	v_p = (int)(PWM0->_1_COUNT - 256) / 3300;
	for (int h_p=0; h_p<12; h_p+=1) {
		if ((PWM0->_1_COUNT > 256) && (PWM0->_1_COUNT < 29952) && (PWM0->_0_COUNT > 8) && (PWM0->_0_COUNT < 43)) {
			GPION->DATA = img[h_p][v_p]<<2;
			for (int wait=0; wait<147; wait++) {
				__ASM("nop");
			}
		} else {
			GPION->DATA = 0<<2;
		}
	}
}

/*void GPIOF_Handler(void)
{
	//GPION->DATA ^= 1;
}*/ 

int main()
{
	h_p = 0;
	setSysClk(120);	
	Setup_PWM();
	Setup_GPIO();
	serialBegin();
	while(1) {
		// Required to reload the PWM timers synchronously
		PWM0->CTL |= 0x3;  // ''''';;;;;\\\oibnrtssssssssssssssssssssssssssssssssssssssssseed
	}
}
