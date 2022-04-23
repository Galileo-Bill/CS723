/* Standard includes. */
#include <stdio.h>
#include <conio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Priorities at which the tasks are created. */
#define	Frequency_VirtualISR_PRIORITY	  ( 4 )
#define Frequency_Updater_PRIORITY		  ( 2 )
#define Keyboard_Simulation_ISR_PRIORITY  ( 3 )
#define load_Manager_Task_PRIORITY        ( 1 )
/* definition of task stack*/
#define TASK_STACKSIZE                   2048
//load Manager State
#define NORMAL 0
#define LOAD_MANAGE 1
#define MAINTENANCE 2


/*###############################################
 * ########Queue#################################
 * ##############################################
 */
xQueueHandle frequencyQ;
xQueueHandle freqRocDataQ;


SemaphoreHandle_t thresholdSemaphore;
/*##############################################
 * ########Gobal Var############################
 * #############################################
 */
double VirtualSystemTime = 0;
double num[100] = {0};
double timestamp[100] = {0};
double frequencyThreshold = 49;
double rocThreshold = 300;
int wasStable = 1;
int timerExpiryFlag = 0;
double frequencyData[50] = {0};
double rocData[50] = {0};
int runningDataIndex = 0;
int loads[5] = {1, 2, 4, 8, 16};
int loadStatus[5] = {1,1,1,1,1};
// 1 is load on, 0 is load off
int SWITCHES[5] = {1,1,1,1,1};
int maintainenceMode = 0;
int loadManagerState = NORMAL;
char state;


TaskHandle_t xHandle;
TaskHandle_t yHandle;
TimerHandle_t timer500ms;
/*##############################################
 * #######struct data type######################
 * ############################################3
 */
struct frequency_time
{
	double frequency;
	double   timestamp;
} freq_SIM_ISR;

struct frequency_ROC
{
	double freqData;
	double rocData;
	double timestamp;
};

/*############Init Function###################*/
void initDataStructs(void);
void initCreateTasks(void);

/*############Tasks###########################*/
void FrequencyVirtualISRTask(  );
void FrequencyUpdaterTask(  );
void KeyboardSimulationISRTask( );
void loadManagerTask( );

/*###########Helper function##################*/
int GetVirutalSystemTime ( int VirtualSystemTimeCount );
void updateRunningData(struct frequency_ROC frequency_ROC_Data);
int checkTrippingConditions(struct frequency_ROC frequency_ROC_Data,  double freqThresholdLocal, double rocThresholdLocal);
int shedLoad(int SWITCHES[]);
int reconnectLoad(int SWITCHES[]);
//void computeReactionTimeStatus(double currentTime, struct frequency_ROC frequency_ROC_Data);
/*###########Timer Callback##################*/
void vTimer500MSCallback(TimerHandle_t timer500ms);
void restartFreeRTOSTimer();
void stopFreeRTOSTimer();
/*** SEE THE COMMENTS AT THE TOP OF THIS FILE ***/
void main_blinky( void )
{

	initDataStructs();
	printf("Data struct has initialised!\n");
	initCreateTasks();
	printf("Tasks has initialised!\n");
	xTimerStart(timer500ms, 0);
	printf("Timer is started\n\n");
	/* Start the tasks. */
	vTaskStartScheduler();

	for( ;; );
}
/*-----------------------------------------------------------*/

void initDataStructs(void){
	frequencyQ = xQueueCreate( 100, sizeof(struct frequency_time  ) );
	freqRocDataQ = xQueueCreate(100, sizeof(struct frequency_ROC ) );
	timer500ms = xTimerCreate("Timer", 500, pdFALSE, NULL, vTimer500MSCallback);
	thresholdSemaphore = xSemaphoreCreateMutex();
	return;
}


void initCreateTasks(void){

	xTaskCreate( FrequencyVirtualISRTask, "FrequencyVirtualISRTask", TASK_STACKSIZE , NULL, Frequency_VirtualISR_PRIORITY, &xHandle );
	xTaskCreate( FrequencyUpdaterTask, "FrequencyUpdaterTask", TASK_STACKSIZE , NULL, Frequency_Updater_PRIORITY, &yHandle );
 // xTaskCreate( KeyboardSimulationISRTask, "KeyboardSimulationISRTask", TASK_STACKSIZE , NULL, Keyboard_Simulation_ISR_PRIORITY, NULL );
	xTaskCreate( loadManagerTask, "loadManagerTask", TASK_STACKSIZE , NULL, load_Manager_Task_PRIORITY, NULL );
	return;
}


