/* Tests cetegorical mutual exclusion with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "lib/random.h" //generate random numbers
#include "../devices/timer.h"

#define BUS_CAPACITY 3
#define SENDER 0
#define RECEIVER 1
#define BOTH -1
#define NORMAL 0
#define HIGH 1

#define MAXSLEEPTIME 10 /*Waiting sleeping ticks for passing the bus*/

/*
 *  initialize task with direction and priority
 *  call o
 * */
typedef struct {
    int direction;
    int priority;
} task_t;

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive);

void senderTask(void *);
void receiverTask(void *);
void senderPriorityTask(void *);
void receiverPriorityTask(void *);
void init_bus(void);


void oneTask(task_t task);/*Task requires to use the bus and executes methods below*/
    void getSlot(task_t task); /* task tries to use slot on the bus */
    void transferData(task_t task); /* task processes data on the bus either sending or receiving based on the direction*/
    void leaveSlot(task_t task); /* task release the slot */

/*Define the semaphores, global variables*/
static struct semaphore bus_cap; /* Number of slots on the bus */
static struct semaphore send_priority;
static struct semaphore receive_priority;
static struct semaphore mutex;
static int current_direction = BOTH; 
static struct semaphore sender_task;
static struct semaphore receiver_task;
static struct semaphore normal_tasks_enter;

/*Number of NORMAL priority tasks on SENDER direction*/
unsigned int normal_tasks_send;
/*Number of NORMAL priority tasks on RECEIVER direction*/
unsigned int normal_tasks_receive;
/*Number of HIGH priority tasks on SENDER direction*/
unsigned int high_tasks_send;
/*Number of HIGH priority tasks on RECEIVER direction*/
unsigned int high_tasks_receive;
/*Number of high priority tasks on the bus*/
int high_tasks_on_bus;
/*Number of tasks currently on the bus*/
int tasks_on_bus;


/* initializes semaphores */ 
void init_bus(void){ 
  
  tasks_on_bus = 0;
  normal_tasks_send = 0;
  normal_tasks_receive = 0;
  high_tasks_send = 0;
  high_tasks_receive = 0;
  high_tasks_on_bus = 0;

  /* Generates the random number */
  random_init((unsigned int)123456789);

  sema_init (&bus_cap, BUS_CAPACITY);
  sema_init (&send_priority, NORMAL);
  sema_init(&receive_priority, NORMAL);

  /* Initialize semaphore for accessing shared data */
  sema_init(&mutex, 1);

  /* Initialize the counting semaphore */
  sema_init(&sender_task, 0);
  sema_init(&receiver_task, 0);
  sema_init(&normal_tasks_enter, 0);
}

/*
 *  Creates a memory bus sub-system  with num_tasks_send + num_priority_send
 *  sending data to the accelerator and num_task_receive + num_priority_receive tasks
 *  reading data/results from the accelerator.
 *
 *  Every task is represented by its own thread. 
 *  Task requires and gets slot on bus system (1)
 *  process data and the bus (2)
 *  Leave the bus (3).
 */

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive)
{
   unsigned int num = 0;
   /* Creating the thread for each task */
   do
   {
        thread_create("Sender_task", 1, senderTask, NULL);
        num++;
   } while(num < num_tasks_send);
   
   do
    {
        thread_create("Sender_Priority_Task", 1, senderPriorityTask, NULL);
        num++;
    } while (num < num_priority_send); 
    
    do
    {
        thread_create("Receiver_task", 1, receiverTask, NULL);
        num++;
    } while(num < num_task_receive);

    do
    {
        thread_create("Receiving_Priority_Task", 1, receiverPriorityTask, NULL);
        num++;
    } while(num < num_priority_receive);
   
}

/* Normal task,  sending data to the accelerator */
void senderTask(void *aux UNUSED){
        task_t task = {SENDER, NORMAL};
        oneTask(task);
}

/* High priority task, sending data to the accelerator */
void senderPriorityTask(void *aux UNUSED){
        task_t task = {SENDER, HIGH};
        oneTask(task);
}

/* Normal task, reading data from the accelerator */
void receiverTask(void *aux UNUSED){
        task_t task = {RECEIVER, NORMAL};
        oneTask(task);
}

/* High priority task, reading data from the accelerator */
void receiverPriorityTask(void *aux UNUSED){
        task_t task = {RECEIVER, HIGH};
        oneTask(task);
}

