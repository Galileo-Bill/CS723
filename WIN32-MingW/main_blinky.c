/* Standard includes. */
#include <stdio.h>
#include <conio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Priorities at which the tasks are created. */
#define	Frequency_VirtualISR_PRIORITY	  ( 2 )
#define Frequency_Updater_PRIORITY		  ( 1 )
#define Keyboard_Simulation_ISR_PRIORITY  ( 3 )

/* definition of task stack*/
#define TASK_STACKSIZE                   2048


/*############Init Function###################*/
void initDataStructs(void);
void initCreateTasks(void);

/*############Tasks###########################*/
void FrequencyVirtualISRTask(  );
void FrequencyUpdaterTask(  );
void KeyboardSimulationISRTask( );
//void loadMangerTask( );

/*###########Helper function##################*/
int GetVirutalSystemTime ( int VirtualSystemTimeCount );

/*###########Timer Callback##################*/
//void vTimer500MSCallback(xTimeHandle t_timer);



/*###############################################
 * ########Queue#################################
 * ##############################################
 */
xQueueHandle frequencyQ;
xQueueHandle freqRocDataQ;
/*##############################################
 * ########Gobal Var############################
 * #############################################
 */
double VirtualSystemTime = 0;
double num[100] = {0};
TaskHandle_t xHandle;
TaskHandle_t yHandle;
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




/*** SEE THE COMMENTS AT THE TOP OF THIS FILE ***/
void main_blinky( void )
{

	initDataStructs();
	printf("Data struct has initialised!\n");
	initCreateTasks();
	printf("Tasks has initialised!\n");

	/* Start the tasks. */
	vTaskStartScheduler();

	for( ;; );
}
/*-----------------------------------------------------------*/

void initDataStructs(void){
	frequencyQ = xQueueCreate( 100, sizeof(struct frequency_time  ) );
	freqRocDataQ = xQueueCreate(100, sizeof(struct frequency_ROC ) );
	return;
}


void initCreateTasks(void){
	xTaskCreate( FrequencyVirtualISRTask, "FrequencyVirtualISRTask", TASK_STACKSIZE , NULL, Frequency_VirtualISR_PRIORITY, &xHandle );
	xTaskCreate( FrequencyUpdaterTask, "FrequencyUpdaterTask", TASK_STACKSIZE , NULL, Frequency_Updater_PRIORITY, &yHandle );
	xTaskCreate( KeyboardSimulationISRTask, "KeyboardSimulationISRTask", TASK_STACKSIZE , NULL, Keyboard_Simulation_ISR_PRIORITY, NULL );
	return;
}


int GetVirtualSystemTime ( int NI ){
	VirtualSystemTime = VirtualSystemTime + NI / 16;
	return VirtualSystemTime;
}

void FrequencyVirtualISRTask(  )
{
	for (int i = 0; i < 100; i++){
		num[i] = ( rand() % (400-266 + 1) + 266 );
	}
	for (int i = 0; i < 100; i++){
		printf("num[%d] is %lf\n", i, num[i]);
	}

	while(1){
		for (int j = 0; j < 100; j++){
			freq_SIM_ISR.frequency = 16000 / (double)num[j];
 	 		freq_SIM_ISR.timestamp = GetVirtualSystemTime(num[j]);

 	 		printf("\n---------------------------------\n");
 	 		printf("frequency is %lf \n", freq_SIM_ISR.frequency);
 	 		printf("timestamp is %lf \n", freq_SIM_ISR.timestamp);
 	 		printf("\n---------------------------------\n");
 	 		xQueueSendToFront(frequencyQ, &freq_SIM_ISR, 0 );
 	 		vTaskDelay(2000);
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
		struct frequency_ROC frequency_ROC;

		while (1){
			printf("\n !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n");
			if (xQueueReceive(frequencyQ, &ReceiveGenerateFre, 0) == pdPASS){
				printf("Frequency ROC caculated. \n");
				freqValNew = ReceiveGenerateFre.frequency;

				if(isFirstIteration){
					isFirstIteration = 0;
					freqValOld = freqValNew;
					continue;
				}
				printf("FreqValNew is %lf \nFreqValOld is %lf\n ----\n", freqValNew, freqValOld);
				//rate of change equation from cs723 assignment 1 brief 2022
				roc = ((freqValNew - freqValOld) * 16000) / ((16000/freqValNew + 16000/freqValOld) / 2 );
				freqValOld = freqValNew;
				frequency_ROC.freqData = freqValNew;
				frequency_ROC.rocData = roc;
				frequency_ROC.timestamp = ReceiveGenerateFre.timestamp;
				xQueueSendToFront(freqRocDataQ, &freuency_ROC, 0);
				printf("FreData is %lf\n", freqValNew);
				printf("ROC is %lf \n", roc);
				printf("Timestamp is %lf \n", frequency_ROC.timestamp);
				printf("\n !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n");
			}
			fflush( stdout );
			vTaskDelay(200);
		}
}

void loadManagerTask(){

	//load Manager State
	#define NORMAL 0
	#define LOAD_MANAGE 1
	#define MAINTENANCE 2

	int loadManagerState = NORMAL;

	while (1){
		switch (loadMangerState){
			case NORMAL:


			case LOAD_MANAGE:


			case MAINTENANCE:
		}
	}
}

//void KeyboardSimulationISRTask( ){
//	float thre_fre = 0;
//	float thre_roc = 0;
//	int modekey;
//	char state;
//	int a = 0;
//	while(1){
//		a = printf("######################\nPlease Enter state input\n############################\n");
//		if (a !=0 ){
//			vTaskSuspendAll();
//		}
//		state = getchar();
//		printf("######################\nThe input state is\n###############################\n");
//		putchar(state);
//		xTaskResumeAll();
////		switch(state){
////			case 'a':
////				printf("Updated threshold of frequency!\n");
////				if (thre_fre != 0){
////					printf("threshold of freqency is 66.6 \n");
////					thre_fre = 0;
////					break;
////				}
////			case 'b':
////				printf("Updated threshold of ROC!\n");
////				if (thre_roc != 0){
////					printf("threshold of roc is 50.5 \n");
////					thre_roc = 0;
////					break;
////				}
////			break;
////		}
//		vTaskDelay(25);
//	}
//}
