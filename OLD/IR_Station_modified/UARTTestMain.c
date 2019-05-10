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
 *
 * turn off timer when decoding
 * sample at 10us and fill an array, when 1st negedge comes, then begin increasing index
 *
 *
 */

#include "PLL.h"
#include "UART.h"
#include "tm4c123gh6pm.h"
#include "rtl.h"

/***********************************************************************************/
char data[1000];

int c_en = 0;//counter enable, only 
int d_in = 0;//data index

int final[6] = {-1,-1,-1,-1,-1,-1};
int dec[11] = {0,0,0,0,0,0,0,0,0,0,0};//holds differences between high and low edge counter values

/***********************************************************************************/
int abs(int x){
	if(x<0)
		x *= -1;
	return x;
}

void decode(void);
void clear(void){
	int i;
	for(i = 0; i < 1000; i++)
	{
		if(i <  6)
			final[i] = 0;
		if(i < 11)
			dec[i] = 0;
		
		data[i] = 1;
	}
	d_in = 0;
	c_en = 0;
}

void Timer0_Init(){//10 us
	SYSCTL_RCGCTIMER_R |= 0x01; //Enable Timer0
	TIMER0_CTL_R   = 0; //disable timer0a for setup
	TIMER0_CFG_R   = 0x00000000; //32 bit mode
	TIMER0_TAMR_R  = 0x00000002; //periodic mode, down-count
	TIMER0_TAILR_R = 160;//10 us period
	TIMER0_TAPR_R  = 0;//bus clock resolution?
	TIMER0_ICR_R   = 0x00000001;//clear TIMER0A flag
	TIMER0_IMR_R   = 0x00000001;//arm timer interrupt
	NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x20000000; //priority 1
	NVIC_EN0_R = 1<<19;
	
	TIMER0_CTL_R = 0x00000001;    //enable TIMER0A
}
void PortF_Init(){ unsigned long delay;//for LED debugging
	SYSCTL_RCGC2_R 		|= 0x00000020;		//activate Port F
	delay = SYSCTL_RCGC2_R; delay++;
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
/***********************************************************************************/
void GPIOPortF_Handler(void){
	 if(~GPIO_PORTF_DATA_R & 0x10)//detect a low edge
	 {
		 if(c_en != 1){
			c_en = 1;
		 }
	 }
	 GPIO_PORTF_ICR_R = 0x11; //acknowledge interrupt
} 
void Timer0A_Handler(){//called every 100 us
	 TIMER0_ICR_R = 0x00000001; //acknowledge timer0A flag
	 
	 data[d_in] = GPIO_PORTF_DATA_R;
	 if(c_en)//counter enabled on negative edge
	 {
		 d_in++;
	 }
	 
	 if(d_in >= 1000)//at max index
	 {		
		 //TIMER0_CTL_R   = 0; //disable timer0a

		 //TIMER0_CTL_R &= ~0x00000001;    //disable TIMER0A
		 d_in = 0;
		 c_en = 0;
		 decode();
		 //clear();
		 //TIMER0_CTL_R = 0x00000001;    //enable TIMER0A
		 
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
	Timer0_Init();
	
  while(1)
	{

  }
}

void decode(void)
{
	int dec[11] = {0,0,0,0,0,0,0,0,0,0,0};//holds differences between high and low edge counter values
	//i.e. counter value between 2 edges
	int i = 200;

	/*****************************************/
	//detect change and track index
	
	int state = 0;
	
	int sum = 0;
	int sum_i = 0;//sum index
	
	int tol = 75;
	
	//TIMER0_CTL_R &= ~0x00000001;    //disable TIMER0A

	/*****************************************/
	//normalize data to 0 and 1
	for(i = 0; i < 1000;i++)
	{
		if(data[i] > 1)
		{
			data[i] = 1;
		}
		else
		{
			data[i] = 0;
		}	
	}
	//detect peaks
	for(i = 1; i < 1000; i++)
	{
		if(data[i] != data[i-1])
		{
			//save index at every peak
			dec[sum] = i;
			sum++;//
		}
		
		
	}
	
	//normalize data, subtract current index value with last index value
	for(i = 10; i > 0; i--)
	{
		dec[i] = dec[i] - dec[i-1];
	}
	
	//get final data
	for(i = 0; i < 11; i=i+2)
	{
		if(dec[i] > tol)
		{
			final[sum_i] = 1;
			sum_i++;
		}			
		else
		{
			final[sum_i] = 0;
			sum_i++;
		}
		
	}

  if(final[0] == 1 && final[1] == 1)//verify address
	{
		     if(final[2] == 0 && final [3] == 0 && final[4] == 0 && final[5] == 0)
		{
			GPIO_PORTF_DATA_R = 0x02;//red - 0
		}
		else if(final[2] == 0 && final [3] == 0 && final[4] == 0 && final[5] == 1)
		{
			GPIO_PORTF_DATA_R = 0x04;//blue - 1
		}
		else if(final[2] == 0 && final [3] == 0 && final[4] == 1 && final[5] == 0)
		{
			GPIO_PORTF_DATA_R = 0x08;//green - 2
		}
		else if(final[2] == 0 && final [3] == 0 && final[4] == 1 && final[5] == 1)
		{
			GPIO_PORTF_DATA_R = 0x0A;//yellow - 3
		}
		else if(final[2] == 0 && final [3] == 1 && final[4] == 0 && final[5] == 0)
		{
			GPIO_PORTF_DATA_R = 0x0C;//sky blue - 4
		}
		else//erorr data
		{
			GPIO_PORTF_DATA_R = 0x06;//error - pink
		}
	}
	else//white light if error
	{
		GPIO_PORTF_DATA_R = 0x0E;//white
	}
	//enable timer
	
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
