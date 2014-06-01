#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/uaccess.h>

#include <linux/types.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/watchdog.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/notifier.h>
#include <linux/reboot.h>

#include "processdog.h"

static int timer_margin = PARAM_TIMER_MARGIN;
module_param(timer_margin, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(timer_margin,
        "processdog's timer margin in milliseconds. Default="__MODULE_STRING(PARAM_TIMER_MARGIN));

static int is_test = PARAM_IS_TEST;
module_param(is_test, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(is_test,
        "processdog's test flag. Default="__MODULE_STRING(PARAM_IS_TEST));

static unsigned long processdog_is_opened = 0;
static int processdog_pid = -1;

// --------------------------------------------------
// watchdog operations

static int processdog_isdigit(char c);

static ssize_t processdog_write(struct file *, const char __user *, size_t, loff_t *);
static long processdog_ioctl(struct file *, unsigned int, unsigned long);
static int processdog_open(struct inode *, struct file *);
static int processdog_release(struct inode *, struct file *);

static int processdog_notify_sys(struct notifier_block *this, unsigned long code, void *unused);

static void processdog_fire(unsigned long);
static int processdog_pet(void);
static int processdog_stop(void);
static int processdog_set_timer_margin(int);
static struct timer_list processdog_tictoc = TIMER_INITIALIZER(processdog_fire, 0, 0);

static int processdog_isdigit(char c) {
    return (c > '0') && (c < '9');
}

static void processdog_fire(unsigned long data) {
    // if the timer expires, this function will be triggered.
    if(is_test) {
        printk(KERN_NOTICE INFO_PREFIX "watchdog fired, ignored by test.\n");
        return;
    }//if

    printk(KERN_NOTICE INFO_PREFIX "watchdog fired!\n");
    // @TODO
}

static int processdog_pet(void) {
    printk(KERN_DEBUG INFO_PREFIX "pet()\n");
    // pet the dog
	mod_timer(&processdog_tictoc, jiffies + (timer_margin * HZ / 1000));
    return 0;
}

static int processdog_stop(void) {
    printk(KERN_DEBUG INFO_PREFIX "stop()\n");
    // stop the watchdog
    del_timer(&processdog_tictoc);
    return 0;
}

static int processdog_set_timer_margin(int t) {
    printk(KERN_DEBUG INFO_PREFIX "set_timer_margin %d\n", t);
    // less than 1 millisecond or greater than 10 seconds is regarded as invalid
    if(t < 1 || t > 10 * 1000) {
		return -EINVAL;
    }//if

    timer_margin = t;
    return 0;
}

static int processdog_open(struct inode * inode, struct file * file) {
    printk(KERN_DEBUG INFO_PREFIX "open()\n");
	if(test_and_set_bit(0, &processdog_is_opened)) {
        printk(KERN_ERR INFO_PREFIX "test_and_set_bit to is_opened failed!\n");
		return -EBUSY;
    }//if

    if(!try_module_get(THIS_MODULE)) {
        printk(KERN_ERR INFO_PREFIX "try_module_get failed!\n");
        return -EBUSY;
    }//if

    processdog_pet();
    return nonseekable_open(inode, file);
}

static int processdog_release(struct inode * inode, struct file * file) {
    printk(KERN_DEBUG INFO_PREFIX "release()\n");
    processdog_stop();
    module_put(THIS_MODULE);
	clear_bit(0, &processdog_is_opened);
    return 0;
}

static ssize_t processdog_write(struct file * file, const char __user * data, size_t len,
        loff_t *ppos)
{
#   define PROCESSDOG_PET   0
#   define PROCESSDOG_EXIT  1
#   define PROCESSDOG_START 2
    /*
     * write protocol:
     * [pid] [pet|exit|start]
     */
    printk(KERN_DEBUG INFO_PREFIX "write()\n");
    if(len) {
        char buffer[32];
        int pid = -1;
        int action = -1;

        char * pbuffer = NULL;
        int i = 0;

        // readin from usr space data
        pbuffer = buffer;
		for(i = 0; i != len; i++) {
            char c;
            if(get_user(c, data + i)) {
                return -EFAULT;
            } else {
                *pbuffer++ = c;
            }//if-else
        }//for

        // print debug info: buffer
        printk(KERN_DEBUG INFO_PREFIX "write() get: %s\n", buffer);

        // format the buffer
        {
            // get pid
            char numbuff[32];
            char * pnumbuff = numbuff;
            pbuffer = buffer;
            while(processdog_isdigit(*pbuffer)) {
                *pnumbuff++ = *pbuffer++;
            }//while
            *pnumbuff = '\0';
            kstrtoint(numbuff, 10, &pid);
            if(pid == 0) {
                return -EFAULT;
            }//if

            // --------------------
            // get action
            if(*pbuffer != ' ') {
                return -EFAULT;
            } else {
                // skip the space
                ++pbuffer;
            }//if-else
            
            if(strncmp(pbuffer, "pet", 4) == 0) {
                action = PROCESSDOG_PET;
            } else if(strncmp(pbuffer, "exit", 5) == 0) {
                action = PROCESSDOG_EXIT;
            } else if(strncmp(pbuffer, "start", 6) == 0) {
                action = PROCESSDOG_START;
            } else {
                return -EFAULT;
            }//if-else
        }//plain block

        // print debug info: pid, action
        printk(KERN_DEBUG INFO_PREFIX "write() pid: %d, action: %d\n", pid, action);

        // process action
        switch(action) {
            case PROCESSDOG_PET:
                if(pid != processdog_pid) {
                    printk(KERN_ERR INFO_PREFIX "Busy with pid: %d\n", processdog_pid);
                    return -EBUSY;
                }//if
                processdog_pet();
                break;
            case PROCESSDOG_EXIT:
                if(pid != processdog_pid) {
                    printk(KERN_ERR INFO_PREFIX "Busy with pid: %d\n", processdog_pid);
                    return -EBUSY;
                }//if
                processdog_stop();
                processdog_pid = -1;
                break;
            case PROCESSDOG_START:
                if(processdog_pid != -1) {
                    printk(KERN_ERR INFO_PREFIX "Busy with pid: %d\n", processdog_pid);
                    return -EBUSY;
                }//if
                processdog_pid = pid;
                processdog_pet();
                break;
            default:
                return -EFAULT;
        }//switch
    }//if
    return len;
#   undef PROCESSDOG_PET
#   undef PROCESSDOG_EXIT
#   undef PROCESSDOG_START
}

static long processdog_ioctl(struct file * file, unsigned int cmd, unsigned long arg) {
    // @TODO
    return 0;
}

static int processdog_notify_sys(struct notifier_block *this, unsigned long code, void *unused) {
	if(code == SYS_DOWN || code == SYS_HALT) {
		processdog_stop();
        processdog_pid = -1;
    }//if
	return NOTIFY_DONE;
}

// --------------------------------------------------
// structs data

static const struct file_operations processdog_file_operations = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.write		= processdog_write,
	.unlocked_ioctl	= processdog_ioctl,
	.open		= processdog_open,
	.release	= processdog_release,
};

static struct miscdevice processdog_miscdev = {
	.minor		= WATCHDOG_MINOR,
	.name		= "processdog",
	.fops		= &processdog_file_operations,
};

static struct notifier_block processdog_notifier = {
	.notifier_call	= processdog_notify_sys,
};

// --------------------------------------------------
// init and exit

static int __init processdog_init(void) {
    int ret = 0;

	ret = register_reboot_notifier(&processdog_notifier);
	if(ret) {
		printk(KERN_ERR INFO_PREFIX "cannot register reboot notifier (err=%d)\n", ret);
		return ret;
	}//if

	ret = misc_register(&processdog_miscdev);
	if(ret) {
		printk(KERN_ERR INFO_PREFIX "cannot register miscdev on minor=%d (err=%d)\n",
                WATCHDOG_MINOR, ret);
		unregister_reboot_notifier(&processdog_notifier);
		return ret;
	}//if

    return 0;
}

static void __exit processdog_exit(void) {
	misc_deregister(&processdog_miscdev);
	unregister_reboot_notifier(&processdog_notifier);
}

module_init(processdog_init);
module_exit(processdog_exit);

// --------------------------------------------------
// info

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adam Basfop Cavendish");
MODULE_DESCRIPTION("This is a watchdog guarding one process");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);

