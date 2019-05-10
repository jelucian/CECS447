/* Jesus Luciano
 * CECS 447
 * Project 2
 * May 6th, 2019
 *
 * This project is split into code running on two different boards
 * 
 * This file is the file controlling the car
 *
 * PC4 - A1 B
 * PC5 - A1 A
 * PC6 - B1 B
 * PC7 - B1 A
 *
 * PA7 - IR LED modulated output
 *		 - M1_PWM3
 *
 * PB0 - UART1 Rx
 * PB1 - UART1 Tx
 *
 * UART Configuration
 * 
 * 57600 Baud rate
 * 8 Data Bits
 * 1 Stop Bit
 * Odd Parity
 *
 *
 * IR Protocol
 * 
 * Start pulse: 1.0ms high, 500us low
 * Logical 1  : 1.1ms high, 400us low
 * Logical 0  : 400us high, 400us low
 *
 *
 */

#include "PLL.h"
#include "UART.h"
#include "tm4c123gh6pm.h"
#include "rtl.h"
/***********************************************************************************/
//values for 100 us timer
int start_hi = 10, start_lo = 5,
		 one_hi  = 11,   one_lo = 4,
		zero_hi  =  4,  zero_lo = 4;
int start = 1, one = 1, zero = 1;

int s_en = 0, o_en = 0, z_en = 0;


//timing
int start_bit[2] = {10, 4};

int delay = 10; // 1ms delay between data pulses

int address[4][4]  ={{ 4, 4, 4, 4}, //00
										 { 4, 4,11, 4},	//01
										 {11, 4, 4, 4},	//10
										 {11, 4,11, 4}};//11

int command[5][8] = {{ 4, 4, 4, 4, 4, 4, 4, 4}, //0000
										 { 4, 4, 4, 4, 4, 4,11, 4},	//0001
										 { 4, 4, 4, 4,11, 4, 4, 4},	//0010
										 { 4, 4, 4, 4,11, 4,11, 4}, //0011 
										 { 4, 4,11, 4, 4, 4, 4, 4}};//0100
				
int counter = 0;
int status = 0;
BIT change = 1;
/***********************************************************************************/

void PortA_Init(){ unsigned long delay;
	SYSCTL_RCGCPWM_R  |= 0x02;					//activate PWM module 1
	SYSCTL_RCGC2_R 		|= 0x00000001;		//activate Port A
	delay = SYSCTL_RCGC2_R;
	GPIO_PORTA_CR_R	  |= 0x80;					//allow changes to PA7
	GPIO_PORTA_AMSEL_R&= 0xEF;					//disable analog for PA7
	GPIO_PORTA_PCTL_R &= 0x0FFFFFFF; 		//set PA7 as PWM output
	GPIO_PORTA_PCTL_R |= 0x50000000;
	GPIO_PORTA_DIR_R  |= 0x80;					//set PA7 as output
	GPIO_PORTA_AFSEL_R|= 0x80;					//enable alt func PA7
	GPIO_PORTA_DEN_R  |= 0x80;					//digital enable PA7
	
	SYSCTL_RCGCPWM_R  |= 0x02;					//activate PWM M1
	SYSCTL_RCGCGPIO_R |= 0x01; 					//clock for Port A
	SYSCTL_RCC_R 			&=~0x00100000; 		//disable divider
	
	PWM1_1_CTL_R 			 = 0x00;					//disable PWM for initializations
	PWM1_1_GENB_R     |= 0x00000C08; 		//drive PWM b high, invert pwm b
	PWM1_1_LOAD_R 		 = 421-1; 			  //needed for 38khz
	PWM1_1_CMPB_R 		 = 210; 					//50% duty cycle?
	//PWM1_1_CTL_R 			&=~0x00000010; 	//set to countdown mode
	PWM1_1_CTL_R 			|= 0x00000001; 		//enable generator
	
	PWM1_ENABLE_R 		|= 0x08; //enable output 3 of module 1
}
void Timer0_Init(){
	SYSCTL_RCGCTIMER_R |= 0x01; //Enable Timer0
	TIMER0_CTL_R   = 0; //disable timer0a for setup
	TIMER0_CFG_R   = 0x00000000; //32 bit mode
	TIMER0_TAMR_R  = 0x00000002; //periodic mode, down-count
	TIMER0_TAILR_R = 1600;//100 us period
	TIMER0_TAPR_R  = 0;//bus clock resolution?
	TIMER0_ICR_R   = 0x00000001;//clear TIMER0A flag
	TIMER0_IMR_R   = 0x00000001;//arm timer interrupt
	NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x20000000; //priority 1
	NVIC_EN0_R = 1<<19;
	TIMER0_CTL_R = 0x00000001;    //enable TIMER0A

}

