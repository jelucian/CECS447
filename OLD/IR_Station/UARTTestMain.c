/* Jesus Luciano
 * CECS 447
 * Project 2
 * May 6th, 2019
 *
 * This project is split into code running on two different boards
 * 
 * This file is the file controlling the IR receiver station and LCD
 *
 *
 *
 * IR Protocol
 * 
 * Start pulse: 1.0ms high, 500us low
 * Logical 1  : 1.1ms high, 400us low
 * Logical 0  : 400us high, 400us low
 *
 * For testing purposes
 * PB7 - M0PWM1 - 38Khz
 *
 *
 */

#include "PLL.h"
#include "UART.h"
#include "tm4c123gh6pm.h"
#include "rtl.h"

/***********************************************************************************/
//values for 100 us timer
int counter = 0;

int expected[6] = {10,5,11,4,4,4};//hi, lo times for start, 1 and 0 in pairs
int data[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int cap_en = 0;//capture enable
int d_index = 0;//data capture index

int in[8] = {0,0,0,0,0,0,0,0};
int in_index = 0;
	
int LCD_en = 0;
int i;

int start = 0;
/***********************************************************************************/
void process(void);
void interpret(void);

int abs(int x){
	if(x<0)
	{
		x *= -1;
	}
	return x;
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


/*void PortB_Init(){ unsigned long delay;
	SYSCTL_RCGCPWM_R  |= 0x01;					//activate PWM module 0
	SYSCTL_RCGC2_R 		|= 0x00000002;		//activate Port B
	delay = SYSCTL_RCGC2_R;
	GPIO_PORTB_CR_R	  |= 0x80;					//allow changes to PB7
	GPIO_PORTB_AMSEL_R&= 0x7F;					//disable analog for PB7
  GPIO_PORTB_PCTL_R &= 0x0FFFFFFF;		//clear PB7 as GPIO
	GPIO_PORTB_DIR_R  &= 0x7F;					//set PB7 as input
	GPIO_PORTB_AFSEL_R&= 0x7F;					//disable alt func PB7
	GPIO_PORTB_DEN_R  |= 0x80;					//digital enable PB7
	
	**************************
	GPIO_PORTB_IS_R		 &= ~0x80; //PB7 edge sensitive
	GPIO_PORTB_IBE_R   |=  0x80; //PB7 both edges
	//GPIO_PORTB_IEV_R   &=  0x11; //PB7 falling edge
	GPIO_PORTB_ICR_R	 |=  0x80; //PB7 clear flags
	GPIO_PORTB_IM_R    |=  0x80; //arm interrupt
	
	//Port B interrupt 1
	//EN0
	NVIC_PRI0_R = (NVIC_PRI0_R&0xFFFF00FF)|0x00000000; 	// priority 0 interrupt for switches				 
  NVIC_EN0_R  = 0x00000002;      			// enable interrupt 1 in NVIC
}*/

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
	GPIO_PORTF_PUR_R  |= 0x01;//0x11;					//pull up resistors PF4,0
	GPIO_PORTF_DEN_R  |= 0x1F;					//digital enable PF4,3,2,1,0
	
	//Interrupt
	GPIO_PORTF_IS_R		 &= ~0x11; //PF4 edge sensitive
	GPIO_PORTF_IBE_R   |=  0x11; //PF4 not both edges
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
void PortB_Handler(){
	GPIO_PORTF_DATA_R = 0x0E;
	//detect both edges
	if(GPIO_PORTB_DATA_R&0x80)//rising edge
	{
		//if(
		GPIO_PORTF_DATA_R = 0x04;
		counter = 0;
		TIMER0_CTL_R = 0x00000001;    //enable TIMER0A to count time high
	}
	if(~GPIO_PORTB_DATA_R&0x80)//falling edge
	{
		GPIO_PORTF_DATA_R = 0x02;
		counter = 0;
		cap_en = 1;//capture enable
	}
	
	GPIO_PORTB_ICR_R = 0x80;//acknowledge interrupt
}
 
 
void SysTick_Handler(void){
	 
}
 
void GPIOPortF_Handler(void){
	 if(GPIO_PORTF_DATA_R & 0x10)//default high
	 {
		 if(cap_en == 0)//if capture is not enables, set start value high
		 {
			 start = 1;//default high
		 }
		 else//cap_enable high
		 {
			 data[d_index] = counter;
			 d_index++;
		 }
		 //counter = 0;//reset counter
 
	 }
	 if(~GPIO_PORTF_DATA_R & 0x10)//detect a low edge
	 {
			if(start == 1)//default high wave
			{
				cap_en = 1;//first low pulse starts capture
				start = 0;
			}
			if(cap_en == 1)//cap enable high
			{
				data[d_index] = counter;
				d_index++;
			}
			counter = 0;//reset counter
	 }
	 
	 if(d_index > 15)//data input is full
	 {
		 process();
		 cap_en = 0;
	 }
		 
	 GPIO_PORTF_ICR_R = 0x11; //acknowledge interrupt
	 
 }
 
void Timer0A_Handler(){//called every 100 us
	 TIMER0_ICR_R = 0x00000001; //acknowledge timer0A flag
	 if(counter < 40) //&& cap_en == 1)//counter less than max value and capture is enabled
	 {
		 	counter++;//increase counter
	 }
	
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
	//PortB_Init();
	PortF_Init();
	SysTick_Init();
	Timer0_Init();
	
  while(1)
	{
		
		
  }
}

//process data from data register
void process(void){
	int score_s= 0, score_z=0, score_o=0;
	int c = 0;
  in_index = 0;
		//GPIO_PORTF_DATA_R |= 0x02;
	for(i = 0; i < 16; i++)
	{
		c += data[i];
	}
	// 15 
	if(data[1] > 20)
	{
		GPIO_PORTF_DATA_R = 0x04;
	}
	else
	{
		GPIO_PORTF_DATA_R = 0x02;
	}
	
	
	//data register holds 16 values
	for(i = 0; i < 16; i+=2)
	{
							//		hi														lo
		score_s = abs(data[i]-expected[0]) + abs(data[i+1]-expected[1]);
		score_o = abs(data[i]-expected[2]) + abs(data[i+1]-expected[3]);
		score_z = abs(data[i]-expected[4]) + abs(data[i+1]-expected[5]);
		//check for score values by finding lowest value
		if(score_s < score_o && score_s < score_z)//s is most likely
		{
			in[in_index] = 9;//9 is a placeholder for s
		}
		else if(score_o < score_s && score_o < score_z)//one is most likely
		{
			in[in_index] = 1;
		}
		else//zero is most likely
		{
			in[in_index] = 0;
		}
		in_index++;
		
	}
	//interpret data
	interpret();
	
}

//interpret data in the in array
void interpret(void){
//	int t = 0;
	/*
	//debug
	if(in[t] == 0)
	{
		GPIO_PORTF_DATA_R = 0x02;//red
	}
	else if(in[t] == 1)
	{
		GPIO_PORTF_DATA_R = 0x04;//blue
	}
	else
	{
		GPIO_PORTF_DATA_R = 0x08;//green
	}
	*/
	
	
	
	
	//check address first, locs 2 & 3 need to be 1 1
	//if(in[2] == 1 && in[3] == 1)
	//{
		LCD_en = 0;//enable LCD change if address is correct
	//}
	if(LCD_en)
	{
		//check which animation to play 0-4
		//in in locations 4, 5, 6
				 if(in[4] == 1 && in[5] == 0 && in[6] == 0)//command 4
		{
			GPIO_PORTF_DATA_R = 0x02;
		}
		else if(in[4] == 0 && in[5] == 1 && in[6] == 1)//command 3
		{
			GPIO_PORTF_DATA_R = 0x04;
		}
		else if(in[4] == 0 && in[5] == 1 && in[6] == 0)//command 2
		{
			GPIO_PORTF_DATA_R = 0x08;
		}
		else if(in[4] == 0 && in[5] == 0 && in[6] == 1)//command 1
		{
			GPIO_PORTF_DATA_R = 0x06;
		}
		else if(in[4] == 0 && in[5] == 0 && in[6] == 0)//command 0
		{
			GPIO_PORTF_DATA_R = 0x0A;
		}
		else//error checking
		{
			GPIO_PORTF_DATA_R = 0x0E;//white for error
		}
		
		
	}
	
	
	//delete all data in the in array after using it
	/*
	for(i = 0; i < 8; i++)
	{
		in[i] = 0;
	}
	*/
	
}

// Color    LED(s) PortF
// dark     ---    0
// red      R--    0x02
// blue     --B    0x04
// green    -G-    0x08
// yellow   RG-    0x0A
// sky blue -GB    0x0C
// white    RGB    0x0E
// pink     R-B    0x06
