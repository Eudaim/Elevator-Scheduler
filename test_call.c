#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>

/* System call stub */
long (*STUB_test_call)(int) = NULL;
EXPORT_SYMBOL(STUB_test_call);

/* System call wrapper */
SYSCALL_DEFINE1(test_call, int, test_int) {
	printk(KERN_NOTICE "Inside SYSCALL_DEFINE1 block. %s: Your int is %d\n", __FUNCTION__, test_int);
	if (STUB_test_call != NULL)
		return STUB_test_call(test_int);
	else
		return -ENOSYS;
}