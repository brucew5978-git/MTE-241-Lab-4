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
#include <stdbool.h>


//constants
#define N 2
#define K 10


//total elapsed time in program
uint32_t timeElapsed;

// Data Package
struct dataPackage{
    uint32_t totalMessages;          // Total number of messages sent by the client
    uint32_t messagesReceived;  // Number of messages successfully received by the server
    uint32_t messageOverflow;   // Number of messages dropped by the server
    uint32_t serverTime;        // Total service time (server delay time)
    osMessageQueueId_t q_id;    // Message Queue ID
};


// Global array of size N for keeping track of the client-server pair to logging data mapping
struct dataPackage dataPack[N];

// Mutex for protecting access to the client-pair logging data
osMutexId_t myMutex;

//This will just set the LEDs to the binary representation of a given unsigned char.
//Quite useful for debugging.
void charToBinLED(unsigned char c)
{
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

//Our client will send a message once every second
void client(void* args)
{
  int msg = 0;
    
    while(1)
    {
        msg++;
                    
        uint32_t index = 0;
        
        //obtain thread queue ID from args
        osMessageQueueId_t threadQueue = *(osMessageQueueId_t *)args;
        
        //find index of data package queue ID from argument   
        for (uint32_t i = 0; i < N; i++) {
            if (dataPack[i].q_id == threadQueue) {
                index = i;
                break;
            }
        }
        
        // delay from random time
        osDelay(get_random_delay_seconds(9, osKernelGetTickFreq()));
        
        /*
                    Put a message into the queue. We give it the ID of the queue we are posting to,
                    the address of the message, 0 for priority (there is only one message, priority is irrelevant), and
                    0 for timeout, meaning "either do this immediately or fail"
        */
        
        osStatus_t stat = osMessageQueuePut(threadQueue, &msg, 0, 0);
				
				osMutexAcquire(myMutex, osWaitForever);			
				dataPack[index].totalMessages++;
				osMutexRelease(myMutex);	
        
        //message sent and received successfully
				if (stat == osOK)
        {            						
							osMutexAcquire(myMutex, osWaitForever);			
              dataPack[index].messagesReceived++;
							osMutexRelease(myMutex);	
        }
				//Tracks when messages overflow
       	else if (stat == osErrorResource){ 
					
					    osMutexAcquire(myMutex, osWaitForever);	
							dataPack[index].messageOverflow++;
							osMutexRelease(myMutex);						
              
        }
					
     }
}


//server function
void server(void* args)
{
    int receivedMessage = 0;
    
    while(1)
    {
        //find index of thread queue from argument
        uint32_t index = 0;

        osMessageQueueId_t threadQueue = *(osMessageQueueId_t *)args;
        for (uint32_t i = 0; i < N; i++) {
					if (dataPack[i].q_id == threadQueue) {
							index = i;
							break;
					}
        }
        
/*
            Get a message from the queue and store it in receivedMessage.
            The third parameter is for recording the message priority, which we are ignoring.
            Finally, we tell this thread to waitForever, since its only purpose is to receive and handle
            messages - it won't do anything if there are no messages available, so it might as well wait!
*/
        
        osStatus_t stat = osMessageQueueGet(threadQueue, &receivedMessage, NULL, osWaitForever);
        
        //random delay to ticks
        uint32_t delay = get_random_delay_seconds(10, osKernelGetTickFreq());
        osDelay(delay);
        
        //acquire mutex
        osMutexAcquire(myMutex, osWaitForever);
        
        //update the data package
        dataPack[index].serverTime += delay;
        
        //release mutex
        osMutexRelease(myMutex);
        
        // Print the message to the LEDs, mod 256:
        charToBinLED((unsigned char)(receivedMessage % 256));
        
        // thread may wake and see a message
        osThreadYield();
    }
}

void monitor(void *args) {
    while (1) {
        //update program time
        timeElapsed++;
        
        //acquire mutex
        osMutexAcquire(myMutex, osWaitForever);
    
        //outputting data package
        for (uint32_t i = 0; i < N; ++i){
            double msgLossRatio = (double) dataPack[i].messageOverflow / (double)(dataPack[i].totalMessages);
            double msgArrivalRate = (double) dataPack[i].totalMessages / (double)(timeElapsed);
            double avgServiceRate = (double) dataPack[i].messagesReceived / (double)(dataPack[i].serverTime);
            
            //Data Package Output
            printf("\n%u,%u,%u,%u,%lf,%lf,%lf",
            dataPack[i].totalMessages,        // total messages sent from client                                                                
            dataPack[i].messagesReceived,     // total messages received by server
            dataPack[i].messageOverflow,      // total messages dropped by server
            dataPack[i].serverTime,          // server random sleep time
            msgLossRatio,                     // average message loss ratio
            msgArrivalRate,                   // average arrival rate
            avgServiceRate);                  // average service rate
        }
    
        
        osMutexRelease(myMutex);
				        
				//1 second delay
        osDelay(osKernelGetTickFreq());
    }
}

int main(void)
{
      SystemInit();
      initLEDPins();
      
      // printf initialization
      printf("\n Lab 4 Project Ready ");
      
      // monitor thread woke count initialization
      timeElapsed = 0;
      
      osKernelInitialize();
      
      // mutex initialization
      myMutex = osMutexNew(NULL);
      
      // Create N clients, N servers, and N message queues (N pairs), with queue size of K
      for (uint32_t i = 0; i < N; i++){
            dataPack[i].q_id = osMessageQueueNew(K, sizeof(int), NULL);
            osThreadNew(client, &(dataPack[i].q_id), NULL);
            osThreadNew(server, &(dataPack[i].q_id), NULL);
      }
      
      // Create monitor thread
      osThreadNew(monitor, NULL, NULL);
      
      osKernelStart();
      
      while(1);
}


