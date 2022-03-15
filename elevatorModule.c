#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/linkage.h>
#include <linux/string.h> 
#include <linux/list.h>
#include <linux/kthread.h> 
#include <linux/delay.h>

MODULE_LICENSE("Dual BSD/GPL");
#define BUF_LEN 100
#define FLOORS 10
//---Floor State---
#define ELEVATOR_IS_PRESENT
//------ELEVATOR STATE-----
#define OFFLINE 0 
#define IDLE 1 
#define LOADING 2
#define UP 3
#define DOWN 4
#define STOPPING 5
//------PASSENGERS-----
#define CAT 0
#define DOG 1
#define LIZARD 2 
//-----Elevator Conditions---- 
#define MAX_PASSENGERS 10
#define MAX_WEIGHT 100



static struct proc_dir_entry* proc_entry;

static char msg[BUF_LEN];
static char *message; 
static char *stringTemp0; 
static char *stringTemp1; 
static char *stringTemp2; 
static char *stringTemp3; 
static char *stringTemp4;
int rp;
struct task_struct* elevator_thread;
static int procfs_buf_len;

//int numOccupants; 
int currentOccupants;
int waitingPassengers; 
int passengerServiced; 
int passengerServicedEachFloor[FLOORS];

int currentFloor; 
int currentWeight; 
int currentState; 
int nextState;
int occupantType;
int passengerEachFloor[FLOORS];
int desFloors[100];
struct list_head listOfPassengers[FLOORS];      //head of list
struct list_head elevatorList;
struct mutex mutexForPassengersQueue; 
struct mutex mutexForElevatorList;
struct passenger 
{
    int startFloor; 
    int endFloor; 
    int type;
    int weight; 
    struct list_head next;
} Passenger;

//Function declarations
int addPassengersToQueue(int type, int floor, int endFloor);
int getWeight(int type);
int addPassengersToElevator(void);
void loadFloors(void);
int getElevatorSize(void);
char * getPassengersOnFloor(int floor);
char * getPassengerType(int type);
char * getPassengerList(void);
char * getPassengersOnElevator(void);
//void delete(struct list_head *start);
//void delete_front_list(struct list_head **front);
void print_current_elevator(void);
// int removeItem(struct list_head * queue, struct passenger * remove_passenger);

extern long (*STUB_start_elevator)(void);
long start_elevator(void) {
    int i = 0;
	if(currentState == OFFLINE)
    {
        currentState = IDLE;
        currentFloor = 1; 
        currentOccupants = 0;
        passengerServiced = 0;
        return 0;
    } else {
        return 1;
    }
}

extern long (*STUB_stop_elevator)(void);
long stop_elevator(void) {
	if(currentState == STOPPING){
        printk("elevator is stopping\n");
        return 1; 
    }
    else{ 
        if(currentOccupants == 0){
            printk("elevator is stop\n"); 
            currentState = OFFLINE;
        }
        else{
            currentState = STOPPING;
        }
        return 0;
    }

}

extern long (*STUB_issue_request)(int,int,int);
long issue_request(int start_floor,int destination_floor, int type) {
    printk("We got a request! %d, %d, %d\n", start_floor,destination_floor,type); 
    if(OFFLINE) {
        return 1; 
    }
    else if((start_floor > 10 || start_floor < 1) && (destination_floor > 10 || destination_floor < 1)) {
        return 1; 
    }
    else if(type > 2 || type < 0) {
        return 1; 
    }
    else {
        waitingPassengers++;
        addPassengersToQueue(type,start_floor,destination_floor);
        passengerEachFloor[start_floor-1]++;
        return 0; 
    }
}
int addPassengersToQueue(int type, int floor, int endFloor) {
    printk("adding passenger to queue....");
    int weight;
    struct passenger *tempPassenger; 

    weight = getWeight(type);

    tempPassenger = kmalloc(sizeof(struct passenger), __GFP_RECLAIM | __GFP_IO | __GFP_FS); 
    
    if(tempPassenger == NULL)
        return -ENOMEM; 
    
    
    
    tempPassenger->startFloor = floor; 
    tempPassenger->endFloor = endFloor; 
    tempPassenger->weight = weight;
    tempPassenger->type = type;
    mutex_lock_interruptible(&mutexForPassengersQueue);
    list_add_tail(&tempPassenger->next, &listOfPassengers[floor - 1]);  //adding passenger to queue on their start floor
    mutex_unlock(&mutexForPassengersQueue);
    printk("done adding to queue....");

}