void PortB_Init(){ unsigned long delay;
	SYSCTL_RCGC2_R 		|= 0x00000002;		//activate Port B
	delay = SYSCTL_RCGC2_R;
	GPIO_PORTB_CR_R	  |= 0x03;					//allow changes to PB1,0
	GPIO_PORTB_AMSEL_R&= 0xFC;					//disable analog for PB1,0
  GPIO_PORTB_PCTL_R &= 0xFFFFFF00;		//clear PB0, 1
	GPIO_PORTB_PCTL_R |= 0x00000011;    //set PB0, 1 as UART1
	GPIO_PORTB_DIR_R  &= 0xFE;					//set PB0 as input
	GPIO_PORTB_DIR_R  |= 0x02;					//set PB1 as output
	GPIO_PORTB_AFSEL_R|= 0x03;					//enable alt func PB1,0
	GPIO_PORTA_DEN_R  |= 0x03;					//digital enable PB1,0
	
	//UART Initialization
	SYSCTL_RCGC1_R    |= 0x00000002;		//enable UART1
	//UART1_CTL_R       |=
}
void PortC_Init(){ unsigned long delay;
	SYSCTL_RCGC2_R 		|= 0x00000004;		//activate Port C
	delay = SYSCTL_RCGC2_R;
	GPIO_PORTC_LOCK_R  = 0x4C4F434B;			//unlock Port C
	GPIO_PORTC_CR_R	  |= 0xF0;					//allow changes to PC7,6,5,4
	GPIO_PORTC_AMSEL_R&= 0x0F;					//disable analog for PC7,6,5,4
	GPIO_PORTC_PCTL_R &= 0x0000FFFF;		//set PC7,6,5,4 as GPIO
	GPIO_PORTC_DIR_R  |= 0xF0;					//set PC7,6,5,4 as output
	GPIO_PORTC_AFSEL_R&= 0x0F;					//disable alt func PC7,6,5,4 
	GPIO_PORTC_PUR_R  |= 0xF0;
	GPIO_PORTC_DEN_R  |= 0xF0;					//digital enable PC7,6,5,4
	
}
void PortF_Init(){ unsigned long delay;//for LED debugging
	SYSCTL_RCGC2_R 		|= 0x00000020;		//activate Port F
	delay = SYSCTL_RCGC2_R; 
	GPIO_PORTF_LOCK_R = 0x4C4F434B;			//unlock Port F
	GPIO_PORTF_CR_R	  |= 0x1F;					//allow changes to PF4,3,2,1,0
	GPIO_PORTF_AMSEL_R&= 0xE0;					//disable analog for PF4,3,2,1,0
	GPIO_PORTF_PCTL_R &= 0xFFF00000;		//set PF4,3,2,1,0 as GPIO
	GPIO_PORTF_DIR_R  &= 0xEE;					//set PF4,0 as input
	GPIO_PORTF_DIR_R  |= 0x0E;					//set PF3,2,1 as output
	GPIO_PORTF_AFSEL_R&= 0xE0;					//disable alt func PF4,3,2,1,0
	GPIO_PORTF_PUR_R  |= 0x11;					//pull up resistors PF4,0
	GPIO_PORTF_DEN_R  |= 0x1F;					//digital enable PF4,3,2,1,0
	
	//Interrupt
	GPIO_PORTF_IS_R		 &= ~0x11; //PF4 edge sensitive
	GPIO_PORTF_IBE_R   &= ~0x11; //PF4 not both edges
	GPIO_PORTF_IEV_R   &=  0x11; //PF4 falling edge
	GPIO_PORTF_ICR_R	 |=  0x11; //PF4 clear flags
	GPIO_PORTF_IM_R    |=  0x11; //arm interrupt
	
	NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00000000; 	// priority 0 interrupt for switches				 
  NVIC_EN0_R  = 0x40000000;      			// enable interrupt 30 in NVIC
 }