/* Abstract task execution*/
void oneTask(task_t task) {
  getSlot(task);
  transferData(task);
  leaveSlot(task);
}


/* Task tries to get slot on the bus subsystem */
void getSlot(task_t task) /* Consider the priority */
{ 
  int direction;
  int priority;
  int total_high_tasks = high_tasks_send + high_tasks_receive;

  /* Enter critical section */
  sema_down(&mutex);
  
  if (current_direction == BOTH){
    /* Change direction to your direction */
    current_direction = direction;
  }

  /* If the priority is either 0 or 1, increment the number 
   * of waiting tasks on either SENDER or RECEIVER direction */
  if (priority == NORMAL){
    if (direction == SENDER){
      normal_tasks_send++;
    } 
    else {
      normal_tasks_receive++;
    }
  }  
  else {
    if (direction == SENDER){
      high_tasks_send++;
    }
    else {
      high_tasks_receive++;
    }
  }

  /* If the bus is free and there are no HIGH priority tasks waiting to access the bus, 
   * the NORMAL priority tasks can access the bus */
  if (priority == NORMAL){
    if ((total_high_tasks == 0) && (high_tasks_on_bus == 0)){
      sema_up(&normal_tasks_enter);
    }

    sema_up(&mutex);

    /* Wait for high priority tasks */
    sema_down(&normal_tasks_enter);

    sema_down(&mutex);
  }

  if ((tasks_on_bus < BUS_CAPACITY) &&(direction == current_direction)){
    if (direction == SENDER){
      sema_up(&sender_task);
    }
    else {
      sema_up(&receiver_task);
    }
  }

  sema_up(&mutex);

  /* Trying to access the bus */
  if (direction == SENDER){
    sema_down(&sender_task);
  }
  else {
    sema_down(&receiver_task);
  }

  /* Decrement the free slots on the bus */
  sema_down(&bus_cap);

  /* Enterng the critical section */
  sema_down(&mutex);

  /* Increment the free slots on the bus */
  tasks_on_bus++;
  if (priority == NORMAL){
    if (direction == SENDER){
      normal_tasks_send--;
    }
    else {
      normal_tasks_receive--;
    }
  }
  else {
    high_tasks_on_bus++;
    if (direction == SENDER){
      high_tasks_send--;
    }
    else {
      high_tasks_receive--;
    }
  }
  sema_up(&mutex);
}


/* Task processes data on the bus send/receive */
void transferData(task_t task) /**/
{
  sema_down(&mutex);
  ASSERT(tasks_on_bus <= BUS_CAPACITY);
  sema_up(&mutex);
  /* Converting the random numbers to very long integers */
  unsigned int random = (unsigned int)random_ulong();  
  random = random % MAXSLEEPTIME;
  /* Sleeping the thread from 0 to MAXSLEEPTIME ticks */
  timer_sleep((int64_t)random);
}


/* Task releases the slot */
void leaveSlot(task_t task) 
{
  int priority;
  int total_high_tasks = high_tasks_send + high_tasks_receive;
  sema_down(&mutex);
  tasks_on_bus--;

  /* Increment the free slots on the bus */
  sema_up(&bus_cap);

  if (priority == HIGH){

    /* When there are HIGH priority tasks waiting to access the bus, 
     * the NORMAL priority tasks have to be blocked until the last 
     * HIGH priority task unblock them */

    high_tasks_on_bus--;
    int blocked_tasks = normal_tasks_send + normal_tasks_receive;
    while ((total_high_tasks == 0) && (blocked_tasks > 0)){
      blocked_tasks--;
      sema_up(&normal_tasks_enter);
    }
  }

  /* When the bus is free and there are HIGH priority tasks waiting to access the bus */
  if (tasks_on_bus == 0){
    if (total_high_tasks > 0){
      /* Selecting the direction for HIGH priority tasks */
      if (high_tasks_receive > high_tasks_send){
        current_direction = RECEIVER;
      }
      else {
        current_direction = SENDER;
      }
    }
    else {
      /* Selecting the direction for NORMAL priority tasks */
      if (normal_tasks_receive > normal_tasks_send){
        current_direction = RECEIVER;
      }
      else{
        current_direction = SENDER;
      }
    }
  }

  /* Allow the new task to use the bus */
  int new_direction = current_direction;
  sema_up(&mutex);
  /* Selecting direction for the new tasks */
  if (new_direction == SENDER){
    sema_up(&sender_task);
  }
  else {
    sema_up(&receiver_task);
  }
}