int addPassengersToElevator(void) {
    struct passenger *newPassenger, *temp3;
    struct list_head *temp, *temp2; 
    printk("ADDING PASSENGER 1");
    currentState = LOADING;

    mutex_lock_interruptible(&mutexForPassengersQueue);
    list_for_each_safe(temp, temp2, &listOfPassengers[currentFloor -1]){  //allows you iterate throught list even thought null elements are present
        newPassenger = list_entry(temp, struct passenger, next);
        //creating a new reference to a new pointer(?
        if(currentOccupants > 10  || (currentWeight + newPassenger->weight) > 100){
            mutex_unlock(&mutexForPassengersQueue);
            return 0;
        }                                                                                                                                                                                                                                    
         temp3 = kmalloc(sizeof(struct passenger), __GFP_RECLAIM | __GFP_IO | __GFP_FS);
         temp3->type = newPassenger->type; 
         temp3->startFloor = newPassenger->startFloor;
         temp3->endFloor = newPassenger->endFloor; 
         temp3->weight = newPassenger->weight; 
         printk("ADDING PASSENGER 2");
        mutex_lock_interruptible(&mutexForElevatorList);
        currentOccupants++;
        currentWeight += temp3->weight;
        list_add_tail(&temp3->next, &elevatorList);
        passengerEachFloor[temp3->startFloor-1]--;
        mutex_unlock(&mutexForElevatorList);
        list_del(temp);
        printk("ADDING PASSENGER 3");
        kfree(newPassenger);
        waitingPassengers--;

    }
    mutex_unlock(&mutexForPassengersQueue);
     printk("PASSENGER ADDED");
   return 1;
}

