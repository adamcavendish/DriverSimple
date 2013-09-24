/**
 * chardev.c: Creates a read-only char device that says how many times you've read from the dev file
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h> // for put_user

#include "chardev.h"

// called at the module is loaded
int init_module(void)
{
	Major = register_chrdev(0, DEVICE_NAME, &fops);

	if(Major < 0) {
		printk(KERN_ALERT "Registering char device failed with %d\n", Major);
		return Major;
	}//if
	
	printk(KERN_INFO "I was assigned major number %d. To talk to\n", Major);
	printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
	printk(KERN_INFO "the device file.\n");
	printk(KERN_INFO "Remove the device file and module when done.\n");

	return SUCCESS;
}//init_module()

// called when the module is unloaded.
void cleanup_module(void)
{
	// unregister the device
	unregister_chrdev(Major, DEVICE_NAME);

//	if(ret < 0)
//		printk(KERN_ALERT "Error in unregister_chrdev: %d\n", ret);
}//cleanup_module()

// called when a process tries to open the device file, like "cat /dev/mycharfile"
static int device_open(struct inode * inode, struct file * file)
{
	static int counter = 0;
	
	if(Device_Open)
		return -EBUSY;

	++Device_Open;
	
	sprintf(msg, "I already told you %d times Hello world!\n", ++counter);
	msg_Ptr = msg;
	try_module_get(THIS_MODULE);

	return SUCCESS;
}//device_open(inode, file)

// called when a process closes the device file
static int device_release(struct inode * inode, struct file * file)
{
	--Device_Open;
	
	// decrement the usage count, or else once you opened the file, you'll never get rid of the module.
	module_put(THIS_MODULE);

	return 0;
}//device_release(inode, file)

// called when a process, which already opened the dev file, attempts to read from it.
static ssize_t device_read(
		struct file *filp,	// see include/linux/fs.h
		char * buffer,		// buffer to fill with data
		size_t length,		// length of the buffer
		loff_t * offset)
{
	// number of bytes actually written to the buffer
	int bytes_read = 0;

	// if we're at the end of the message, return 0 signifying the end of file
	if(*msg_Ptr == 0)
		return 0;

	// Actually put the data into the buffer
	while(length && *msg_Ptr) {
		// the buffer is in the user data segment, not the kernel segment so "*" assignment won't work.
		// We have to use put_user with copies data from the kernel data segment to the user data segment
		put_user(*(msg_Ptr++), buffer++);

		length--;
		bytes_read++;
	}//while

	// most read functions return the number of bytes put into the buffer
	return bytes_read;
}//device_read(filp, buffer, length, offset)

// called when a process writes to dev file: echo "hi" > /dev/hello
static ssize_t device_write(struct file * filp, const char * buff, size_t len, loff_t * off)
{
	printk(KERN_ALERT "sorry, writing operation is not supported.\n");
	return -EINVAL;
}//device_write(filp, buff, len, off)



