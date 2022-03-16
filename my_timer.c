#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("Dual BSD/GPL");

#define BUF_LEN 100
static struct timespec mydifftime(void);
static struct proc_dir_entry* proc_entry;
static char msg[BUF_LEN];
static char *message;
static int procfs_buf_len;
static struct timespec elapsedTime;
static struct timespec previousTime = {0, 0}; 
static struct timespec  currentTime;

static ssize_t procfile_read(struct file* file, char * ubuf, size_t count, loff_t *ppos)
{
	
	if (*ppos > 0 || count < procfs_buf_len)
		return 0;
    message = kmalloc(sizeof(char) * 3000, __GFP_RECLAIM | __GFP_IO | __GFP_FS); 		
	currentTime = current_kernel_time();
	
	if(previousTime.tv_sec != 0 && previousTime.tv_nsec != 0 ) {
		printk(KERN_INFO "previous time is not zero\n");
		elapsedTime = mydifftime();
		sprintf(message, "current time: %ld.%ld \nelapsed time: %ld.%ld"
						"\n", currentTime.tv_sec, currentTime.tv_nsec, elapsedTime.tv_sec, elapsedTime.tv_nsec);
	}
	else{
		previousTime = currentTime;
		sprintf(message, "current time: %ld.%ld \n", currentTime.tv_sec, currentTime.tv_nsec);
	}
	procfs_buf_len = strlen(message);
	if(copy_to_user(ubuf, message, procfs_buf_len))
		return -EFAULT;
	printk(KERN_ALERT "currentTime: %ld. %ld\n", currentTime.tv_sec, currentTime.tv_nsec);
	printk(KERN_ALERT "previousTime: %ld. %ld\n", previousTime.tv_sec, previousTime.tv_nsec);
	printk(KERN_ALERT "elapsedTime: %ld. %ld\n", elapsedTime.tv_sec, elapsedTime.tv_nsec);
	(*ppos) += procfs_buf_len;
	
	return procfs_buf_len;
}

static struct timespec mydifftime()
{
	struct timespec eTime;
	if(currentTime.tv_nsec < previousTime.tv_nsec) {
		currentTime.tv_sec -= 1; 
		currentTime.tv_nsec += 1000000000;
		eTime.tv_sec = currentTime.tv_sec - previousTime.tv_sec;
		eTime.tv_nsec = currentTime.tv_nsec - previousTime.tv_nsec;

	}else{
		eTime.tv_sec = currentTime.tv_sec - previousTime.tv_sec;
		eTime.tv_nsec = currentTime.tv_nsec - previousTime.tv_nsec;
	}

	return eTime;

}

 static ssize_t procfile_write(struct file* file, const char * ubuf, size_t count, loff_t* ppos)
{
	printk(KERN_INFO "proc_write\n");

	if (count > BUF_LEN)
		procfs_buf_len = BUF_LEN;
	else
		procfs_buf_len = count;

	return procfs_buf_len;
}


static struct file_operations procfile_fops = {
	.owner = THIS_MODULE,
	.read = procfile_read,
	.write = procfile_write,
};

static int timer_init(void)
{
	proc_entry = proc_create("timer", 0666, NULL, &procfile_fops);
	if (proc_entry == NULL)
		return -ENOMEM;
	
	return 0;
}

static void timer_exit(void)
{
	proc_remove(proc_entry);
	return;
}



module_init(timer_init);
module_exit(timer_exit);
