/*
	Lab 4 sample code. This code shows you the syntax for creating message queues and 
	receiving them, but it does not permit for an arbitrary number of threads of each type -
	that's your job to figure out. It also uses a fixed delay, rather than a random delay, and
	does not generate any statistics.
	
	Use this code as a starting point to learn how to use the RTX features. Feel free to modify
	it and use it as the start of your own main function. 
	
	Copyright Mike Cooper-Stachowsky, 2022, under the GNU General Public License v3.0, 
	available here: https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "uart.h" //for retargeting printf to UART
#include <cmsis_os2.h> //for RTX functions
#include <lpc17xx.h> //for system init and  MMIO
#include <stdio.h> //for printf - this sample code only initializes printf, but you may choose to use it later on, so it's included here
#include "random.h" //we don't need this here, but it's included so you remember to include it yourselves in your lab
#include <time.h>

#define N 1

int K = 10;
int messageOverflow[N];
int totalMessageSent[N];
int timeElapsed[N];

struct dataPackage{
	int messageOverflow;
	int totalMessageSent;
	int timeElapsed;
	osMessageQueueId_t q_id;
};
//Note: Also need mutex to prevent from reading and writing to timeElapsed at the same time


//This will just set the LEDs to the binary representation of a given unsigned char.
//Quite useful for debugging.
void charToBinLED(unsigned char c)
{
		//& is bitmasking
		if(c&1)
			LPC_GPIO1->FIOSET |= 1<<28;
		else
			LPC_GPIO1->FIOCLR |= 1<<28;
		if(c&2)
			LPC_GPIO1->FIOSET |= 1<<29;
		else
			LPC_GPIO1->FIOCLR |= 1<<29;
		if(c&4)
			LPC_GPIO1->FIOSET |= 1U<<31;
		else
			LPC_GPIO1->FIOCLR |= 1U<<31;
		if(c&8)
			LPC_GPIO2->FIOSET |= 1<<2;
		else
			LPC_GPIO2->FIOCLR |= 1<<2;
		if(c&16)
			LPC_GPIO2->FIOSET |= 1<<3;
		else
			LPC_GPIO2->FIOCLR |= 1<<3;
		if(c&32)
			LPC_GPIO2->FIOSET |= 1<<4;
		else
			LPC_GPIO2->FIOCLR |= 1<<4;
		if(c&64)
			LPC_GPIO2->FIOSET |= 1<<5;
		else
			LPC_GPIO2->FIOCLR |= 1<<5;
		if(c&128)
			LPC_GPIO2->FIOSET |= 1<<6;
		else
			LPC_GPIO2->FIOCLR |= 1<<6;
}

//set the LED pins to be outputs
void initLEDPins()
{
	//set the LEDs to be outputs. You may or may not care about this
	LPC_GPIO1->FIODIR |= 1<<28; //LED on pin 28
	LPC_GPIO1->FIODIR |= 1<<29;
	LPC_GPIO1->FIODIR |= 1U<<31;
	LPC_GPIO2->FIODIR |= 1<<2;
	LPC_GPIO2->FIODIR |= 1<<3;
	LPC_GPIO2->FIODIR |= 1<<4;
	LPC_GPIO2->FIODIR |= 1<<5;
	LPC_GPIO2->FIODIR |= 1<<6;
}

//create a single message queue for testing purposes only. It will be initialized in main
osMessageQueueId_t q_id[N]; 

/*
	
	Our client will send a message once every second

*/
void client(void* args)
{
	int msg = 0;
	uint32_t clientFreq = 9; 
	osMessageQueueId_t threadQueue = *(osMessageQueueId_t*) args;
	while(1)
	{
		msg++;
		
		/*
			Put a message into the queue. We give it the ID of the queue we are posting to,
			the address of the message, 0 for priority (there is only one message, priority is irrelevant), and
			0 for timeout, meaning "either do this immediately or fail"
		
			The put function does return a osStatus_t variable that tells us if it succeeded, which you should use in your 
			lab to determine if the put was successful (meaning the queue had space for the new message...).   In this example
			we will not do anything with this information.
		*/
		
		osStatus_t stat = osMessageQueuePut(q_id,&msg,0,0);
		uint32_t clientDelay = get_random_delay_seconds(clientFreq, osKernelGetTickFreq());
		osDelay(clientDelay); //delay for tick frequency number of ticks. This means 1 second. We do not need to yield because
		//of this delay
		
		if(stat == osOK){
			//printf("\n Message Sent: %d" , msg);
		}
		
		else if(stat == osErrorResource){
			printf("\n Message Not Recieved");
			messageOverflow++;
		}
		
	}
}

void server(void* args)
{
	int receivedMessage = 0;
	int serverFreq = 10;
	osMessageQueueId_t threadQueue = *(osMessageQueueId_t*) args;
	while(1)
	{
		/*
			Get a message from the queue and store it in receivedMessage. 
			The third parameter is for recording the message priority, which we are ignoring.
			Finally, we tell this thread to waitForever, since its only purpose is to receive and handle
			messages - it won't do anything if there are no messages available, so it might as well wait!
			
			This thread does not use any status information
		*/
		osMessageQueueGet(q_id,&receivedMessage,NULL,osWaitForever);
		uint32_t serverDelay = get_random_delay_seconds(serverFreq, osKernelGetTickFreq());
		osDelay(serverDelay); //delay for tick frequency number of ticks. This means 1 second. We do not need to yield because
		
		timeElapsed[0] += serverDelay;
		printf("\nTime Elapsed: %d", timeElapsed[0]);
		
		//we're just going to print the message to the LEDs, mod 256:
		charToBinLED((unsigned char)(receivedMessage % 256));
		
		//We need to yield because it is possible that this thread wakes and sees a message right away and we aren't
		//using priority in this course
		osThreadYield();
	}
}

//monitor thread
void monitor(void* args){
	accquire mutex
	
	printf elapsed time
		int messageOverflow;
	int totalMessageSent;
	int timeElapsed;
	osMessageQueueId_t q_id;
	
	
	
}

int main(void)
{
	//always call this function first
	SystemInit();
	initLEDPins();
	
	//we need to initialize printf outside of any threads
	printf("Project 4 ready\n"); 
	
	//initialize the kernel
	osKernelInitialize();
	
	//initialize our message queue: 10 messages that are integers with default parameters (the "NULL" part means "Let the OS figure out the configuration")
	int count;
	struct dataPackage dataPack[N];
	for(count = 0; count < N; count++){
		//Another datatype
		q_id[count] = osMessageQueueNew(K,sizeof(int),NULL);
		
		
		
		//set up the threads
		osThreadNew(client,&q_id[count],NULL);
		osThreadNew(server,&q_id[count],NULL);	
	}		

	
	osKernelStart();
	
	while(1);
}
