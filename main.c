/*
 * main.c
 *
 *  Created on: 2017 Feb 28 23:59:24
 *  Author: Ciara Power
 */




#include <DAVE.h>                 //Declarations from DAVE Code Generation (includes SFR declaration)

int32_t button1=0;     // buttons for GUI
int32_t button2=0;
int32_t button3=0;
int32_t button4=0;
int32_t reset=0;



int32_t count;      //variables for GUI
int32_t locked=0;   // either 1 for locked, or 0 for unlocked, used for GUI bitmap of lock
int32_t resetValue=0;    // either 1 for in reset mode or 0 for not in reset mode, used in GUI
int32_t errorCount=0;    //how many errors have occurred
int32_t errorFlashCount=0;   //how many flashes have occurred of the error LED( ON/OFF is 2 "flashes")
int32_t thirtySecCount=0;  // how many 30 sec intervals have passed on minutes_timer


int saved_combo[4]={1,2,3,4};   //the combination saved on lock
int entered_combo[4];   //user entry

int nsTable[10][20]={{-1,2,-1,4,-1,-1,-1,9,-1,-1,-1,-1,6,5,-1},     //table for events and next states
										 {-1,2,-1,4,-1,-1,-1,9,-1,-1,-1,-1,6,5,-1},
										 {-1,-1,6,3,-1,-1,-1,-1,-1,-1,-1,-1,-1,5,-1},
										 {-1,-1,-1,-1,-1,-1,2,-1,-1,-1,-1,-1,-1,5,-1},
										 {-1,-1,-1,-1,0,-1,-1,-1,-1,-1,-1,-1,-1,5,1},
										 {0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,1,-1,-1,-1},
										 {0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,1,-1,5,-1},
										 {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,5,-1,-1,-1,-1},
										 {0,-1,-1,-1,-1,-1,-1,-1,-1,7,-1,1,-1,-1,-1},
										 {-1,-1,-1,-1,-1,5,-1,-1,8,-1,-1,-1,-1,5,-1}};
int currentState=0;
int event;
int entered=0;   // variable to indicate if user entry button value was entered into entered_combo array
int saved=0; // variable to indicate if user entry button value was entered into saved_combo array
int same=0;   // entered_combo and saved_combo comparison variable
int checked=0;   //if entered_combo was checked against saved_combo
int tenSecTimer=0;   // if tenSecTimer has completed this equals 1
											
				
/**

 * @brief main() - Application entry point
 *
 * <b>Details of function</b><br>
 * This routine is the application entry point. It is invoked by the device startup code. It is responsible for
 * invoking the APP initialization dispatcher routine - DAVE_Init() and hosting the place-holder for user application
 * code.
 */

int getEvent(){    //method to get the event number occured
	if (thirtySecCount<10 && errorFlashCount==8) return NULL;  // while in state of user entry lock stay in same state
  else if(thirtySecCount==10) return 10;   // 5 minutes has passed
	else if(errorCount==4 && thirtySecCount==0) return 9;   // 4 errors and 5 min timer hasnt started yet
	else if((thirtySecCount==2) && (errorCount!=4) ) return 13;   // 1 minute has passed and less than 4 errors
	else if(tenSecTimer==1) return 12;   // if 10 secs have passed
	else if(DIGITAL_IO_GetInput(&BUTTON_1)==1 || DIGITAL_IO_GetInput(&BUTTON_2)==1 || DIGITAL_IO_GetInput(&BUTTON_3)==1 || DIGITAL_IO_GetInput(&BUTTON_4)==1) return 3;  //button pressed
	else if(same==4) return 5;  //same combinations
	else if(same!=4 && checked==1) return 8;  //not same combinations after they've been checked
	else if(saved==1) return 6;   // the user entry button value was put into saved_combo array
  else if(DIGITAL_IO_GetInput(&BUTTON_RESET)==1) { COUNTER_ResetCounter(&PUSHBUTTON_COUNT); return 1;}  // reset button was pressed
	else if((COUNTER_GetCurrentCount(&PUSHBUTTON_COUNT)==4) && (resetValue==1)) return 2;   // 4 buttons were entered while in reset mode
	else if(entered==1 && DIGITAL_IO_GetInput(&LOCK_LED)==0 ) return 4;  // user entry button value was entered into entered_combo array and unlocked
	else if(entered==1 && DIGITAL_IO_GetInput(&LOCK_LED)==1) return 14;  // user entry button value was entered into entered_combo array and locked
	else if(COUNTER_GetCurrentCount(&PUSHBUTTON_COUNT)==4) return 7;    // if 4 buttons were entered
	else if(COUNTER_GetCurrentCount(&PUSHBUTTON_COUNT)==0 && DIGITAL_IO_GetInput(&LOCK_LED)==0) return 0;   // if no buttons entered and unlocked
	else if(COUNTER_GetCurrentCount(&PUSHBUTTON_COUNT)==0 && DIGITAL_IO_GetInput(&LOCK_LED)==1) return 11;   // if no buttons entered and locked
	else return NULL;    // if none of the above events occurred
}

