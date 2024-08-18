  /**********************************************************************/
  #include <Arduino.h>

  /**********************************************************************/
  //this code is the design for a cooperative multitasking operating system for the ROV
  // Author : Abdallah Ayman
  // version : 0.3
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
bool taskFinished[MAX_TASKS]; // Array to track if a task is finished
  /**********************************************************************/
  //                         LOGGING START                              // 
  /**********************************************************************/


  void logToSerial(const char* message) {
    Serial.println(message);
  }


  /**********************************************************************/
  //                         LOGGING END                                // 
  /**********************************************************************/




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
      taskFinished[i + 1] = false; // Initialize task finished flag
      logToSerial("Task Created");
      taskCount++;
    }
  }

  void delayTask(unsigned long delayMillis) {
    if (currentTaskIndex != -1) {
      tasks[currentTaskIndex].delayUntil = millis() + delayMillis;
      logToSerial("Task Suspended");
      tasks[currentTaskIndex].state = SUSPENDED;
      scheduler();
    }

  } 

  void markTaskAsFinished() {
  if (currentTaskIndex != -1) {
    taskFinished[currentTaskIndex] = true;
    tasks[currentTaskIndex].state = FINISHED;
  }
}

void resetTask(int index) {
  // Only reset the task if its period has elapsed
  unsigned long currentTime = millis();
  if (currentTime - tasks[index].lastRun >= tasks[index].period) {
    tasks[index].state = READY;
    tasks[index].delayUntil = 0; // Reset delay
    taskFinished[index] = false; // Clear finished flag
    logToSerial("Task RESET");
  }
}

void switchTask() {
  // Save the state of the current task if it's running
  if (currentTaskIndex != -1 && tasks[currentTaskIndex].state == RUNNING) {
    tasks[currentTaskIndex].state = READY;
    logToSerial("Task stopped");
  }
  if (tasks[currentTaskIndex].state == SUSPENDED) {
      logToSerial("kkkkkkkkkkk");
  }
  // Find the next task to run
  int nextTaskIndex = -1;
  unsigned long currentTime = millis();
  for (int i = 0; i < taskCount; i++) {
    // A task is eligible if it's READY or SUSPENDED and its delay has passed
    if (tasks[i].state == READY || (tasks[i].state == SUSPENDED && currentTime >= tasks[i].delayUntil)) {
      if (nextTaskIndex == -1 || tasks[i].priority < tasks[nextTaskIndex].priority) {
        nextTaskIndex = i;
        logToSerial("A Task Became Ready");
      }
    }
  }

  // Switch to the next task
  if (nextTaskIndex != -1) {
    currentTaskIndex = nextTaskIndex;
    tasks[currentTaskIndex].state = READY;
    logToSerial("Task Successful");
  } else {
    currentTaskIndex = -1; // No task to run
    logToSerial("No Task to Run");
  }
}



