
JOB SCHEDULING AND SYNCHRONIZATION in a modern operating systems using Pintos (an educational operating system supporting kernel threads, loading and running of user programs and a file system). This was a lab project in the course Operating Systems, EDA092 at Chalmers University of Technology, Gothenburg,Sweden.

This assignment was divided into 2 tasks:

The first task focused on the enhancement of synchronization implementations. One of the classic synchronization methods for a thread is busy-waiting, i.e. spinning in an endless loop until some information from another thread/process stops it. An obvious drawback, is that CPU cycles are wasted without any useful work being done. In this task, we dived deeper in the synchronization implementation and tried to provide an alternative implementation of a sleep function.
In the second task, we dealt with the synchronization problems that occur when scheduling jobs send and receive data through a common bus for an external hardware accelerator (e.g. GPU, co-processor).



Implementing the Solution

The solution to the first task was implemented in the file pintos/src/devices/timer.c , reimplement timer_sleep().  Necessary changes were made in the thread control structure which aids in the solution. An understanding of thread states and interrupt handling was very crucial for this task. 

The second task was in the file pintos/src/devices/batch-scheduler.c which contained function prototypes that needed to be implemented.  They include: 

init_bus() : to initialize necesarry synchronization primitives for our solution

batchScheduler(): for creating threads using functions from the threading library to represent the different entities in the solution, that is bus users. Every task is represented by its own thread.

getSlot(): called each time a thread is ready to use the bus subsystem

transferData(): called to simulate the delay the thread experiences while using the bus. Used timer_sleep() to block the thread for a random duration

leaveSlot(): called as the thread leaves the bus, should free up resources used by the thread. 
Please do not change the prototype names or structure as these are expected by the test scripts. 

Configuring and Building

Since we were working primarily in the threads directory for this assignment, with some work in the devices directory on the side, compilation was done in the threads directory and this created  a 'build' folder which would be used for tests as described below.

Testing

Several tests were created to test both parts of the assignment and these are alarm-single, alarm-multiple, alarm-simultaneous, alarm-zero, alarm-negative, batch-scheduler. The tests with alarm-* were meant to test the timer  timer_sleep()  implementation and the other test was for testing the batch-scheduler implementation. 

To execute a particular test, cd into the newly created build directory after compilation. Then issue the command pintos run <test name>, such as  pintos run alarm-multiple, which executes the alarm-multiple test.

To completely test the entire solution, invoke make check from the project `build' directory. This builds and runs each test and prints a "pass" or "fail" message for each one. When a test fails, make check also prints some details of the reason for failure. After running all the tests, make check also prints a summary of the test results.