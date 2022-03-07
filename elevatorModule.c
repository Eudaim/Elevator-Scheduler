#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/linkage.h>

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



static struct proc_dir_entry* proc_entry;

static char msg[BUF_LEN];
static int procfs_buf_len;

int numOccupants; 
int currentOccupants;
int waitingPassengers; 
int passengerServiced; 

int currentFloor; 
int currentWeight; 
int currentState; 
int nextState;
int occupantType;
int eachFloor[FLOORS];
struct customer 
{
    int startFloor; 
    int endFloor; 
    int type; 
};

extern long (*STUB_start_elevator)(void);
long start_elevator(void) {
	if(currentState == OFFLINE)
    {
        currentState = IDLE;
        currentFloor = 1; 
        numOccupants = 0;
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
    else if((start_floor > 10 || start_floor < 0) && (destination_floor > 10 || destination_floor < 0)) {
        return 1; 
    }
    else if(type > 2 || type < 0) {
        return 1; 
    }
    else {
        waitingPassengers++;
        eachFloor[start_floor-1]++;
        return 0; 
    }
}

static ssize_t procfile_read(struct file* file, char * ubuf, size_t count, loff_t *ppos)
{
	printk(KERN_INFO "proc_read\n");
	procfs_buf_len = strlen(msg);

	if (*ppos > 0 || count < procfs_buf_len)
		return 0;

	if (copy_to_user(ubuf, msg, procfs_buf_len))
		return -EFAULT;

	*ppos = procfs_buf_len;

	printk(KERN_INFO "gave to user %s\n", msg);

	return procfs_buf_len;
}


static ssize_t procfile_write(struct file* file, const char * ubuf, size_t count, loff_t* ppos)
{
	printk(KERN_INFO "proc_write\n");

	if (count > BUF_LEN)
		procfs_buf_len = BUF_LEN;
	else
		procfs_buf_len = count;

	copy_from_user(msg, ubuf, procfs_buf_len);

	printk(KERN_INFO "got from user: %s\n", msg);

	return procfs_buf_len;
}


static struct file_operations procfile_fops = {
	.owner = THIS_MODULE,
	.read = procfile_read,
	.write = procfile_write,
};

static int elevator_init(void)
{
	proc_entry = proc_create("hello", 0666, NULL, &procfile_fops);

    printk("Elevator init.....\n");
    
    currentState = OFFLINE; 
    currentState = IDLE; 

	if (proc_entry == NULL)
		return -ENOMEM;
	
	return 0;
}

static void elevator_exit(void)
{
	proc_remove(proc_entry);
	return;
}
static int calWeight(int type)
{
    if(CAT)
        return 15; 
    else if(DOG)
        return 45; 
    else if(LIZARD)
        return 5;
}


module_init(elevator_init);
module_exit(elevator_exit);
