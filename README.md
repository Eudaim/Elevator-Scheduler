# Operating Systems Project 2
### Part 1 
Develop a program that call 3 system calls
### Part 2
Develop a program that kept keep track of time
### Part 3 
Develop a elevator
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
  * something
  * something 
* my_timer - part2 
  * something
  * something 
* SystemCalls - part 3
  * elevatorModule.c 
  * Makefile 
  * start_elevator.c
  * stop_elevator.c
  * issueu_request.c
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
after that the the Makefile should run it scripts and a shell file should pop up. From there run
```
./shell
```
## MakeFile Description
When you run the makefile in the repective folders it will generate the module for each part of the project

## Bugs and Unfinished Portions
There are currently no known bugs.