void moveFloor(void){


    struct list_head *ptrTemp, *ptrTemp2; 
    struct passenger * temp;
    temp = kmalloc(sizeof(struct passenger), __GFP_RECLAIM | __GFP_IO | __GFP_FS);
    mutex_lock_interruptible(&mutexForElevatorList);
    temp = list_first_entry_or_null(&elevatorList, struct passenger, next);
    printk("First Entry End Floor: %d", temp->endFloor );
    int difference = 0;
    if(temp->endFloor < currentFloor){
        if(currentState != STOPPING)
            currentState = DOWN;
        difference = currentFloor - temp->endFloor;
    }
    else if(temp->endFloor > currentFloor){
        if(currentState != STOPPING)
           currentState = UP;
        difference = temp->endFloor -  currentFloor;
    }

    printk("NEW CODE...\n");
    ssleep(difference * 2);
    currentFloor = temp->endFloor;

    printk("NEW CODE2...\n");
    passengerServiced++;
    currentWeight -= temp->weight; 
    list_del(elevatorList.next);

    printk("NEW CODE3...\n");
    currentOccupants--;
    mutex_unlock(&mutexForElevatorList);
    kfree(temp);
}
int openElevatorModule(struct inode *sp_inode, struct file *sp_file)
{
    rp  = 1; 
    message = kmalloc(sizeof(char) * 3000, __GFP_RECLAIM | __GFP_IO | __GFP_FS); 
    return 0;
}
static ssize_t procfile_read(struct file* sp_file, char __user * ubuf, size_t count, loff_t *ppos)
{
    message = kmalloc(sizeof(char) * 3000, __GFP_RECLAIM | __GFP_IO | __GFP_FS); 
    switch(currentState)
    {
        case OFFLINE: 
            sprintf(message,"Elevator State: OFFLINE\n"
                            "Current Floor: %d\n"
                            "Current Weight: %d \n"
                            "Elevator Status: %s\n"
                            "Number of Passengers:%d \n"
                            "Number of Passengers waiting: %d\n"
                            "Number of Passengers Serviced %d\n"
                            "\n%s\n", currentFloor, currentWeight, getPassengersOnElevator(), currentOccupants, waitingPassengers, passengerServiced, getPassengerList()); 
            break;
        case IDLE: 
            sprintf(message,"Elevator State: IDLE\n"
                            "Current Floor: %d\n"
                            "Current Weight: %d \n"
                            "Elevator Status: %s\n"
                            "Number of Passengers:%d \n"
                            "Number of Passengers Serviced %d\n"
                            "\n%s\n", currentFloor, currentWeight, getPassengersOnElevator(), currentOccupants, waitingPassengers, passengerServiced, getPassengerList());
            break;
        case UP: 
            sprintf(message,"Elevator State: UP\n"
                            "Current Floor: %d\n"
                            "Current Weight: %d \n"
                            "Elevator Status: %s\n"
                            "Number of Passengers:%d \n"
                            "Number of Passengers Serviced %d\n"
                            "\n%s\n", currentFloor, currentWeight, getPassengersOnElevator(), currentOccupants, waitingPassengers, passengerServiced, getPassengerList());
            break;
        case DOWN: 
            sprintf(message,"Elevator State: DOWN\n"
                            "Current Floor: %d\n"
                            "Current Weight: %d \n"
                            "Elevator Status: %s\n"
                            "Number of Passengers:%d \n"
                            "Number of Passengers Serviced %d\n"
                            "\n%s\n", currentFloor, currentWeight, getPassengersOnElevator(), currentOccupants, waitingPassengers, passengerServiced, getPassengerList());
            break;
        case LOADING: 
            sprintf(message,"Elevator State: LOADING\n"
                            "Current Floor: %d\n"
                            "Current Weight: %d \n"
                            "Elevator Status: %s\n"
                            "Number of Passengers:%d \n"
                            "Number of Passengers Serviced %d\n"
                            "\n%s\n", currentFloor, currentWeight, getPassengersOnElevator(), currentOccupants, waitingPassengers, passengerServiced, getPassengerList());
            break;
        case STOPPING:
         sprintf(message,"Elevator State: STOPPING\n"
                            "Current Floor: %d\n"
                            "Current Weight: %d \n"
                            "Elevator Status: %s\n"
                            "Number of Passengers:%d \n"
                            "Number of Passengers Serviced %d\n"
                            "\n%s\n", currentFloor, currentWeight, getPassengersOnElevator(), currentOccupants, waitingPassengers, passengerServiced, getPassengerList());
            break;
        default:
            sprintf(message, "");
            break;
    };
	int len = strlen(message); 
     rp = !rp; 

     if(rp)
        return 0; 

    copy_to_user(ubuf,message,len);
    // kfree(stringTemp0); 
    // kfree(stringTemp1);
    kfree(message);
    return len;
}

int threadRelease(struct inode *sp_inode, struct file *sp_file)
{
    kfree(message); 
    return 0;
}

int elevatorRun(void *data)
{
    int k = 1;

    while(!kthread_should_stop())
    {
        if(currentState == STOPPING && currentOccupants == 0){
                currentState = OFFLINE;
        }
        if(currentState != OFFLINE){ 
            if(currentState != STOPPING) 
                addPassengersToElevator();
            if(currentOccupants != 0){
                moveFloor();
            }
            else if(waitingPassengers != 0){
             if(k < FLOORS){
                    k++;
                    currentFloor = k;
                    ssleep(2);
                }
                printk("Status is: %d", currentState);
                printk("waiting passengers: %d", waitingPassengers); 
                printk("currentFloor:%d\n", currentFloor); 
                printk("elevatorSize: %d\n", getElevatorSize());
                printk("total serviced: %d\n", passengerServiced);
                printk("--------------------------------------------------------\n");

                if(k == FLOORS)
                    k = 0;
            }
            else
                currentState = IDLE;
        }
    }
    
    return 0;
}