int getNextState(int currentState,int nsTable[10][20],int event){   //gets next state with the given state and event that has occurred
	if (nsTable[currentState][event] != -1)  return nsTable[currentState][event];   // state can transition to a next state with the current event
	else return currentState;   // current state doesnt have entry for current event so stay in current state 
}

void combosEqual()   //test if enetered_combo == saved_combo
	{
		same=0;
		for(int i=0;i<4;i++){
			if(saved_combo[i]==entered_combo[i]){
				same++;   //if combos are the same this variable will end up to be 4
			}
		}
		checked=1;  //combo was checked
	}
	
void openIdleMode(){      
	DIGITAL_IO_SetOutputLow(&LOCK_LED);   //unlocked
	resetValue=0;  // resetMode is off
	locked=0;  //unlocked
	entered=0;  //item not just entered into combo
	same=0;   // combos similarity reset to 0
}

void lockedIdleMode(){
  DIGITAL_IO_SetOutputHigh(&LOCK_LED); //locked
  locked=1;  //locked
	entered=0;  // item not just entered into combo
	resetValue=0; // resetMode is off
	same=0;   // combos similarity reset to 0
}

void resetMode(){
	resetValue=1;   //in reset mode
	saved=0;   // item was not just added into saved_combo
}

void savedAddMode(){
	DIGITAL_IO_SetOutputHigh(&PUSHBUTTON_PRESSED);     // digital io that will increment counter
	count=COUNTER_GetCurrentCount(&PUSHBUTTON_COUNT);   // to display count on gui
	if(DIGITAL_IO_GetInput(&BUTTON_1)==1){
		  saved_combo[count]=1;
	  }
		else if(DIGITAL_IO_GetInput(&BUTTON_2)==1){
			saved_combo[count]=2;
		}	
		else if(DIGITAL_IO_GetInput(&BUTTON_3)==1){
			saved_combo[count]=3;
		}
		else if(DIGITAL_IO_GetInput(&BUTTON_4)==1){
			saved_combo[count]=4;
		}
		saved=1;  //item just added to saved_combo
}

void enteredAddMode(){
	DIGITAL_IO_SetOutputHigh(&PUSHBUTTON_PRESSED);  // digital io that will increment counter
	count=COUNTER_GetCurrentCount(&PUSHBUTTON_COUNT);  // to display count on gui
	if(count==0){   // if its the first button pressed, start the 10 second timer for user input
			TIMER_SetTimeInterval(&SECONDS_TIMER,1000000000);
			TIMER_Start(&SECONDS_TIMER);
			}
		if(DIGITAL_IO_GetInput(&BUTTON_1)==1){
		  entered_combo[count]=1;
	  }
		else if(DIGITAL_IO_GetInput(&BUTTON_2)==1){
			entered_combo[count]=2;
		}	
		else if(DIGITAL_IO_GetInput(&BUTTON_3)==1){
			entered_combo[count]=3;
		}
		else if(DIGITAL_IO_GetInput(&BUTTON_4)==1){
			entered_combo[count]=4;
		}
		entered=1;   // item just added to entered_combo
}

void resetEntriesMode(){
	COUNTER_ResetCounter(&PUSHBUTTON_COUNT);
	count=COUNTER_GetCurrentCount(&PUSHBUTTON_COUNT);
	tenSecTimer=0;   // reset the ten second timer variable indicating it has/hasnt completed
	if(same==4) // if combos were just checked and found the same
	{   
		checked=0;   // reset to unchecked
	}
}

void resetErrorMode(){
	if (same==4){     // if combos were checked and are the same
		resetEntriesMode();     //reset entries
	}
		same=0;
		errorCount=0;  
	  errorFlashCount=0; 
		thirtySecCount=0;   // the 1 minute timer is reset , so this variable counting 30 sec intervals is reset too
	

	
}



void entryLockMode(){
	while(errorFlashCount<8);    // this is <8 when the errors entered has not reached 4 yet
	if (thirtySecCount==0){   // if no 30 secs have passed in minutes_timer
		TIMER_SetTimeInterval(&MINUTES_TIMER,3000000000);    //start the timer for 30 secs
		TIMER_Start(&MINUTES_TIMER);
	}	
}

void errorMode(){   
	  checked=0;      
	  errorCount++;
    DIGITAL_IO_SetOutputHigh(&ERROR_LED);   // turn on error LED
		TIMER_SetTimeInterval(&SECONDS_TIMER,100000000); //start 1 sec timer
		TIMER_Start(&SECONDS_TIMER);
		if(errorCount==1){  // if first error, start the minute timer at 30 secs
		TIMER_SetTimeInterval(&MINUTES_TIMER,3000000000);
		TIMER_Start(&MINUTES_TIMER);
	  }
}

