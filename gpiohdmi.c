/*
A Loadable Linux Kernel Module (LKM) to monitor the status of HDMI connection
on  raspberry pi.This  polls the GPIO pin 46 in raspberry pi
and generate an uevent in case of a HDMI cable is disconnected.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/gpio.h>                 // Required for the GPIO functions
#include <linux/timer.h>
#include <linux/miscdevice.h>
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bhoomi Team");
MODULE_DESCRIPTION("Hdmi Status");
MODULE_VERSION("0.1");
 
static unsigned int gpiohdmi = 46;

static unsigned int interval = 1; 	//Timer runs for every 1 second

static struct timer_list checktimer;

static struct miscdevice gpiohdmi_dev={
.name="gpiohdmi"
};

static int status_pre;

#define secs_to_jiffies(i)	(msecs_to_jiffies((i)*1000))


/*Timer Callback*/
static void check_timeout(unsigned long dummy)
{
        int status_now;
	status_now = gpio_get_value(gpiohdmi); 				//Get the value from GPIO 46
        if(status_now == 1 && status_pre == 0)
	{
                printk("HDMI disonnected\n");
		kobject_uevent(&gpiohdmi_dev.this_device->kobj,KOBJ_REMOVE);	//Notify the user by sending uevent
	}
	
        status_pre = status_now;
	mod_timer(&checktimer, jiffies+secs_to_jiffies(interval));	//keep calling the timer 
}


/*Initialize the Module*/ 
static int __init hdmigpio_init(void){
   int err;
   printk(KERN_INFO "hdmi_status: Initializing the HDMI_GPIO LKM\n");
   // Is the GPIO a valid GPIO number 
   if (!gpio_is_valid(gpiohdmi)){
      printk(KERN_INFO "hdmi_status: invalid GPIO PIN\n");
      return -ENODEV;
   }
   err= misc_register(&gpiohdmi_dev);					//register as a miscellaneous device
   setup_timer(&checktimer, check_timeout, 0);
   err = mod_timer(&checktimer, secs_to_jiffies(interval));
   if (unlikely(err)) {
	pr_err("hdmi_status: timer error %d...\n", err);
	del_timer(&checktimer);
        gpio_unexport(gpiohdmi);                  // Unexport the LED GPIO
        gpio_free(gpiohdmi);                      // Free the LED GPIO
        return err;
	}
   gpio_request(gpiohdmi, "sysfs");         // gpiohdmi is hardcoded to 46, request it
   gpio_direction_input(gpiohdmi);	    // Set the gpio to be in input mode
   gpio_export(gpiohdmi, false);             // Causes gpio46 to appear in /sys/class/gpio
                    
   printk(KERN_INFO "Hdmi_status: The pin state is currently: %d\n", gpio_get_value(gpiohdmi));
 
   return 0;
}
 
/**  Used to release the GPIOs and display cleanup messages.*/

static void __exit hdmigpio_exit(void){
   int err;
   err = del_timer(&checkktimer);
   if (err)
        pr_err("hdmi_status: timer delete error %d...\n", err);
   misc_deregister(&gpiohdmi_dev);

   gpio_unexport(gpiohdmi);                  // Unexport the LED GPIO
   gpio_free(gpiohdmi);                      // Free the LED GPIO
   err = del_timer(&checkktimer);
   if (err)
	pr_err("hdmi_status: timer delete error %d...\n", err);

   printk(KERN_INFO "hdmi_status: Goodbye from the LKM!\n");
}

 
module_init(hdmigpio_init);
module_exit(hdmigpio_exit);
