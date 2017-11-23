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

static unsigned int interval = 10; 	//Timer runs for every 10 seconds

static struct timer_list blinktimer;
static struct miscdevice gpiohdmi_dev={
.name="gpiohdmi"
};

static int status_pre;

#define secs_to_jiffies(i)	(msecs_to_jiffies((i)*1000))

static void blink_timeout(unsigned long dummy)
{
        int status_now;
	status_now = gpio_get_value(gpiohdmi); 
        if(status_now == 1 && status_pre == 0)
	{
                printk("HDMI disonnected\n");
		kobject_uevent(&gpiohdmi_dev.this_device->kobj,KOBJ_REMOVE);
		//misc_deregister(&gpiohdmi_dev);
		//printk("Misc device is removed");
	}
	
        status_pre = status_now;
	mod_timer(&blinktimer, jiffies+secs_to_jiffies(interval));
}
 
static int __init hdmigpio_init(void){
   int err;
   printk(KERN_INFO "GPIO_TEST: Initializing the GPIO_TEST LKM\n");
   // Is the GPIO a valid GPIO number 
   if (!gpio_is_valid(gpiohdmi)){
      printk(KERN_INFO "GPIO_TEST: invalid LED GPIO\n");
      return -ENODEV;
   }
   err= misc_register(&gpiohdmi_dev);
   setup_timer(&blinktimer, blink_timeout, 0);
   err = mod_timer(&blinktimer, secs_to_jiffies(interval));
   if (unlikely(err)) {
	pr_err("bln: timer error %d...\n", err);
	del_timer(&blinktimer);
        gpio_unexport(gpiohdmi);                  // Unexport the LED GPIO
        gpio_free(gpiohdmi);                      // Free the LED GPIO
        return err;
	}
   gpio_request(gpiohdmi, "sysfs");         // gpiohdmi is hardcoded to 46, request it
   gpio_direction_input(gpiohdmi);	    // Set the gpio to be in input mode
   gpio_export(gpiohdmi, false);             // Causes gpio49 to appear in /sys/class/gpio
                     // the bool argument prevents the direction from being changed
   // Perform a quick test to see that the button is working as expected on LKM load
   printk(KERN_INFO "GPIO_TEST: The button state is currently: %d\n", gpio_get_value(gpiohdmi));
 
   return 0;
}
 
/**  Used to release the GPIOs and display cleanup messages.*/

static void __exit hdmigpio_exit(void){
   int err;
   err = del_timer(&blinktimer);
   if (err)
        pr_err("bln: timer delete error %d...\n", err);
   misc_deregister(&gpiohdmi_dev);

   gpio_unexport(gpiohdmi);                  // Unexport the LED GPIO
   gpio_free(gpiohdmi);                      // Free the LED GPIO
   err = del_timer(&blinktimer);
   if (err)
	pr_err("bln: timer delete error %d...\n", err);

   printk(KERN_INFO "GPIO_TEST: Goodbye from the LKM!\n");
}

 
/// This next calls are  mandatory -- they identify the initialization function
/// and the cleanup function (as above).
module_init(hdmigpio_init);
module_exit(hdmigpio_exit);
