/*
 * Linux device module for LED connected to GPIO 4
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>
#include <linux/gpio.h>
#include <linux/timer.h>

#define GPIO_PIN 46

static unsigned int interval = 1;
static struct timer_list blinktimer;
static struct platform_device *hdmi_state_dev;
int status_pre;

#define secs_to_jiffies(i)      (msecs_to_jiffies((i)*1000))



/* 1 define device specific data */

struct digi_dev_data {
	char *name;
	//char *status;
};

#define to_digi_dev_data(p)	((struct digi_dev_data *)((p)->platform_data))

/* 2 define show and store ops for each attribute */

static ssize_t name_show(struct device *dev, struct device_attribute *attr, char *resp)
{
	struct platform_device *pdev = to_platform_device(dev);

	return snprintf(resp, 40, "%s\n", pdev->name);
}

static DEVICE_ATTR_RO(name);

/*static ssize_t status_show(struct device *dev, struct device_attribute *attr, char *resp)
{
        struct platform_device *pdev = to_platform_device(dev);
		
        return snprintf(resp, 8, "%s\n", pdev->status);
}

static DEVICE_ATTR_RO(status);*/


/* 3 build groups of attributes */

static struct attribute *digi_basic_attrs[] = {
	&dev_attr_name.attr,
	//&dev_attr_status.attr,
	NULL
};

static struct attribute_group digi_basic_group = {
	.attrs = digi_basic_attrs,
	NULL
};
static const struct attribute_group *digi_all_attributes[] = {
 	&digi_basic_group,
 	NULL
};


/* 5 define device specific ops. release is a must */

static void digi_release(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	dev_alert(dev, "releasing gpio device %s\n", pdev->name);
}


static void blink_timeout(unsigned long dummy)
{

        int status_now = gpio_get_value(GPIO_PIN); 
        if(status_now == 1 && status_pre == 0){
                printk("HDMI disonnected");
		kobject_uevent(&hdmi_state_dev->dev.kobj,KOBJ_REMOVE);
	}
	
        status_pre = status_now;
        mod_timer(&blinktimer, jiffies+secs_to_jiffies(interval));
}


/* 6 define register and unregister ops */

static struct platform_device hdmi_state = {
                .name = "hdmi_state",
		//.status = "unknown", 
                .dev = {
                        .release = digi_release,
                        .groups  = digi_all_attributes,
                },
};


static int digi_register_device(struct platform_device *pdev)
{
	int err;
	pr_info("hdmi module: registering device %s\n", pdev->name);
	err = platform_device_register(pdev);
	hdmi_state_dev = pdev;
	if (err)
		pr_err("hdmi module: device %s failed to register, error %d\n", pdev->name, err);
	
	setup_timer(&blinktimer, blink_timeout, 0);
        err = mod_timer(&blinktimer, secs_to_jiffies(interval));
        if (unlikely(err)) {
                pr_err("hdmi module: timer error %d...\n", err);
                del_timer(&blinktimer);
                return err;
        }

	return err;
}

static void digi_unregister_device(struct platform_device *pdev)
{
	dev_alert(&pdev->dev, "hdmi module: unregistering hdmi device %s\n", pdev->name);
	platform_device_unregister(pdev);
}

/* 7 define devices. These could also be loaded from device trees */


static int __init digiout_load(void)
{
	pr_info("hdmi module: loading with pin GPIO_PIN\n");
	gpio_direction_input(GPIO_PIN);
	return digi_register_device(&hdmi_state);

}
module_init(digiout_load);

static void __exit digiout_unload(void)
{	
	int err;
	pr_info("hdmi module: unloading\n");
	err = del_timer(&blinktimer);
        if (err)
                pr_err("hdmi module: timer delete error %d...\n", err);

	digi_unregister_device(&hdmi_state);
}

module_exit(digiout_unload);

MODULE_DESCRIPTION("HDMI Detection");
MODULE_LICENSE("GPL");
