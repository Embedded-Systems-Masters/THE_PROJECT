/**********************************************************************/
#include <Arduino.h>

/**********************************************************************/
//this code is the design for a cooperative multitasking operating system for the ROV
// Author : Abdallah Ayman
// version : 0.1
/**********************************************************************/
//defining the tasks priorities (LOWER number means LOWER priority)
#define PRIORITY_SENSOR_DATA 1
#define PRIORITY_MOTOR_CONTROL 2
#define PRIORITY_ETHERNET_COMM 3
#define PRIORITY_USER_CMD_PROCESSING 4
#define PRIORITY_CAMERA_STREAMING 5
#define PRIORITY_COMM_WITH_SOC 6
#define PRIORITY_BATTERY_MONITORING 7
#define PRIORITY_SYSTEM_MONITORING 8
#define PRIORITY_BATTERY_PACK_MANAGEMENT 9
#define PRIORITY_RELAY_CONTROL 10



#define MAX_TASKS 10
#define TIME_SLICE 10 // Time slice duration in milliseconds

/**********************************************************************/

// Task struct to hold task information
typedef struct {
  void (*taskFunction)(); // Pointer to the task function
  unsigned long period;   // Task period in milliseconds
  unsigned long lastRun;  // Last time the task was run
  int priority;           // Task priority (higher number = higher priority)
} Task_t;

Task_t tasks[MAX_TASKS]={0};
int taskCount = 0; 

void addTask(void (*taskFunction)(), unsigned long period, int priority) {
  if (taskCount < MAX_TASKS) {
    int i;
    for (i = taskCount - 1; i >= 0 && tasks[i].priority < priority; i--) {
      tasks[i + 1] = tasks[i];
    }
    tasks[i + 1].taskFunction = taskFunction;
    tasks[i + 1].period = period;
    tasks[i + 1].lastRun = millis();
    tasks[i + 1].priority = priority;
    taskCount++;
  }
}

void scheduler()
{
  unsigned long currentMillis = millis();
  Task_t* HighestPriorityTask = NULL;

  for(int i = 0 ; i<taskCount; i++)
  {
    if( (currentMillis - tasks[i].lastRun) >= tasks[i].period)
    {
      if(HighestPriorityTask ==  NULL || tasks[i].priority > HighestPriorityTask->priority)
      {
        HighestPriorityTask = &tasks[i] ;
      }
    }
  }

  //if a high-priority task was found run it
  if(HighestPriorityTask != NULL)
  {
    unsigned long start_time = millis();
    HighestPriorityTask->taskFunction();
    HighestPriorityTask->lastRun = currentMillis;
    //but we have to ensure the task doesnt exceed the time slice
    while( (millis() - start_time) < TIME_SLICE)
    {
      //as long as it's still in its time slice span we check for other tasks to be scheduled
      Task_t* nextTask = NULL;
      for(int i = 0 ; i < taskCount ; i++)
      {
        if(currentMillis - tasks[i].lastRun > tasks[i].period)
        {
          if(nextTask == NULL || tasks[i].priority > nextTask->priority)
          {
            nextTask = &tasks[i];
          }
        }
      }

      if (nextTask && nextTask->priority > highestPriorityTask->priority) {
        break; // Exit early to allow higher-priority tasks to run
      }
    }
  }
}

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