double GetVirtualSystemTime ( double NI ){
	VirtualSystemTime = VirtualSystemTime + NI / 16;
	return VirtualSystemTime;
}

void FrequencyVirtualISRTask(  )
{
	for ( int i = 0; i < 20; i++){
		num[i] = 320;
		printf("num[%d] is %lf\n", i, num[i]);
	}
	num[20] = 321;
	num[21] = 323;
	num[22] = 324;
	num[23] = 325;
	printf("num[20] is 321\n");
	printf("num[21] is 323\n");
	printf("num[22] is 324\n");
	printf("num[23] is 325\n");
	for ( int i = 24; i < 40; i++){
		num[i] = 327;
		printf("num[%d] is %lf\n", i, num[i]);
	}
	num[40] = 325;
	num[41] = 324;
	num[42] = 323;
	num[41] = 321;
	printf("num[40] is 325\n");
	printf("num[41] is 324\n");
	printf("num[42] is 323\n");
	printf("num[43] is 321\n");
	for ( int i = 42; i < 100; i++){
		num[i] = 320;
		printf("num[%d] is %lf\n", i, num[i]);
	}



	vTaskDelay(100);
	while(1){
		for (int j = 0; j < 100; j++){
			freq_SIM_ISR.frequency = 16000 / num[j];
 	 		freq_SIM_ISR.timestamp = GetVirtualSystemTime(num[j]);

 	 		printf("\n\n---------------------------------\n");
 	 		printf("Timestamp %lf | frequency is %lf\n\n", freq_SIM_ISR.timestamp, freq_SIM_ISR.frequency);
 	 		printf("time_expiry_flag is %d.\n", timerExpiryFlag);
 	 		printf("Current tick count Time is %d\n", xTaskGetTickCount());
 	 		printf("---------------------------------\n\n");
 	 		if ( (freq_SIM_ISR.timestamp > 1840) && (freq_SIM_ISR.timestamp < 2000) ){
 	 			maintainenceMode = 1;
 	 			SWITCHES[1] = 0;
 	 			SWITCHES[4] = 0;
 	 			//load 1 and 4 will be manually off.\n
 	 		}
 	 		//task become normal at 2028
 	 		if (freq_SIM_ISR.timestamp > 2000){
 	 		 	maintainenceMode = 0;
 	 		}
 	 		xQueueSendToFront(frequencyQ, &freq_SIM_ISR, 0 );
 	 		vTaskDelay(100);
		}
	}
}
/*-----------------------------------------------------------*/

