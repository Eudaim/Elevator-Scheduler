# Operating Systems Project 2
### Part 1 
Wrote an empty C program, empty.c. Then, created a copy of this program called part1.c to add exactly four system calls to the program. 
### Part 2
created a kernel module called my_timer that calls current_kernel_time() and stores the time value; current_kernel_time() holds the number of seconds and nanoseconds since the Epoch
### Part 3 
implemented a scheduling algorithm for a pet elevator. 
## Project Members
* Ahmed Alaoui
  * Implemented Mutex for List used in Elevator Module
  * Made Function to delete elements on top of List to handle Passengers leaving elevator
  * implemented current kernel time module
* Amanda Grant 
  * Implemented Elevator Movement Functionality
  * Defined the conditions for Elevator
  * Handled Printing to the buffer for my_timer module
  * implemented current kernel time module
* Ardonniss Zimero
  * Implemented System Calls for Elevator
  * Handled Printing to the buffer for elevator module
  * Handled Adding Passengers to a List to act as a Queue & Handled Passengers loading elevator
  * implemented current kernel time module
  * implemented threading to handle elevator activity
## File Contents

* Part 1
  * empty.c
  * part1.c
  * empty.trace
  * part1.trace 
* my_timer - part2 
  * 
  * something 
* SystemCalls - part 3
  * elevatorModule.c 
  * Makefile 
  * start_elevator.c
  * stop_elevator.c
  * issue_request.c
* Systemcall(part 3)
  * 

# How to Run
first untar by going to the location of the > .tar and in the command prompt
```
tar -xvf <tar file> 
```
Go to folder where the contents of the program is held and type in the command prompt
```
make
```
after that the the Makefile should run it scripts and a the repective module should be created for part 2 and part 3 to load the module run
```
sudo insmod modulename.ko
```
from there the module is loaded and should be running

## MakeFile Description
When you run the makefile in the repective folders it will generate the module for each part of the project

## Bugs and Unfinished Portions
There are currently no known bugs.