void SysTick_Init(){
	 NVIC_ST_CTRL_R = 0; //disable systick
	 NVIC_ST_RELOAD_R = 0x00FFFFFF; // period
	 NVIC_ST_CURRENT_R = 0; //reset current couter value
	 //NVIC_ST_CTRL_R |= 0x00000007;
 }
 
/***********************************************************************************/
void LED_on(){
	PWM1_1_CMPB_R = 210;

}
void LED_off(){
	PWM1_1_CMPB_R = 1;
}

/***********************************************************************************/
void SysTick_Handler(void){

}
 
void GPIOPortF_Handler(void){
	 //changing the A's
	 if(GPIO_PORTF_DATA_R & 0x01){
		 LED_on();
		 GPIO_PORTC_DATA_R = GPIO_PORTC_DATA_R ^ 0xC0;
		 GPIO_PORTF_DATA_R = GPIO_PORTF_DATA_R ^ 0x04;
	 }
	 if(GPIO_PORTF_DATA_R & 0x10){
		 LED_off();
		 GPIO_PORTC_DATA_R = 0x00;	
		 GPIO_PORTF_DATA_R = GPIO_PORTF_DATA_R ^ 0x08;
		 
	 }
	 
	 GPIO_PORTF_ICR_R = 0x11; //acknowledge interrupt
	 
 }
 
void Timer0A_Handler(){
	 TIMER0_ICR_R = 0x00000001; //acknowledge timer0A flag
	 //toggle light at 1000 Hz
	 //GPIO_PORTF_DATA_R = GPIO_PORTF_DATA_R ^ 0x02;
	 //handler called every 100 us
	 /*
	 if(start == 1){//hi pulse
		 if(counter < start_hi){
				counter++;
		 }
		 else{
			  counter = 0;
			  LED_off();
			  start = 0;
		 }

	 }
	 else{//start = 0
		 if(counter < start_lo){
				counter++;
		 }
		 else{
			  counter = 0;
			  LED_on();
			  start = 1;
		 }
	 }	
	 */
	 //if(o_en == 1){
		 if(one == 1){//hi pulse
			 if(counter < one_hi){
					counter++;
			 }
			 else{
					counter = 0;
					LED_off();
					one = 0;
			 }

		 }
		 else{//start = 0
			 if(counter < one_lo){
					counter++;
			 }
			 else{
					counter = 0;
					LED_on();
					one = 1;
			 }
		 }
		 //status++;

		 //if(status > 9){
			 //o_en = 0;
			 //status = 0;
		 //}
		 
	 //}
	 /*
	 if(zero == 1){//hi pulse
		 if(counter < zero_hi){
				counter++;
		 }
		 else{
			  counter = 0;
			  LED_off();
			  zero = 0;
		 }

	 }
	 else{//zero = 0
		 if(counter < zero_lo){
				counter++;
		 }
		 else{
			  counter = 0;
			  LED_on();
			  zero = 1;
		 }
	 }
	 */
	
	
}
/***********************************************************************************/

//---------------------OutCRLF---------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
void OutCRLF(void){
  UART_OutChar(CR);
  UART_OutChar(LF);
}

//debug code
int main(void){
  PortA_Init();
	PortB_Init();
	PortC_Init();
	PortF_Init();
	SysTick_Init();
	Timer0_Init();
	
  while(1){
		
  }
}
