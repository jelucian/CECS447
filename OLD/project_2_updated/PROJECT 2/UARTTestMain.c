
// U0Rx (VCP receive) connected to PB0
// U0Tx (VCP transmit) connected to PB1

/*
*  
*
*
*
*
*
*
*
*
*
*/

#include "PLL.h"
#include "UART.h"
#include "tm4c123gh6pm.h"

void PortF_Init(void);
void PortC_Init(void);
void PortA_Init();
void Timer0_Init();
void LED_off();
void LED_on();

/***********************************************************************************/
//values for 100 us timer
int start_hi = 10, start_lo = 5,
		 one_hi  = 11,   one_lo = 4,
		zero_hi  =  4,  zero_lo = 4;
int start = 1, one = 1, zero = 1;

int s_en = 0, o_en = 0, z_en = 0;

//timing
int start_bit[2] = {10, 4};

int delay = 20; // 1ms delay between data pulses

int address[4][4]  ={{ 4, 4, 4, 4}, //00
										 { 4, 4,11, 4},	//01
										 {11, 4, 4, 4},	//10
										 {11, 4,11, 4}};//11

int command[5][8] = {{ 4, 4, 4, 4, 4, 4, 4, 4}, //0000
										 { 4, 4, 4, 4, 4, 4,11, 4},	//0001
										 { 4, 4, 4, 4,11, 4, 4, 4},	//0010
										 { 4, 4, 4, 4,11, 4,11, 4}, //0011 
										 { 4, 4,11, 4, 4, 4, 4, 4}};//0100

int counter = 0;//general counter										 
										 
int st = 0;//start bit indexs
										 
int add = 3;//address
int add_c = 0;//address index countercounter 		

int com = 0;//command
int com_c = 0;//command index

int send_e = 0;		

int state = 0;
//state 0 - start bit
//state 1 - address
//state 2 - command													 
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
	char character;
  PLL_Init();
  UART_Init();              // initialize UART
  OutCRLF();
  PortF_Init();
	PortC_Init();
	PortA_Init();
	Timer0_Init();
	LED_off();
	GPIO_PORTC_DATA_R = 0xF0;

  while(1){
		
		character = UART_InChar();

		// FORWARD
		if(character=='w')
		{
			GPIO_PORTF_DATA_R = 0x02;
			GPIO_PORTC_DATA_R = GPIO_PORTC_DATA_R ^ 0x50;
		}
		
		// LEFT
		else if(character=='a')
		{
			GPIO_PORTF_DATA_R = 0x04;
			GPIO_PORTC_DATA_R = GPIO_PORTC_DATA_R ^ 0x60;
		}
		
		// RIGHT
		else if(character=='d')
		{
			GPIO_PORTF_DATA_R = 0xA0;
			GPIO_PORTC_DATA_R = GPIO_PORTC_DATA_R ^ 0x90;
		}
		
		// REVERSE
		else if(character=='s')
		{
			GPIO_PORTF_DATA_R = 0x08;
			GPIO_PORTC_DATA_R = GPIO_PORTC_DATA_R ^ 0xA0;
		}
		else if(character=='c')
		{
			//change device address
			
		}
		else if(character=='0')
		{
			//command 0
			state = 0;
			GPIO_PORTF_DATA_R = 0x0E;
			send_e = 1;
			com = 0;
			add_c = 0;
		}
		else if(character=='1')
		{
			//command 1
			state = 0;
			send_e = 1;
			com = 1;
			com_c = 0;
			add_c = 0;
		}
		else if(character=='2')
		{
			//command 2
			state = 0;
			send_e = 1;
			com = 2;
			com_c = 0;
			add_c = 0;			
		}
		else if(character=='3')
		{
			//command 3
			state = 0;
			send_e = 1;
			com = 3;
			com_c = 0;
			add_c = 0;
		}
		else if(character=='4')
		{
			//command 4
			state = 0;
			send_e = 1;
			com = 4;
			com_c = 0;
			add_c = 0; 
		}
		else if(character=='8')//debugging
		{
			LED_on();
		}
		else if(character=='9')//debugging
		{
			LED_off();
		}
		
		
		
  }
}

void PortF_Init(void){ volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x00000020;     // 1) F clock
  delay = SYSCTL_RCGC2_R;           // delay   
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock PortF PF0  
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0       
  GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog function
  GPIO_PORTF_PCTL_R = 0x00000000;   // 4) GPIO clear bit PCTL  
  GPIO_PORTF_DIR_R = 0x0E;          // 5) PF4,PF0 input, PF3,PF2,PF1 output   
  GPIO_PORTF_AFSEL_R = 0x00;        // 6) no alternate function
	GPIO_PORTF_PUR_R = 0x11;
  GPIO_PORTF_DEN_R = 0x1F;          // 7) enable digital pins PF4-PF0        
}

