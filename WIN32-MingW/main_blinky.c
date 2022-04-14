/* Standard includes. */
#include <stdio.h>
#include <conio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Priorities at which the tasks are created. */
#define	Frequency_VirtualISR_PRIORITY	  ( 3 )
#define Frequency_Updater_PRIORITY		  ( 1 )
#define Keyboard_Simulation_ISR_PRIORITY  ( 2 )

/* definition of task stack*/
#define TASK_STACKSIZE                   2048


/*############Init Function###################*/
void initDataStructs(void);
void initCreateTasks(void);

/*############Tasks###########################*/
void FrequencyVirtualISRTask(  );
void FrequencyUpdaterTask(  );
void KeyboardSimulationISRTask( );
void loadMangerTask( );

/*###########Helper function##################*/
int GetVirutalSystemTime ( int VirtualSystemTimeCount );

/*###########Timer Callback##################*/
//void vTimer500MSCallback(xTimeHandle t_timer);



/*###############################################
 * ########Queue#################################
 * ##############################################
 */
xQueueHandle frequencyQ;

/*##############################################
 * ########Gobal Var############################
 * #############################################
 */
double VirtualSystemTime = 0;
double num[100] = {0};

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
	return;
}


void initCreateTasks(void){
	xTaskCreate( FrequencyVirtualISRTask, "FrequencyVirtualISRTask", TASK_STACKSIZE , NULL, Frequency_VirtualISR_PRIORITY, NULL );
	xTaskCreate( FrequencyUpdaterTask, "FrequencyUpdaterTask", TASK_STACKSIZE , NULL, Frequency_Updater_PRIORITY, NULL );
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
				printf("FreData is %lf\n", freqValNew);
				printf("ROC is %lf \n", roc);
				printf("Timestamp is %lf \n", frequency_ROC.timestamp);
				printf("\n !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n");
			}
			fflush( stdout );
			vTaskDelay(200);
		}
}
/*-----------------------------------------------------------
static void KeyboardSimulationISRTask( ){
	float thre_fre = 0;
	float thre_roc = 0;
	int modekey;
	int state;
	while (1){
		vTaskDelay(200);
		printf("Please enter keyboard manager state, 1 for udpate thre_fre, 2 for  update thre_roc, 3 for update mode\n");
		scanf("%d", &state);
		switch(state){
			case 1:
				printf("Please enter updated threshold of frequency!\n");
				scanf("%f", &thre_fre);
				if (thre_fre != 0){
					printf("threshold of freqency is %f\n", thre_fre);
					thre_fre = 0;
					break;
				}
			case 2:
				printf("Please enter updated threshold of ROC!\n");
				scanf("%f\n", &thre_roc);
				if (thre_roc != 0){
					printf("threshold of roc is %f \n",  thre_roc);
					thre_roc = 0;
					break;
				}
			break;
		}
		vTaskDelay(200);
	}
}
*/
