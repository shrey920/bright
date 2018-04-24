#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/hid.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>


#define DRIVER_VERSION "v0.1"
#define DRIVER_AUTHOR "IT2020"
#define DRIVER_DESC "Brightness driver"
#define DRIVER_LICENSE "GPL"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

#define DEVICE_NAME "bright"
#define MAX_SCREEN 100
#define MIN_SCREEN 0

int Major;


struct file* file_open(const char* path, int flags, int rights)
{
	struct file* filp = NULL;
	mm_segment_t oldfs;
	int err = 0;
	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(path, flags, rights);
	set_fs(oldfs);
	if (IS_ERR(filp))
	{
		err = PTR_ERR(filp);
		return NULL;
	}
	return filp;
}
int file_read(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;
	oldfs = get_fs();
	set_fs(get_ds());
	ret = vfs_read(file, data, size, &offset);
	set_fs(oldfs);
	return ret;
}
int file_write(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;
	oldfs = get_fs();
	set_fs(get_ds());
	ret = vfs_write(file, data, size, &offset);
	set_fs(oldfs);
	return ret;
}
void file_close(struct file* file)
{
	filp_close(file, NULL);
}

struct bright_device {
    signed char data[4];     /* use a 4-byte protocol */
    struct input_dev *idev;   /* input device, to push out input  data */
    int x, y;                /* keep track of the position of this device */
};

static struct bright_device *bright;

int bright_open(struct inode *inode, struct file *filp)
{

    printk(KERN_INFO "bright: faking an USB via the misc device\n");
    return 0; /* Ok */
}    

int bright_release(struct inode *inode, struct file *filp)
{
   printk(KERN_INFO "bright: closing misc device\n");
    return 0;
}

ssize_t bright_read(struct file *filp,   /* see include/linux/fs.h   */
                           char *buffer,        /* buffer to fill with data */
                           size_t length,       /* length of the buffer     */
                           loff_t * offset)
{
    printk(KERN_INFO "bright: Restricted. That is okay but do not repeat this mistake.\n");
    return 0;
}

static ssize_t bright_write(struct file *filp, const char *buf, size_t count,
		    loff_t *offp)
{
    static char localbuf[16];
    int i, command = -1;

    if (count >16) count=16;
    copy_from_user(localbuf, buf, count);

	struct input_dev *dev = bright->idev;


	switch(localbuf[0]){

	    case 'b':{
            static char brightness_buff[4];
	        struct file *filehandle1 = file_open("/sys/class/backlight/intel_backlight/brightness", 0, 0);
	        file_read((struct file*)filehandle1, 0, brightness_buff, 4);
	        printk("%s",brightness_buff);

	        int brightness =0;
            for(i=0; i<4; i++){
                brightness = brightness * 10 + ( brightness_buff[i] - '0' );
            }
            printk("%d",brightness);
            if(brightness<7000)
            brightness+=500;

            brightness_buff[0] = brightness/1000 + '0';
            brightness%=1000;
            brightness_buff[1] = brightness/100 + '0';
            brightness%=100;
            brightness_buff[2] = brightness/10 + '0';
            brightness_buff[3] = brightness%10 + '0';
	        printk("bb:%s",brightness_buff);

        	file_close((struct file*)filehandle1);
            filehandle1 = file_open("/sys/class/backlight/intel_backlight/brightness", 1, 0);
            file_write((struct file*)filehandle1, 0, brightness_buff, 4);
            file_close((struct file*)filehandle1);
            break;

	    }
	    case 'v':{
            static char brightness_buff[4];
	        struct file *filehandle1 = file_open("/sys/class/backlight/intel_backlight/brightness", 0, 0);
	        file_read((struct file*)filehandle1, 0, brightness_buff, 4);
	        printk("%s",brightness_buff);

	        int brightness =0;
            for(i=0; i<4; i++){
                brightness = brightness * 10 + ( brightness_buff[i] - '0' );
            }
            if(brightness>600);
            brightness-=500;
            printk("%d",brightness);

            brightness_buff[0] = brightness/1000 + '0';
            brightness%=1000;
            brightness_buff[1] = brightness/100 + '0';
            brightness%=100;
            brightness_buff[2] = brightness/10 + '0';
            brightness_buff[3] = brightness%10 + '0';
	        printk("%s",brightness_buff);

        	file_close((struct file*)filehandle1);
            filehandle1 = file_open("/sys/class/backlight/intel_backlight/brightness", 1, 0);
            file_write((struct file*)filehandle1, 0, brightness_buff, 4);
            file_close((struct file*)filehandle1);
            break;

	    }
	}
    


	input_sync(dev);

	
    return count;
}


static struct file_operations bright_fops = {
	write:    bright_write,
	read:     bright_read,
	open:     bright_open,
	release:  bright_release,
};


int init_module(void)
{
    int retval;
	
	Major = register_chrdev(0, DEVICE_NAME, &bright_fops);
	if (Major < 0) {
	  printk(KERN_ALERT "Registering char device failed with %d\n", Major);
	  return Major;
	}
	

	struct input_dev *input_dev;

    /* allocate and zero a new data structure for the new device */
    bright = kmalloc(sizeof(struct bright_device), GFP_KERNEL);
    if (!bright) return -ENOMEM; /* failure */
    memset(bright, 0, sizeof(*bright));

	input_dev = input_allocate_device();
	if (!input_dev) {
		printk(KERN_ERR "bright.c: Not enough memory\n");
		retval = -ENOMEM;
	}
	//updating struct
	bright->idev = input_dev;
//
//
	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);

	input_dev->name = DEVICE_NAME;	
	input_set_drvdata(input_dev, bright);
	
	retval = input_register_device(input_dev);
	if (retval) {
		printk(KERN_ERR "bright: Failed to register device\n");
		goto err_free_dev;
	}

	
	printk(KERN_INFO "I was assigned major number %d. To talk to\n", Major);
	printk(KERN_INFO "the driver, create a dev file with\n");   
    printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
	printk(KERN_INFO "the device file.\n");
	printk(KERN_INFO "Remove the device file and module when done.\n");
	
	
return 0;

err_free_dev:
	input_free_device(bright->idev);
	kfree(bright);

return retval;
}

void cleanup_module(void)
{
    /*
    * Unregister the device
    */
	if(!bright) return;
	
	input_unregister_device(bright->idev);
	kfree(bright);
	unregister_chrdev(Major, DEVICE_NAME);
    
	printk(KERN_ALERT "Uninstalled. Delete device from dev.");


}