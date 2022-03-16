#include <unistd.h>
#include <sys/syscall.h> 
#include <stdlib.h>
int main(){
    syscall(SYS_write, 1, "does this work?",0); 
    syscall(SYS_sync);
    syscall(SYS_getgid);
    syscall(SYS_gettid);
    return 0;
}