void scheduler() {
  unsigned long currentMillis = millis();
  Task_t* highestPriorityTask = NULL;

  // Find the highest priority task that is ready to run or can be resumed
  for (int i = 0; i < taskCount; i++) {
    // Check if the task is READY
    if (tasks[i].state == READY) {
      if ((currentMillis - tasks[i].lastRun) >= tasks[i].period) {
        if (highestPriorityTask == NULL || tasks[i].priority < highestPriorityTask->priority) {
          highestPriorityTask = &tasks[i];
          logToSerial("HIGHEST Task Became Ready");
        }
      }
    }
    // Check if the task is SUSPENDED but its delay has expired
    else if (tasks[i].state == SUSPENDED && currentMillis >= tasks[i].delayUntil) {
      if (highestPriorityTask == NULL || tasks[i].priority < highestPriorityTask->priority) {
        highestPriorityTask = &tasks[i];
        logToSerial("HIGHEST Task Became Ready");
      }
    }
  }

  // Time slicing logic
  if (highestPriorityTask != NULL) {
    unsigned long startTime = millis();
    // Save the state of the current task
    if (currentTaskIndex != -1 && tasks[currentTaskIndex].state == RUNNING) {
      tasks[currentTaskIndex].state = WAITING;
      logToSerial("Task stopped");
    }
    currentTaskIndex = highestPriorityTask - tasks;
    tasks[currentTaskIndex].state = RUNNING;
    logToSerial("Current Task started");

    while (millis() - startTime < TIME_SLICE) {
      // Execute the current highest priority task
       tasks[currentTaskIndex].taskFunction();
      logToSerial("Task executed");

      // Check if the task is finished
      if (taskFinished[currentTaskIndex]) {
        logToSerial("A Task Finished");
        break;
      }

      // Check for preemption
      Task_t* nextTask = NULL;
      for (int i = 0; i < taskCount; i++) {
        if ((currentMillis - tasks[i].lastRun) >= tasks[i].period) {
          if (nextTask == NULL || tasks[i].priority < nextTask->priority) {
            nextTask = &tasks[i];
            logToSerial("Next Task Found");
          }
        }
      }

      if (nextTask && nextTask->priority < highestPriorityTask->priority) {
        logToSerial("Task preempted");
        break; // Exit early to allow higher-priority tasks to run
      }
    }

    // Save the task's elapsed time
    highestPriorityTask->elapsedTime += millis() - startTime;

    // If the task is not finished, return it to the READY state
    if (!taskFinished[currentTaskIndex]) {
      highestPriorityTask->state = READY;
    }
    
  } else {
    logToSerial("No Task to Run");
  }

  // Switch to the next task
  switchTask();

  // Reset finished tasks
  for (int i = 0; i < taskCount; i++) {
    if (taskFinished[i]) {
      resetTask(i);
    }
  }
}
  void setup() {
    // Add tasks
    Serial.begin(115200); // Start serial communication at a baud rate of 9600
    // Add tasks
  addTask(SensorData, 250, PRIORITY_SENSOR_DATA);       // Periodic sensor data acquisition
  addTask(MotorControl, 500, PRIORITY_MOTOR_CONTROL);    // Periodic motor control
  addTask(EthernetComm, 1000, PRIORITY_ETHERNET_COMM);   // Periodic Ethernet communication
  addTask(UserCommandProcessing, 1500, PRIORITY_USER_CMD_PROCESSING); // Periodic user command processing
  addTask(CameraStreaming, 2500, PRIORITY_CAMERA_STREAMING); // Periodic camera streaming
  addTask(BatteryMonitoring, 3000, PRIORITY_BATTERY_MONITORING); // Periodic battery monitoring

  }

  void loop() {
    // put your main code here, to run repeatedly:
    scheduler();
  }

// Example task functions
void SensorData() {
  // Simulate sensor data acquisition
  logToSerial("Acquiring sensor data...");
  delayTask(40); // Simulate work and then suspend
  markTaskAsFinished(); // Mark task as finished
}

void MotorControl() {
  // Simulate motor control
  logToSerial("Controlling motors...");
  delayTask(50); // Simulate work and then suspend
  markTaskAsFinished(); // Mark task as finished
}

void EthernetComm() {
  // Simulate Ethernet communication
  logToSerial("Communicating over Ethernet...");
  delayTask(70); // Simulate work and then suspend
  markTaskAsFinished(); // Mark task as finished
}

void UserCommandProcessing() {
  // Simulate processing user commands
  logToSerial("Processing user commands...");
  delayTask(80); // Simulate work and then suspend
  markTaskAsFinished(); // Mark task as finished
}

void CameraStreaming() {
  // Simulate camera streaming
  logToSerial("Streaming camera data...");
  delayTask(100); // Simulate work and then suspend
  markTaskAsFinished(); // Mark task as finished
}

void BatteryMonitoring() {
  // Simulate battery monitoring
  logToSerial("Monitoring battery...");
  delayTask(150); // Simulate work and then suspend
  markTaskAsFinished(); // Mark task as finished
}