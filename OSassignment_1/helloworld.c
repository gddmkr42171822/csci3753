/*This file defines the HelloWorld system call*/ 
#include <linux/kernel.h>
#include <linux/linkage.h>
asmlinkage int sys_helloworld(){
	printk(KERN_EMERG "rowe7280 says Hello World!\n"); 
	return 0;
}
