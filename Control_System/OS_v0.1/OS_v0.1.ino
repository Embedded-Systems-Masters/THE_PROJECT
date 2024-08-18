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
typedef enum { READY, RUNNING, WAITING, SUSPENDED, FINISHED } TaskState;

typedef struct {
  void (*taskFunction)();    // Pointer to the task function
  unsigned long period;      // Task period in milliseconds (0 for event-driven tasks)
  unsigned long lastRun;     // Last time the task was run
  unsigned long delayUntil;  // Time until the task should resume
  unsigned long elapsedTime; // Time the task has been running
  int priority;              // Task priority (higher number = higher priority)
  TaskState state;           // Current state of the task
} Task_t;


typedef unsigned int sens_t;  
Task_t tasks[MAX_TASKS]={0};
int taskCount = 0; 
int currentTaskIndex = -1;


void addTask(void (*taskFunction)(), unsigned long period, int priority) {
  if (taskCount < MAX_TASKS) {
    int i;
    for (i = taskCount - 1; i >= 0 && tasks[i].priority < priority; i--) {
      tasks[i + 1] = tasks[i];
    }
    tasks[i + 1].taskFunction = taskFunction;
    tasks[i + 1].period = period;
    tasks[i + 1].lastRun = millis();
    tasks[i + 1].delayUntil = 0;
    tasks[i + 1].elapsedTime = 0;
    tasks[i + 1].priority = priority;
    tasks[i + 1].state = READY;
    taskCount++;
  }
}

void delayTask(unsigned long delayMillis) {
  if (currentTaskIndex != -1) {
    tasks[currentTaskIndex].delayUntil = millis() + delayMillis;
    tasks[currentTaskIndex].state = SUSPENDED;
  }

} 
void switchTask() {
  // Save the state of the current task
  if (currentTaskIndex != -1 && tasks[currentTaskIndex].state == RUNNING) {
    tasks[currentTaskIndex].state = WAITING;
  }

  // Find the next READY or SUSPENDED task that should be resumed
  int nextTaskIndex = -1;
  unsigned long currentTime = millis();
  for (int i = 0; i < taskCount; i++) {
    if (tasks[i].state == READY || (tasks[i].state == SUSPENDED && currentTime >= tasks[i].delayUntil)) {
      nextTaskIndex = i;
      tasks[i].state = READY;
      break;
    }
  }

  // Switch to the next task
  if (nextTaskIndex != -1) {
    currentTaskIndex = nextTaskIndex;
    tasks[currentTaskIndex].state = RUNNING;
  } else {
    currentTaskIndex = -1; // No READY or resumable task found
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

      if (nextTask && nextTask->priority > HighestPriorityTask->priority)
      {
        break; // Exit early to allow higher-priority tasks to run
      }
    }
  }
}

void setup() {
  // Add tasks
  addTask(Red_LED, 1000, 1);               // Periodic task 1
  addTask(Blue_LED, 500, 2);                // Periodic task 2
  addTask(US_SENS, 250, 10);
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  scheduler();
}

void task1() {
  // Code for Task 1
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
  digitalWrite(12,LOW);


}

void task2() {

  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  digitalWrite(13, LOW);
  // Code for Task 2
}
void US_SENS()
{

}