static struct file_operations procfile_fops = {
	.owner = THIS_MODULE,
	.read = procfile_read
};

static int elevator_init(void)
{
    int i = 0;
    //------SET FILE OPERATION------
    procfile_fops.open = openElevatorModule; 
    procfile_fops.read = procfile_read; 
    procfile_fops.release = threadRelease;

	proc_entry = proc_create("elevator", 0664, NULL, &procfile_fops);
    elevator_thread = kthread_run(elevatorRun, NULL, "Elevator Thread");
    printk("Elevator init.....\n");
    
    //-----INIT SYSTEM CALLS----
    STUB_start_elevator  = start_elevator; 
    STUB_issue_request = issue_request; 
    STUB_stop_elevator = stop_elevator;
    
    //------INIT MUTEXES------
    mutex_init(&mutexForElevatorList); 
    mutex_init(&mutexForPassengersQueue); 

    //-------SET ELEVATOR INITITAL STATE------
    currentState = OFFLINE;
    currentFloor = 1; 
    currentOccupants = 0; 
    currentWeight = 0; 
    waitingPassengers = 0;
    passengerServiced = 0;
    
    
    //------INIT LIST------                         //moved init list down here because we can have a queue before the elevator starts (?sud)
    while(i < FLOORS)
    {
        INIT_LIST_HEAD(&listOfPassengers[i]);
        i++;
    }
     INIT_LIST_HEAD(&elevatorList);



	if (proc_entry == NULL)
		return -ENOMEM;
	
	return 0;
}