void FrequencyUpdaterTask( )
{
		float freqValNew = 0;
		float freqValOld = 0;
		float roc = 0;

		uint8_t isFirstIteration = 1;

		struct frequency_time ReceiveGenerateFre;
		struct frequency_ROC frequency_ROC_Data;

		while (1){
			if (xQueueReceive(frequencyQ, &ReceiveGenerateFre, 0) == pdPASS){
				printf("\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				printf("Frequency ROC will be caculated. \n");
				freqValNew = ReceiveGenerateFre.frequency;

				if(isFirstIteration){
					isFirstIteration = 0;
					freqValOld = freqValNew;
					printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
					continue;
				}
				printf("FreqValNew is %lf \nFreqValOld is %lf\n\n", freqValNew, freqValOld);
				//rate of change equation from cs723 assignment 1 brief 2022
				roc = ((freqValNew - freqValOld) * 16000) / ((16000/freqValNew + 16000/freqValOld) / 2 );
				freqValOld = freqValNew;
				frequency_ROC_Data.freqData = freqValNew;
				frequency_ROC_Data.rocData = roc;
				frequency_ROC_Data.timestamp = ReceiveGenerateFre.timestamp;
				xQueueSendToFront(freqRocDataQ, &frequency_ROC_Data, 0);
				printf("Timestamp is %lf | FreData is %lf | ROC is %lf \n\n", frequency_ROC_Data.timestamp, freqValNew, roc);
			    printf("Current tick count Time is %d\n", xTaskGetTickCount());
				printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n\n");
			}
			fflush( stdout );
			vTaskDelay(100);
		}
}


void loadManagerTask(){

	double rocThresholdLocal = 0;
	double freqThresholdLocal = 0;
	int isTripCond = 0;

	struct frequency_ROC frequency_ROC_Data;


	while (1){
		switch (loadManagerState){
			case NORMAL:
				if (xQueueReceive(freqRocDataQ, &frequency_ROC_Data, 0)){
					printf("\n################Queue Received success##################\n");
					//store 50 recent frequency and roc
					updateRunningData(frequency_ROC_Data);
					printf("################Update finished##################\n");
					//update the threshold

					xSemaphoreTake(thresholdSemaphore, 0);
					rocThresholdLocal = rocThreshold;
					freqThresholdLocal = frequencyThreshold;
					//problem is here
				    xSemaphoreGive(thresholdSemaphore);
					printf("Semaphore given process is succeed!\n");

					isTripCond = checkTrippingConditions(frequency_ROC_Data, freqThresholdLocal, rocThresholdLocal);

					printf("Trip Condition = %d\n", isTripCond);

					printf("Current tick count Time is %d\n", xTaskGetTickCount());
					if (isTripCond){
						//first load shedding
						printf("Timestamp is %lf \n", frequency_ROC_Data.timestamp);
						shedLoad(SWITCHES);
					//	computeReactionTimeStatus(xTaskGetTickCount(), frequency_ROC_Data);

						//state become unstable
						wasStable = 0;
						loadManagerState = LOAD_MANAGE;
						restartFreeRTOSTimer();

						printf("\n################Load manager mode##################\n");
					}
					else{
						printf("Timestamp is %lf \n", frequency_ROC_Data.timestamp);
						printf("Nothing happened, everything is normal\n");
						printf("##################################\n\n");
					}

					if (maintainenceMode){
						loadManagerState = MAINTENANCE;
						printf("Enter into the MAINTENANCE mode.\n");
						printf("All the Green Led is OFF.\n");
					}
				}
				break;


			case LOAD_MANAGE:
				printf("\n###############case second mode##################\n");

				//Handles timer expiry if an input has not arrived yet
				if (timerExpiryFlag){
					printf("####Timer expiry before new input received#####\n");
					if (wasStable){
						if(reconnectLoad(SWITCHES) == 1){
							stopFreeRTOSTimer();
							loadManagerState = NORMAL;
							printf("\n\n######Normal Mode#########\n\n");
						}
						else{
							restartFreeRTOSTimer();
						}
					}else{
						shedLoad(SWITCHES);
						restartFreeRTOSTimer();
					}
					break;
				}



				if (xQueueReceive(freqRocDataQ, &frequency_ROC_Data, 0)){
					 	printf("\nReceive a new DATA within the 500ms!\n");
					 	printf("Timestamp is %lf \n", frequency_ROC_Data.timestamp);
						//store 50 recent frequency and roc
						updateRunningData(frequency_ROC_Data);

						//update the threshold
						xSemaphoreTake(thresholdSemaphore, 0);

						rocThresholdLocal = rocThreshold;
						freqThresholdLocal = frequencyThreshold;

						xSemaphoreGive(thresholdSemaphore);

						isTripCond = checkTrippingConditions(frequency_ROC_Data, freqThresholdLocal, rocThresholdLocal);

						//if isTripCond is true, then unstable
						if (isTripCond && wasStable){
							printf("stable => unstable");
							wasStable = 0;
							restartFreeRTOSTimer();
						}else if(isTripCond && !wasStable){

							wasStable = 0;
							if (timerExpiryFlag){
									shedLoad(SWITCHES);
									restartFreeRTOSTimer();
							}
						}else if(!isTripCond && wasStable){
							wasStable = 1;
							if (timerExpiryFlag){
								if (reconnectLoad(SWITCHES) == 1){
									stopFreeRTOSTimer();
									loadManagerState = NORMAL;
									printf("Timestamp is %lf \n", frequency_ROC_Data.timestamp);
									printf("\n\n##################Noraml Mode###########\n\n");
								}else{
									restartFreeRTOSTimer();
								}
							}
						}else if (!isTripCond && !wasStable){
							wasStable = 1;
							printf("Timestamp is %lf \n", frequency_ROC_Data.timestamp);
							printf("\nIt has become stable\n");
							restartFreeRTOSTimer();
						}

				}
				break;
				//isTripCond = false mean nothing happen , unstable (wasstable = 0)


			case MAINTENANCE:
				printf("\n################Maintenance mode##################\n");
				if ( maintainenceMode == 0 ){
					printf("maintainencemode is 0\n");
					for (int i = 0; i < 5; i++){
						SWITCHES[i] = 1;
					}
					printf("Load manager task become normal.\n");
					loadManagerState = NORMAL;
				}
				for (int i = 0; i < 5; i++){
					if (SWITCHES[i] == 0){
						loadStatus[i] = 0;
						printf("LOAD %d is manually off.\n", i);
					}else if(SWITCHES[i] == 1){
						loadStatus[i] = 1;
						printf("LOAD %d is manually on.\n", i);
					}
				}

				break;
		}
		vTaskDelay(110);
	}
}
/*
void KeyboardSimulationISRTask( ){
	char state;
	printf("######################\nPlease Enter state input\n############################\n");
	//vTaskSuspendAll();
	state = getchar();
	printf("######################\nThe input state is\n###############################\n");
	putchar(state);
	//xTaskResumeAll();
	switch(state){
		case 'a':
			printf("Test the change(threshold of frequency)\n");
			break;
		case 'b':
			printf("Test Maintenance mode!\n");
			break;
	vTaskDelay(2500);
	}
}
*/

void updateRunningData(struct frequency_ROC frequency_ROC_Data){
	double freDataLocal = frequency_ROC_Data.freqData;
	double rocDataLocal = frequency_ROC_Data.rocData;

	//add the recent 50 data
	if(runningDataIndex < 49){
		frequencyData[runningDataIndex] = freDataLocal;
		rocData[runningDataIndex] = rocDataLocal;
		runningDataIndex++;
		printf("Updating !!!\n");
	}else{
		//move all the data forward
		for(int i = 48; i >= 0; i--){
			frequencyData[i+1] = frequencyData[i];
			rocData[i+1] = rocData[i];
		}
		frequencyData[0] = freDataLocal;
		rocData[0] = rocDataLocal;
	}
}

int checkTrippingConditions(struct frequency_ROC frequency_ROC_Data,  double freqThresholdLocal, double rocThresholdLocal){
	int isTripCond;

	if (frequency_ROC_Data.freqData < freqThresholdLocal){
		isTripCond = 1;
	}
	else if (abs(frequency_ROC_Data.rocData) > rocThresholdLocal){
		isTripCond = 1;
	}else{
		isTripCond = 0;
	}
	return isTripCond;
}

void vTimer500MSCallback(TimerHandle_t timer500ms){
	timerExpiryFlag = 1;
	printf("\n\n#########################################\n");
	printf("Current Time is %d\n", xTaskGetTickCount());
	printf("################Timer Expired!###############\n\n");
}

//int loads[5] = {1, 2, 4, 8, 16};
//int loadStatus[5] = {1,1,1,1,1};
int shedLoad(int SWITCHES[]){
	int loadToShed = -1;
	for (int i = 0; i < 5; i++){
		if (SWITCHES[i] == 1 && loadStatus[i] == 1){
			loadToShed = i;
		}
	}
	//switch 1 means light is on,  0 is turn light off
	if (loadToShed != -1){
		loadStatus[loadToShed] = 0;
		printf("\nRed Led%d is %d, Green Led%d is %d\n",loadToShed, 0, loadToShed, 1);
		//red led 0 means turn off, green led 1 means turn on
		printf("\nshed load: %d\n", loadToShed);
		return 1;
	}
	else{
		printf("\nAll loads are already disconnected!\n");
		return 0;
	}

}

int reconnectLoad(int SWITCHES[]){
	int loadToReconnect = -1;
	for (int i = 4; i >= 0; i--){
		if (SWITCHES[i] == 1 && loadStatus[i] == 0){
			loadToReconnect = i;
		}
	}
	//switch 1 means light is on,  0 is turn light off
	if (loadToReconnect != -1){
		loadStatus[loadToReconnect] = 1;
		printf("\nRed Led%d is %d, Green Led%d is %d\n",loadToReconnect, 1, loadToReconnect, 0);
		//red led 0 means turn off, green led 1 means turn on
		printf("\nReconnected load: %d\n", loadToReconnect);
		if (loadStatus[0] == 1 && loadStatus[1] == 1 && loadStatus[2] == 1 && loadStatus[3] == 1 && loadStatus[4] == 1){
			return 1;
		}else{
			return 0;
		}
	}
	else{
		printf("\nAll load connected or all loads manully switched off\n");
		return -1 ;
	}

}

/*
//it has a problem about currentTime
void computeReactionTimeStatus(double currentTime, struct frequency_ROC frequency_ROC_Data){
	double reactionTimeLocal = currentTime - frequency_ROC_Data.timestamp;
	printf("reactionTime is %lf", reactionTimeLocal);
}
*/

void restartFreeRTOSTimer(){
	stopFreeRTOSTimer();

	while(xTimerStart(timer500ms, 0) == pdFAIL){
		printf("Cannot start timer\n\n");
	}
}

void stopFreeRTOSTimer(){
	if (xTimerIsTimerActive(timer500ms) == pdTRUE){
			while(xTimerStop(timer500ms, 0) == pdFAIL){
				printf("Cannot stop timer\n\n");
			}
		}
		timerExpiryFlag = 0;
}
