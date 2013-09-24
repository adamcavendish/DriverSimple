#pragma once

int init_module(void);
void cleanup_module(void);

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "chardev"	// dev name as it appears in /proc/devices
#define BUF_LEN 80				// max length of the message from the device

static int Major;				// major number assigned to our device driver
static int Device_Open = 0;		// is device opened?
static char msg[BUF_LEN];		// Used to prvent multiple access to device
static char * msg_Ptr;			// The msg the device will give when asked

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};