static void elevator_exit(void)
{
	proc_remove(proc_entry);
    kthread_stop(elevator_thread);
    STUB_start_elevator  = NULL; 
    STUB_issue_request = NULL; 
    STUB_stop_elevator = NULL;
    printk("Elevator Stopped\n");
	return;
}
int getWeight(int type)
{
    if(CAT)
        return 15; 
    else if(DOG)
        return 45; 
    else if(LIZARD)
        return 5;
}
int getElevatorSize(void){
    struct passenger *entry; 
    struct list_head *temp;
    int i = 0; 
    mutex_lock_interruptible(&mutexForElevatorList);
    list_for_each(temp, &elevatorList){
            entry = list_entry(temp, struct passenger, next);
        i++;
    }
    mutex_unlock(&mutexForElevatorList);
    return i;
}
void print_current_elevator(void) {
    struct list_head *temp;
    struct passenger *entry;
    int i = 0;
    list_for_each(temp, &elevatorList) {
        entry = list_entry(temp, struct passenger, next);
        printk("Passenger (%d): %d\n", i, entry->type);
        i++;
    }
}
char * getPassengerList(void)
{ 
    stringTemp0 = kmalloc(sizeof(char) * 3000, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
    switch(currentFloor)
    {
        case 1: 
            sprintf(stringTemp0,"[] Floor 10: \n"
                 "[] Floor 9: \n"
                 "[] Floor 8: \n"
                 "[] Floor 7: \n"
                 "[] Floor 6: \n"
                 "[] Floor 5: \n"
                 "[] Floor 4: \n"
                 "[] Floor 3: \n"
                 "[] Floor 2: \n"
                 "[*] Floor 1: \n");
            break;
        case 2: 
            sprintf(stringTemp0,"[] Floor 10: %d %s\n"
                 "[] Floor 9: %d %s\n"
                 "[] Floor 8: %d %s\n"
                 "[] Floor 7: %d %s\n"
                 "[] Floor 6: %d %s\n"
                 "[] Floor 5: %d %s\n"
                 "[] Floor 4: %d %s\n"
                 "[] Floor 3: %d %s\n"
                 "[*] Floor 2: %d %s\n"
                 "[] Floor 1: %d %s\n",passengerEachFloor[0], getPassengersOnFloor(1), passengerEachFloor[1], getPassengersOnFloor(2), passengerEachFloor[2], getPassengersOnFloor(3), passengerEachFloor[3], getPassengersOnFloor(4), passengerEachFloor[4], getPassengersOnFloor(5),
                                passengerEachFloor[5], getPassengersOnFloor(6), passengerEachFloor[6], getPassengersOnFloor(7), passengerEachFloor[7], getPassengersOnFloor(8), passengerEachFloor[8], getPassengersOnFloor(9), passengerEachFloor[9], getPassengersOnFloor(10));
             break;
        case 3: 
            sprintf(stringTemp0,"[] Floor 10: %d %s\n"
                 "[] Floor 9: %d %s\n"
                 "[] Floor 8: %d %s\n"
                 "[] Floor 7: %d %s\n"
                 "[] Floor 6: %d %s\n"
                 "[] Floor 5: %d %s\n"
                 "[] Floor 4: %d %s\n"
                 "[*] Floor 3: %d %s\n"
                 "[] Floor 2: %d %s\n"
                 "[] Floor 1: %d %s\n",passengerEachFloor[0], getPassengersOnFloor(1), passengerEachFloor[1], getPassengersOnFloor(2), passengerEachFloor[2], getPassengersOnFloor(3), passengerEachFloor[3], getPassengersOnFloor(4), passengerEachFloor[4], getPassengersOnFloor(5),
                                passengerEachFloor[5], getPassengersOnFloor(6), passengerEachFloor[6], getPassengersOnFloor(7), passengerEachFloor[7], getPassengersOnFloor(8), passengerEachFloor[8], getPassengersOnFloor(9), passengerEachFloor[9], getPassengersOnFloor(10));
            break;
        case 4: 
            sprintf(stringTemp0,"[] Floor 10: %d %s\n"
                 "[] Floor 9: %d %s\n"
                 "[] Floor 8: %d %s\n"
                 "[] Floor 7: %d %s\n"
                 "[] Floor 6: %d %s\n"
                 "[] Floor 5: %d %s\n"
                 "[*] Floor 4: %d %s\n"
                 "[] Floor 3: %d %s\n"
                 "[] Floor 2: %d %s\n"
                 "[] Floor 1: %d %s\n",passengerEachFloor[0], getPassengersOnFloor(1), passengerEachFloor[1], getPassengersOnFloor(2), passengerEachFloor[2], getPassengersOnFloor(3), passengerEachFloor[3], getPassengersOnFloor(4), passengerEachFloor[4], getPassengersOnFloor(5),
                                passengerEachFloor[5], getPassengersOnFloor(6), passengerEachFloor[6], getPassengersOnFloor(7), passengerEachFloor[7], getPassengersOnFloor(8), passengerEachFloor[8], getPassengersOnFloor(9), passengerEachFloor[9], getPassengersOnFloor(10));
            break;
        case 5: 
            sprintf(stringTemp0,"[] Floor 10: %d %s\n"
                 "[] Floor 9: %d %s\n"
                 "[] Floor 8: %d %s\n"
                 "[] Floor 7: %d %s\n"
                 "[] Floor 6: %d %s\n"
                 "[*] Floor 5: %d %s\n"
                 "[] Floor 4: %d %s\n"
                 "[] Floor 3: %d %s\n"
                 "[] Floor 2: %d %s\n"
                 "[] Floor 1: %d %s\n",passengerEachFloor[0], getPassengersOnFloor(1), passengerEachFloor[1], getPassengersOnFloor(2), passengerEachFloor[2], getPassengersOnFloor(3), passengerEachFloor[3], getPassengersOnFloor(4), passengerEachFloor[4], getPassengersOnFloor(5),
                                passengerEachFloor[5], getPassengersOnFloor(6), passengerEachFloor[6], getPassengersOnFloor(7), passengerEachFloor[7], getPassengersOnFloor(8), passengerEachFloor[8], getPassengersOnFloor(9), passengerEachFloor[9], getPassengersOnFloor(10));
            break;
        case 6: 
            sprintf(stringTemp0,"[] Floor 10: %d %s\n"
                 "[] Floor 9: %d %s\n"
                 "[] Floor 8: %d %s\n"
                 "[] Floor 7: %d %s\n"
                 "[*] Floor 6: %d %s\n"
                 "[] Floor 5: %d %s\n"
                 "[] Floor 4: %d %s\n"
                 "[] Floor 3: %d %s\n"
                 "[] Floor 2: %d %s\n"
                 "[] Floor 1: %d %s\n",passengerEachFloor[0], getPassengersOnFloor(1), passengerEachFloor[1], getPassengersOnFloor(2), passengerEachFloor[2], getPassengersOnFloor(3), passengerEachFloor[3], getPassengersOnFloor(4), passengerEachFloor[4], getPassengersOnFloor(5),
                                passengerEachFloor[5], getPassengersOnFloor(6), passengerEachFloor[6], getPassengersOnFloor(7), passengerEachFloor[7], getPassengersOnFloor(8), passengerEachFloor[8], getPassengersOnFloor(9), passengerEachFloor[9], getPassengersOnFloor(10));
            break;
        case 7: 
            sprintf(stringTemp0,"[] Floor 10: %d %s\n"
                 "[] Floor 9: %d %s\n"
                 "[] Floor 8: %d %s\n"
                 "[*] Floor 7: %d %s\n"
                 "[] Floor 6: %d %s\n"
                 "[] Floor 5: %d %s\n"
                 "[] Floor 4: %d %s\n"
                 "[] Floor 3: %d %s\n"
                 "[] Floor 2: %d %s\n"
                 "[] Floor 1: %d %s\n",passengerEachFloor[0], getPassengersOnFloor(1), passengerEachFloor[1], getPassengersOnFloor(2), passengerEachFloor[2], getPassengersOnFloor(3), passengerEachFloor[3], getPassengersOnFloor(4), passengerEachFloor[4], getPassengersOnFloor(5),
                                passengerEachFloor[5], getPassengersOnFloor(6), passengerEachFloor[6], getPassengersOnFloor(7), passengerEachFloor[7], getPassengersOnFloor(8), passengerEachFloor[8], getPassengersOnFloor(9), passengerEachFloor[9], getPassengersOnFloor(10));
            break;
        case 8: 
            sprintf(stringTemp0,"[] Floor 10: %d %s\n"
                 "[] Floor 9: %d %s\n"
                 "[*] Floor 8: %d %s\n"
                 "[] Floor 7: %d %s\n"
                 "[] Floor 6: %d %s\n"
                 "[] Floor 5: %d %s\n"
                 "[] Floor 4: %d %s\n"
                 "[] Floor 3: %d %s\n"
                 "[] Floor 2: %d %s\n"
                 "[] Floor 1: %d %s\n",passengerEachFloor[0], getPassengersOnFloor(1), passengerEachFloor[1], getPassengersOnFloor(2), passengerEachFloor[2], getPassengersOnFloor(3), passengerEachFloor[3], getPassengersOnFloor(4), passengerEachFloor[4], getPassengersOnFloor(5),
                                passengerEachFloor[5], getPassengersOnFloor(6), passengerEachFloor[6], getPassengersOnFloor(7), passengerEachFloor[7], getPassengersOnFloor(8), passengerEachFloor[8], getPassengersOnFloor(9), passengerEachFloor[9], getPassengersOnFloor(10));
            break;
        case 9: 
            sprintf(stringTemp0,"[] Floor 10: %d %s\n"
                 "[*] Floor 9: %d %s\n"
                 "[] Floor 8: %d %s\n"
                 "[] Floor 7: %d %s\n"
                 "[] Floor 6: %d %s\n"
                 "[] Floor 5: %d %s\n"
                 "[] Floor 4: %d %s\n"
                 "[] Floor 3: %d %s\n"
                 "[] Floor 2: %d %s\n"
                 "[] Floor 1: %d %s\n",passengerEachFloor[0], getPassengersOnFloor(1), passengerEachFloor[1], getPassengersOnFloor(2), passengerEachFloor[2], getPassengersOnFloor(3), passengerEachFloor[3], getPassengersOnFloor(4), passengerEachFloor[4], getPassengersOnFloor(5),
                                passengerEachFloor[5], getPassengersOnFloor(6), passengerEachFloor[6], getPassengersOnFloor(7), passengerEachFloor[7], getPassengersOnFloor(8), passengerEachFloor[8], getPassengersOnFloor(9), passengerEachFloor[9], getPassengersOnFloor(10));
            break;
        case 10: 
            sprintf(stringTemp0,"[*] Floor 10: %d %s\n"
                 "[] Floor 9: %d %s\n"
                 "[] Floor 8: %d %s\n"
                 "[] Floor 7: %d %s\n"
                 "[] Floor 6: %d %s\n"
                 "[] Floor 5: %d %s\n"
                 "[] Floor 4: %d %s\n"
                 "[] Floor 3: %d %s\n"
                 "[] Floor 2: %d %s\n"
                 "[] Floor 1: %d %s\n",passengerEachFloor[0], getPassengersOnFloor(1), passengerEachFloor[1], getPassengersOnFloor(2), passengerEachFloor[2], getPassengersOnFloor(3), passengerEachFloor[3], getPassengersOnFloor(4), passengerEachFloor[4], getPassengersOnFloor(5),
                                passengerEachFloor[5], getPassengersOnFloor(6), passengerEachFloor[6], getPassengersOnFloor(7), passengerEachFloor[7], getPassengersOnFloor(8), passengerEachFloor[8], getPassengersOnFloor(9), passengerEachFloor[9], getPassengersOnFloor(10));
            break;
        default:
            sprintf(stringTemp0, " ");
            break;
    }
    return stringTemp0;
}
char * getPassengersOnFloor(int floor)
{
    char *temp;  
    char *temp2;
    struct passenger *entry; 
    struct list_head *ptemp;
    struct list_head *nullptemp;
    int check = 0;
    temp = kmalloc(sizeof(char) * 20, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
    temp2 = kmalloc(sizeof(char) * 20, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
    strcpy(temp2, "");
    mutex_lock_interruptible(&mutexForPassengersQueue);
    list_for_each_safe(ptemp, nullptemp, &listOfPassengers[floor-1]) {
        entry = list_entry(ptemp, struct passenger, next);
        sprintf(temp,"%s", getPassengerType(entry->type));
        //printk("%s\n", temp);
        strcat(temp2,temp);
        //printk("%s\n", temp2);
        check = 1;
    }
    mutex_unlock(&mutexForPassengersQueue);
    if(!check)
    {
        strcpy(temp2, " ");
    }
    printk("%s\n", temp2);
    return temp2;
}
char * getPassengerType(int type)
{
    stringTemp1 = kmalloc(sizeof(char) * 4, __GFP_RECLAIM | __GFP_IO | __GFP_FS); 
    switch(type)
    {
        case CAT: 
            sprintf(stringTemp1, "C "); 
            break;
        case LIZARD: 
            sprintf(stringTemp1, "L "); 
            break; 
        case DOG: 
            sprintf(stringTemp1, "D "); 
            break;
    }

    return stringTemp1;
}
char * getPassengersOnElevator(void)
{
    char *temp;  
    char *temp2;
    struct passenger *entry; 
    struct list_head *ptemp;
    struct list_head *nullptemp;
    int check = 0;
    temp = kmalloc(sizeof(char) * 20, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
    temp2 = kmalloc(sizeof(char) * 20, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
    strcpy(temp2, "");
    mutex_lock_interruptible(&mutexForElevatorList);
    list_for_each_safe(ptemp, nullptemp, &elevatorList) {
        entry = list_entry(ptemp, struct passenger, next);
        sprintf(temp,"%s", getPassengerType(entry->type));
        //printk("%s\n", temp);
        strcat(temp2,temp);
        //printk("%s\n", temp2);
        check = 1;
    }
    mutex_unlock(&mutexForElevatorList);
    if(!check)
    {
        strcpy(temp2, " ");
    }
    printk("%s\n", temp2);
    return temp2;
}
module_init(elevator_init);
module_exit(elevator_exit);