void PortC_Init(){ unsigned long delay;
	SYSCTL_RCGC2_R 		|= 0x00000004;		//activate Port C
	delay = SYSCTL_RCGC2_R;
	GPIO_PORTC_LOCK_R = 0x4C4F434B;			//unlock Port C
	GPIO_PORTC_CR_R	  |= 0xF0;					//allow changes to PC7,6,5,4
	GPIO_PORTC_AMSEL_R&= 0x0F;					//disable analog for PC7,6,5,4
	GPIO_PORTC_PCTL_R &= 0x0000FFFF;		//set PC7,6,5,4 as GPIO
	GPIO_PORTC_DIR_R  |= 0xF0;					//set PC7,6,5,4 as output
	GPIO_PORTC_AFSEL_R&= 0x0F;					//disable alt func PC7,6,5,4 
	GPIO_PORTC_DEN_R  |= 0xF0;					//digital enable PC7,6,5,4
	
}


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
	PWM1_1_LOAD_R 		 = 1316-1;//421-1; 			  //needed for 38khz
	PWM1_1_CMPB_R 		 = 658;//210; 					//50% duty cycle?
	//PWM1_1_CTL_R 			&=~0x00000010; 	//set to countdown mode
	PWM1_1_CTL_R 			|= 0x00000001; 		//enable generator
	
	PWM1_ENABLE_R 		|= 0x08; //enable output 3 of module 1
}

void Timer0_Init(){
	SYSCTL_RCGCTIMER_R |= 0x01; //Enable Timer0
	TIMER0_CTL_R   = 0; //disable timer0a for setup
	TIMER0_CFG_R   = 0x00000000; //32 bit mode
	TIMER0_TAMR_R  = 0x00000002; //periodic mode, down-count
	TIMER0_TAILR_R = 5000;//for 50Mhz clock //1600;//100 us period
	TIMER0_TAPR_R  = 0;//bus clock resolution?
	TIMER0_ICR_R   = 0x00000001;//clear TIMER0A flag
	TIMER0_IMR_R   = 0x00000001;//arm timer interrupt
	NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x20000000; //priority 1
	NVIC_EN0_R = 1<<19;
	TIMER0_CTL_R = 0x00000001;    //enable TIMER0A
}

/***********************************************************************************/
void LED_on(){
	PWM1_1_CMPB_R = 658;//enable pwm

}
void LED_off(){
	PWM1_1_CMPB_R = 0;
}

/***********************************************************************************/
void Timer0A_Handler(){
	 TIMER0_ICR_R = 0x00000001; //acknowledge timer0A flag
	 //handler called every 100 us
	 if(send_e == 1)//send enable
	 {
		 if(state == 0)//start bit
		 {
			 if(counter < start_bit[st])//less than index value ie times to loop in Timer handler
			 {
				 if(st % 2 == 0)//even is on
				 {
					 LED_on();
				 }
				 else//odd index is off
				 {
					 LED_off();
				 }
				 counter++;//increase counter every loop
			 }
			 else
			 {
				 if(st < 1){
					 st++;
				 }
				 else
				 {
					 st = 0;//reset start index
					 state = 1;//next state
				 }
				 counter = 0;
			 }
		 }
		 else if(state == 1)//address
		 {
			 if(counter < address[add][add_c])//less than address and indexs
			 {
				 if(add_c % 2 == 0)//even is on
				 {
					 LED_on();
				 }
				 else//odd index is off
				 {
					 LED_off();
				 }
				 counter++;
			 }
			 else//counter greater than index value
			 {
				 if(add_c < 4)//less than max index
				 {
					 add_c++;
				 }
				 else//max index reached
				 {
					 add_c = 0;//reset address index
					 state = 2;//next state
				 }
				 counter = 0;//reset counter
			 }
		 }
		 else if(state == 2)//command
		 { 
				if(counter < command[com][com_c])//counter less than command index
				{
					if(com_c % 2 == 0)//even index is on
					{
						LED_on();
					}
					else//odd index is off
					{
						LED_off();
					}
					counter++;//increase counter if less than max
				}
				else//counter greater than current index value
				{
					if(com_c < 8)//index less than max index of command array
					{
						com_c++;
					}
					else//reached max index, next state is delay
					{
						com_c = 0;//reset command index
						state = 3;//go to next state
					}
					counter = 0;//reset counter
				}
		 }
		 else//delay state,
		 {
			 if(counter < delay)
			 {
				 LED_off();
				 counter++;
			 }
			 else//reached end of delay
			 {
				 state = 0;//reset state
				 counter = 0;
				 send_e = 0;//turn off send enable
			 }
		 }
		 
		 
	 }
	 else{
		 LED_off();
	 }
	 
}