void checkComboMode(){
	TIMER_Stop(&SECONDS_TIMER);    // stop the 10 sec user input timer
	TIMER_Clear(&SECONDS_TIMER);
	TIMER_ClearEvent(&SECONDS_TIMER);
  combosEqual();  //check if equal
	resetEntriesMode();  //reset entries
	if(same==4){  //if same
		DIGITAL_IO_ToggleOutput(&LOCK_LED); 
	}
}


	void debounceTimer(void){   
		TIMER_Start(&DEBOUNCE_TIMER);
		while(!TIMER_GetInterruptStatus(&DEBOUNCE_TIMER));   //stays here while timer is active
		TIMER_ClearEvent(&DEBOUNCE_TIMER);
		TIMER_Clear(&DEBOUNCE_TIMER);		
	}
	
	
void Minutes_IRQHandler(void){
	  if(thirtySecCount>=10 || (thirtySecCount==2 && errorFlashCount!=8)){  // if less than 5 mins have passed 
		TIMER_Stop(&MINUTES_TIMER);      
    TIMER_Clear(&MINUTES_TIMER);
		TIMER_ClearEvent(&MINUTES_TIMER);                                                     // OR  1 minute has passed AND total error flashes havent occured
		}
		else{
			thirtySecCount++;
			if(thirtySecCount==2 && errorFlashCount!=8){
				TIMER_Stop(&MINUTES_TIMER);      
    TIMER_Clear(&MINUTES_TIMER);
		TIMER_ClearEvent(&MINUTES_TIMER);
		}
}
		}


	void Seconds_IRQHandler(void){
		if(DIGITAL_IO_GetInput(&ERROR_LED)==1 && errorCount<4){   // error light is on and not all erros have occured
		TIMER_Stop(&SECONDS_TIMER);
    TIMER_Clear(&SECONDS_TIMER);
		TIMER_ClearEvent(&SECONDS_TIMER);
		errorFlashCount++;
		DIGITAL_IO_ToggleOutput(&ERROR_LED);  
		}
		else if(errorCount==4){   //all errors have occured
			DIGITAL_IO_ToggleOutput(&ERROR_LED);    
			errorFlashCount++;
		if (errorFlashCount==8){   // all error flashes have occured
		TIMER_Stop(&SECONDS_TIMER);
    TIMER_Clear(&SECONDS_TIMER);
		TIMER_ClearEvent(&SECONDS_TIMER);
			}
		}
		else{    // 10 secs have passed ( nothing to do with error timing) so timer is stopped
		TIMER_Stop(&SECONDS_TIMER);
    TIMER_Clear(&SECONDS_TIMER);
		TIMER_ClearEvent(&SECONDS_TIMER);
		tenSecTimer=1;   // variable for 10 secs set to 1 
		}
	}
	

int main(void)
{
  DAVE_STATUS_t status;

  status = DAVE_Init();           /* Initialization of DAVE APPs  */

  if(status != DAVE_STATUS_SUCCESS)
  {
    /* Placeholder for error handler code. The while loop below can be replaced with an user error handler. */
    XMC_DEBUG("DAVE APPs initialization failed\n");

    while(1U)
    {

    }
  }

  /* Placeholder for user application code. The while loop below can be replaced with user application code. */
	
  
  while(1U)
  {  //check each gui button and set pin high for each if pressed, and debounce
		if(button1==1){
			DIGITAL_IO_SetOutputHigh(&BUTTON_1);
			debounceTimer();
		}
		else if(button2==1){
			DIGITAL_IO_SetOutputHigh(&BUTTON_2);
			debounceTimer();
		}
		else if (button3==1 ){
			DIGITAL_IO_SetOutputHigh(&BUTTON_3);
			debounceTimer();
	}
		else if(button4==1){
      DIGITAL_IO_SetOutputHigh(&BUTTON_4);
			debounceTimer();
		}
   else if(reset==1){
		 DIGITAL_IO_SetOutputHigh(&BUTTON_RESET);
		 debounceTimer();
	 }
	 else{   //if no button on gui is being pressed, make sure all pins are low and update count on gui
		 DIGITAL_IO_SetOutputLow(&BUTTON_RESET);
		 DIGITAL_IO_SetOutputLow(&BUTTON_1);
		 DIGITAL_IO_SetOutputLow(&BUTTON_2);
		 DIGITAL_IO_SetOutputLow(&BUTTON_3);
		 DIGITAL_IO_SetOutputLow(&BUTTON_4);
		 DIGITAL_IO_SetOutputLow(&PUSHBUTTON_PRESSED);
		 count=COUNTER_GetCurrentCount(&PUSHBUTTON_COUNT);
	 }
	 

	 event=getEvent();  
	 currentState=getNextState(currentState,nsTable,event);
	 
	 // jump to the next current state 
	 if(currentState==0) openIdleMode();
	 else if(currentState==1) lockedIdleMode();
	 else if(currentState==2) resetMode();
	 else if(currentState==3) savedAddMode();
	 else if(currentState==4) enteredAddMode();
	 else if(currentState==5) resetErrorMode();
	 else if(currentState==6) resetEntriesMode();
	 else if(currentState==7) entryLockMode();
	 else if(currentState==8) errorMode();
	 else if(currentState==9) checkComboMode();
	 
}
	}
