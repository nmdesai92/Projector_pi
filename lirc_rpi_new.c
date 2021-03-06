
/*
 *

 *	      (space-lengths) (just like the lirc_serial driver does)
 *	      between GPIO interrupt events on the Raspberry Pi.
 *	      Lots of code has been taken from the lirc_serial module,
 *	      so I would like say thanks to the authors.
 *
 * Copyright (C) 2012 Aron Robert Szabo <aron@reon.hu>,
 *		      Michael Bishop <cleverca22@gmail.com>
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/timekeeping.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <media/lirc.h>
#include <media/lirc_dev.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/platform_data/bcm2708.h>

#define LIRC_DRIVER_NAME "lirc_rpi_new"
#define RBUF_LEN 400
#define LIRC_TRANSMITTER_LATENCY 50

#ifndef MAX_UDELAY_MS
#define MAX_UDELAY_US 5000
#else
#define MAX_UDELAY_US (MAX_UDELAY_MS*1000)
#endif

#define dprintk(fmt, args...)					\
	do {							\
		if (debug)					\
			printk(KERN_DEBUG LIRC_DRIVER_NAME ": "	\
			       fmt, ## args);			\
	} while (0)

/* module parameters */

/* set the default GPIO input pin */
static int gpio_in_pin = 18;
/* set the default pull behaviour for input pin */
static int gpio_in_pull = BCM2708_PULL_DOWN;
/* set the default GPIO output pin */
static int gpio_out_pin = 17;
/* enable debugging messages */
static bool debug;
/* -1 = auto, 0 = active high, 1 = active low */
static int sense = -1;
/* use softcarrier by default */
static bool softcarrier = 1;
/* 0 = do not invert output, 1 = invert output */
static bool invert = 0;

struct gpio_chip *gpiochip;
static int irq_num;
static int auto_sense = 1;


/* forward declarations */
static long send_pulse(unsigned long length);
static void send_space(long length);
static void lirc_rpi_exit(void);
int irw(void); //Function to receive data
void irsend(void); //Function to send data


static struct platform_device *lirc_rpi_dev;
static struct timeval lasttv = { 0, 0 };
static struct lirc_buffer rbuf;
static spinlock_t lock;

/* initialized/set in init_timing_params() */
static unsigned int freq = 38000;
static unsigned int duty_cycle = 50;
static unsigned long period;
static unsigned long pulse_width;
static unsigned long space_width;


//Default byte code
unsigned long code_data[150]={
	     3502,    1744,     438,     439,     439,    1293,
              438,     439,     438,     439,     439,     438,
              440,     438,     439,     438,     440,     445,
              440,     437,     440,     437,     440,     437,
              440,     437,     439,     438,     440,    1290,
              440,     437,     440,     445,     440,     437,
              440,     437,     447,     429,     441,     437,
              447,     428,     443,     434,     442,     434,
              443,    1295,     442,     435,     442,     435,
              442,     436,     441,    1294,     436,     435,
              442,     435,     442,    1288,     441,     452,
              442,     436,     441,     436,     442,     435,
              442,     434,     443,     435,     442,     435,
              448,     429,     442,     443,     442,    1288,
              442,     435,     442,    1287,     442,    1289,
              441,    1289,     441,    1295,     434,     436,
              442,     449,     441,    1288,     442,     436,
              442,    1289,     441,     437,     441,    1289,
              447,    1284,     439,    1292,     440,    1293,
              441};
int pulse_count=0; //number of pulses

//array for translation of attribute values
static const char *names[] = {
#define OFF	0
	[OFF] = "off",
#define ON	1
	[ON] = "on",
};

//Driver data struct skeleton
struct lirc_drv_data{
struct device *dev;
int send;
int receive;
};


//send Function for attribute 'send'
static ssize_t send_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	printk("Attribute Send show\n");
	//Retreive the driver data pointer from the struct device
        struct lirc_drv_data *lirc_drv_data = dev_get_drvdata(dev);
        int len;
	//Error checking of attribute read
        len = sprintf(buf, "%s\n", names[lirc_drv_data->send]);
        if (len <= 0)
	{
		//Print out error in case of invalid value of the attribute
		printk("Invalid sprintf len: %d\n", len);
	}
	return len;
}

//store function for the attribute 'send'
static ssize_t send_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t valsize)
{
	char send_response[4];//Buffer to store the input attribute value
	printk("Atrribute send store\n");
	//Retreive the driver data pointer from the device struct
	struct lirc_drv_data *lirc_drv_data = dev_get_drvdata(dev);
	//Checking if the input data is valid
	if (sscanf(buf, "%4s", &send_response) != 1)
	{
		return -EINVAL;
	}
	if(pulse_count==0)
	pulse_count=115;
	//If the input value is 'on' change the attribute value to 1 and send pulse using irsend
	if(strcmp(send_response,"on") == 0)
	{
		lirc_drv_data->send=1;
		printk("file got value, calling irsend\n");
		irsend();
		lirc_drv_data->send=0;
	}
	//If the input value is 'off' change the attribute value to 0
	else
	{
		lirc_drv_data->send=0;
	}
	return valsize;
}

//Give necessary permissions for the send and store functions
static DEVICE_ATTR(send, S_IWUSR | S_IRUGO, send_show, send_store);

//Show function for the attribute 'receive'
static ssize_t receive_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	printk("Attribute receive show\n");
	//Retreive the driver data pointer from the struct device
        struct lirc_drv_data *lirc_drv_data = dev_get_drvdata(dev);
        int len;
	//Error checking of attribute read
        len = sprintf(buf, "%s\n", names[lirc_drv_data->receive]);
        if (len <= 0)
	{
		//Print out error in case of invalid value of the attribute
        	 printk("Invalid sprintf len: %d\n", len);
	}
	return len;
}

//Store function for the attribute 'receive'
static ssize_t receive_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t valsize)
{
	 char receive_response[4];//Buffer to store the input attribute value
         printk("Atrribute receive store\n");
	//Retreive the driver data pointer from the device struct
         struct lirc_drv_data *lirc_drv_data = dev_get_drvdata(dev);
	//Checking if the input data is valid
         if(sscanf(buf, "%4s", receive_response) != 1)
  	       return -EINVAL;
	//If the input value is 'on' change the attribute value to 1 and receive pulse using irw
         if(strcmp(receive_response,"on") == 0)
	{	
		pulse_count=0;
		lirc_drv_data->receive=1;
                printk("file got value,calling ir receive\n");
		irw();
		printk("Recepiton Done\n");
	}
	//If the input value is 'off' change the attribute value to 0
	else
	{
		lirc_drv_data->receive=0;
		printk("set_use_dec");
		/* GPIO Pin Falling/Rising Edge Detect Disable */
		irq_set_irq_type(irq_num, 0);
		disable_irq(irq_num);
		free_irq(irq_num, (void *) 0);
		printk(": freed IRQ %d\n", irq_num);
	}
  return valsize;
}

//Give necessary permissions for the send and store functions
static DEVICE_ATTR(receive, S_IWUSR | S_IRUGO, receive_show, receive_store);
/* 3 build groups of attributes */

static struct attribute *lirc_rpi_attrs[] = {
	&dev_attr_send.attr,
	&dev_attr_receive.attr,
	NULL
};

//Creating attribute group with the attributes for the driver
static struct attribute_group lirc_rpi_attr_group = {
	.attrs = lirc_rpi_attrs,
};

static void safe_udelay(unsigned long usecs)
{
	//printk("safe_udelay");
	while (usecs > MAX_UDELAY_US) {
		udelay(MAX_UDELAY_US);
		usecs -= MAX_UDELAY_US;
	}
	udelay(usecs);
}

static unsigned long read_current_us(void)
{
	//printk("read_current_us");
	struct timespec now;
	getnstimeofday(&now);
	return (now.tv_sec * 1000000) + (now.tv_nsec/1000);
}

static int init_timing_params(unsigned int new_duty_cycle,
	unsigned int new_freq)
{
	if (1000 * 1000000L / new_freq * new_duty_cycle / 100 <=
	    LIRC_TRANSMITTER_LATENCY)
		return -EINVAL;
	if (1000 * 1000000L / new_freq * (100 - new_duty_cycle) / 100 <=
	    LIRC_TRANSMITTER_LATENCY)
		return -EINVAL;
	duty_cycle = new_duty_cycle;
	freq = new_freq;
	period = 1000 * 1000000L / freq;
	pulse_width = period * duty_cycle / 100;
	space_width = period - pulse_width;
	printk("in init_timing_params, freq=%d pulse=%ld, "
		"space=%ld\n", freq, pulse_width, space_width);
	return 0;
}

static long send_pulse_softcarrier(unsigned long length)
{
	//printk("send_pulse_softcarrier");
	int flag;
	unsigned long actual, target;
	unsigned long actual_us, initial_us, target_us;

	length *= 1000;

	actual = 0; target = 0; flag = 0;
	actual_us = read_current_us();
	while (actual < length) {
		if (flag) {
			gpiochip->set(gpiochip, gpio_out_pin, invert);
			target += space_width;
		} else {
			gpiochip->set(gpiochip, gpio_out_pin, !invert);
			target += pulse_width;
		}
		initial_us = actual_us;
		target_us = actual_us + (target - actual) / 1000;
		/*
		 * Note - we've checked in ioctl that the pulse/space
		 * widths are big enough so that d is > 0
		 */
		if  ((int)(target_us - actual_us) > 0)
			udelay(target_us - actual_us);
		actual_us = read_current_us();
		actual += (actual_us - initial_us) * 1000;
		flag = !flag;
	}
	return (actual-length) / 1000;
}

static long send_pulse(unsigned long length)
{
	//printk("send_pulse");
	if (length <= 0)
		return 0;

	if (softcarrier) {
		return send_pulse_softcarrier(length);
	} else {
		gpiochip->set(gpiochip, gpio_out_pin, !invert);
		safe_udelay(length);
		return 0;
	}
}

static void send_space(long length)
{
	//printk("send_space");
	gpiochip->set(gpiochip, gpio_out_pin, invert);
	if (length <= 0)
		return;
	safe_udelay(length);
}

static void rbwrite(int l)
{
	//printk("rbwrite");
	if (lirc_buffer_full(&rbuf)) {
		/* no new signals will be accepted */
		dprintk("Buffer overrun\n");
		return;
	}
	lirc_buffer_write(&rbuf, (void *)&l);
}

static void frbwrite(int l)
{
	//printk("frbwrite");
	/* simple noise filter */
	static int pulse, space;
	static unsigned int ptr;

	if (ptr > 0 && (l & PULSE_BIT)) {
		pulse += l & PULSE_MASK;
		if (pulse > 250) {
			rbwrite(space);
			rbwrite(pulse | PULSE_BIT);
			ptr = 0;
			pulse = 0;
		}
		return;
	}
	if (!(l & PULSE_BIT)) {
		if (ptr == 0) {
			if (l > 20000) {
				space = l;
				ptr++;
				return;
			}
		} else {
			if (l > 20000) {
				space += pulse;
				if (space > PULSE_MASK)
					space = PULSE_MASK;
				space += l;
				if (space > PULSE_MASK)
					space = PULSE_MASK;
				pulse = 0;
				return;
			}
			rbwrite(space);
			rbwrite(pulse | PULSE_BIT);
			ptr = 0;
			pulse = 0;
		}
	}
	rbwrite(l);
}

static irqreturn_t irq_handler(int i, void *blah, struct pt_regs *regs)
{
	//printk("irq_handler");
	struct timeval tv;
	long deltv;
	int data;
	int signal;
	
	/* use the GPIO signal level */
	signal = gpiochip->get(gpiochip, gpio_in_pin);

	if (sense != -1) {
		/* get current time */
		do_gettimeofday(&tv);

		/* calc time since last interrupt in microseconds */
		deltv = tv.tv_sec-lasttv.tv_sec;
		if (tv.tv_sec < lasttv.tv_sec ||
		    (tv.tv_sec == lasttv.tv_sec &&
		     tv.tv_usec < lasttv.tv_usec)) {
			printk(KERN_WARNING LIRC_DRIVER_NAME
			       ": AIEEEE: your clock just jumped backwards\n");
			printk(KERN_WARNING LIRC_DRIVER_NAME
			       ": %d %d %lx %lx %lx %lx\n", signal, sense,
			       tv.tv_sec, lasttv.tv_sec,
			       tv.tv_usec, lasttv.tv_usec);
			data = PULSE_MASK;
		} else if (deltv > 15) {
			data = PULSE_MASK; /* really long time */
			if (!(signal^sense)) {
				/* sanity check */
				printk(KERN_DEBUG LIRC_DRIVER_NAME
				       ": AIEEEE: %d %d %lx %lx %lx %lx\n",
				       signal, sense, tv.tv_sec, lasttv.tv_sec,
				       tv.tv_usec, lasttv.tv_usec);
				/*
				 * detecting pulse while this
				 * MUST be a space!
				 */
				if (auto_sense) {
					sense = sense ? 0 : 1;
				}
			}
		} else {
			data = (int) (deltv*1000000 +
				      (tv.tv_usec - lasttv.tv_usec));
			code_data[++pulse_count]=data;
			//pr_info("data %d",data);
		}
		//printk("Pulse count is %d",pulse_count);
		frbwrite(signal^sense ? data : (data|PULSE_BIT));
		lasttv = tv;
		wake_up_interruptible(&rbuf.wait_poll);
	}
	return IRQ_HANDLED;
}

static int is_right_chip(struct gpio_chip *chip, void *data)
{
	dprintk("is_right_chip %s %d\n", chip->label, strcmp(data, chip->label));

	if (strcmp(data, chip->label) == 0)
		return 1;
	return 0;
}

static inline int read_bool_property(const struct device_node *np,
				     const char *propname,
				     bool *out_value)
{
	//printk("read_bool_property");
	u32 value = 0;
	int err = of_property_read_u32(np, propname, &value);
	if (err == 0)
		*out_value = (value != 0);
	return err;
}

static void read_pin_settings(struct device_node *node)
{
	//printk("read_pin_settings");
	u32 pin;
	int index;

	for (index = 0;
	     of_property_read_u32_index(
		     node,
		     "brcm,pins",
		     index,
		     &pin) == 0;
	     index++) {
		u32 function;
		int err;
		err = of_property_read_u32_index(
			node,
			"brcm,function",
			index,
			&function);
		if (err == 0) {
			if (function == 1) /* Output */
				gpio_out_pin = pin;
			else if (function == 0) /* Input */
				gpio_in_pin = pin;
		}
	}
}

static int init_port(void)
{
	//printk("init_port");
	int i, nlow, nhigh;
	struct device_node *node;

	node = lirc_rpi_dev->dev.of_node;

	gpiochip = gpiochip_find("bcm2708_gpio", is_right_chip);

	/*
	 * Because of the lack of a setpull function, only support
	 * pinctrl-bcm2835 if using device tree.
	*/
	if (!gpiochip && node)
		gpiochip = gpiochip_find("pinctrl-bcm2835", is_right_chip);

	if (!gpiochip) {
		pr_err(LIRC_DRIVER_NAME ": gpio chip not found!\n");
		return -ENODEV;
	}

	if (node) {
		struct device_node *pins_node;

		pins_node = of_parse_phandle(node, "pinctrl-0", 0);
		if (!pins_node) {
			printk(KERN_ERR LIRC_DRIVER_NAME
			       ": pinctrl settings not found!\n");
			return -EINVAL;
		}

		read_pin_settings(pins_node);

		of_property_read_u32(node, "rpi,sense", &sense);

		read_bool_property(node, "rpi,softcarrier", &softcarrier);

		read_bool_property(node, "rpi,invert", &invert);

		read_bool_property(node, "rpi,debug", &debug);

	} else {
		return -EINVAL;
	}

	gpiochip->set(gpiochip, gpio_out_pin, invert);

	irq_num = gpiochip->to_irq(gpiochip, gpio_in_pin);
	printk("to_irq %d\n", irq_num);

	/* if pin is high, then this must be an active low receiver. */
	if (sense == -1) {
		/* wait 1/2 sec for the power supply */
		msleep(500);

		/*
		 * probe 9 times every 0.04s, collect "votes" for
		 * active high/low
		 */
		nlow = 0;
		nhigh = 0;
		for (i = 0; i < 9; i++) {
			if (gpiochip->get(gpiochip, gpio_in_pin))
				nlow++;
			else
				nhigh++;
			msleep(40);
		}
		sense = (nlow >= nhigh ? 1 : 0);
		printk(KERN_INFO LIRC_DRIVER_NAME
		       ": auto-detected active %s receiver on GPIO pin %d\n",
		       sense ? "low" : "high", gpio_in_pin);
	} else {
		printk(KERN_INFO LIRC_DRIVER_NAME
		       ": manually using active %s receiver on GPIO pin %d\n",
		       sense ? "low" : "high", gpio_in_pin);
		auto_sense = 0;
	}

	return 0;
}

// called when the character device is opened
static int set_use_inc(void *data)
{
//	printk("set_use_inc");
	int result;

	/* initialize timestamp */
	do_gettimeofday(&lasttv);

	result = request_irq(irq_num,
			     (irq_handler_t) irq_handler,
			     IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING,
			     LIRC_DRIVER_NAME, (void*) 0);

	switch (result) {
	case -EBUSY:
		printk(KERN_ERR LIRC_DRIVER_NAME
		       ": IRQ %d is busy\n",
		       irq_num);
		return -EBUSY;
	case -EINVAL:
		printk(KERN_ERR LIRC_DRIVER_NAME
		       ": Bad irq number or handler\n");
		return -EINVAL;
	default:
		dprintk("Interrupt %d obtained\n",
			irq_num);
		break;
	};

	/* initialize pulse/space widths */
	init_timing_params(duty_cycle, freq);

	return 0;
}

static void set_use_dec(void *data)
{
//	printk("set_use_dec");
	/* GPIO Pin Falling/Rising Edge Detect Disable */
	irq_set_irq_type(irq_num, 0);
	disable_irq(irq_num);

	free_irq(irq_num, (void *) 0);

	dprintk(KERN_INFO LIRC_DRIVER_NAME
		": freed IRQ %d\n", irq_num);
}

static ssize_t lirc_write(struct file *file, const char *buf,
	size_t n, loff_t *ppos)
{
	printk("lirc_write");
	int i, count;
	unsigned long flags;
	long delta = 0;
	int *wbuf;

	count = n / sizeof(int);
	if (n % sizeof(int) || count % 2 == 0)
		return -EINVAL;
	wbuf = memdup_user(buf, n);
	if (IS_ERR(wbuf))
		return PTR_ERR(wbuf);
	spin_lock_irqsave(&lock, flags);

	for (i = 0; i < count; i++) {
		if (i%2)
			send_space(wbuf[i] - delta);
		else
			delta = send_pulse(wbuf[i]);
	}
	gpiochip->set(gpiochip, gpio_out_pin, invert);

	spin_unlock_irqrestore(&lock, flags);
	kfree(wbuf);
	return n;
}

static long lirc_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
//	printk("lirc_ioctl");
//	printk("command %d",cmd);
//	printk("argument %ld",arg);

	int result;
	__u32 value;
	printk("where\n");
	switch (cmd) {
	case LIRC_GET_SEND_MODE:
		return -ENOIOCTLCMD;
		break;

	case LIRC_SET_SEND_MODE:
		result = get_user(value, (__u32 *) arg);
		if (result)
			return result;
		/* only LIRC_MODE_PULSE supported */
		if (value != LIRC_MODE_PULSE)
			return -ENOSYS;
		break;

	case LIRC_GET_LENGTH:
		return -ENOSYS;
		break;

	case LIRC_SET_SEND_DUTY_CYCLE:
		dprintk("SET_SEND_DUTY_CYCLE\n");
		result = get_user(value, (__u32 *) arg);
		if (result)
			return result;
		if (value <= 0 || value > 100)
			return -EINVAL;
		return init_timing_params(value, freq);
		break;

	case LIRC_SET_SEND_CARRIER:
		dprintk("SET_SEND_CARRIER\n");
		result = get_user(value, (__u32 *) arg);
		if (result)
			return result;
		if (value > 500000 || value < 20000)
			return -EINVAL;
		return init_timing_params(duty_cycle, value);
		break;

	default:
	printk("default");
		return lirc_dev_fop_ioctl(filep, cmd, arg);
	}
	printk("exit lirc_ioctl");
	return 0;
}

static const struct file_operations lirc_fops = {
	.owner		= THIS_MODULE,
	.write		= lirc_write,
	.unlocked_ioctl	= lirc_ioctl,
	.read		= lirc_dev_fop_read,
	.poll		= lirc_dev_fop_poll,
	.open		= lirc_dev_fop_open,
	.release	= lirc_dev_fop_close,
	.llseek		= no_llseek,
};

static struct lirc_driver driver = {
	.name		= LIRC_DRIVER_NAME,
	.minor		= -1,
	.code_length	= 1,
	.sample_rate	= 0,
	.data		= NULL,
	.add_to_buf	= NULL,
	.rbuf		= &rbuf,
	.set_use_inc	= set_use_inc,
	.set_use_dec	= set_use_dec,
	.fops		= &lirc_fops,
	.dev		= NULL,
	.owner		= THIS_MODULE,
};


static int lirc_rpi_probe(struct platform_device *pdev)
{
	struct lirc_drv_data *lirc_drv_data;
	int ret;
//	pr_info("lirc_rpi: probing %s id %d\n", pdev->name, pdev->id);

	lirc_drv_data = devm_kzalloc(&pdev->dev, sizeof(*lirc_drv_data), GFP_KERNEL);
    lirc_drv_data->dev = &pdev->dev;
    platform_set_drvdata(pdev, lirc_drv_data);

        ret = sysfs_create_group(&pdev->dev.kobj, &lirc_rpi_attr_group);
        if (ret) {
	pr_info("sysfs creation fialed");
        dev_err(&pdev->dev, "sysfs creation failed\n");
        return ret;
        }
	pr_info("Attribute created");
	return 0;
}

static int lirc_rpi_remove(struct platform_device *pdev)
{
	pr_info("lirc_rpi: removing %s id %d\n", pdev->name, pdev->id);
	sysfs_remove_group(&pdev->dev.kobj, &lirc_rpi_attr_group);
	return 0;
}

static const struct of_device_id lirc_rpi_of_match[] = {
	{ .compatible = "rpi,lirc-rpi", },
	{},
};
MODULE_DEVICE_TABLE(of, lirc_rpi_of_match);

static struct platform_driver lirc_rpi_driver = {
	.probe=lirc_rpi_probe,
	.remove=lirc_rpi_remove,
	.driver = {
		.name   = LIRC_DRIVER_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(lirc_rpi_of_match),
	},
};

static int __init lirc_rpi_init(void)
{
//	printk("lirc_rpi_init");
	struct device_node *node;
	int result;

	/* Init read buffer. */
	result = lirc_buffer_init(&rbuf, sizeof(int), RBUF_LEN);
	if (result < 0)
		return -ENOMEM;

	result = platform_driver_register(&lirc_rpi_driver);
	if (result) {
		printk(KERN_ERR LIRC_DRIVER_NAME
		       ": lirc register returned %d\n", result);
		goto exit_buffer_free;
	}

	node = of_find_compatible_node(NULL, NULL,
				       lirc_rpi_of_match[0].compatible);

	if (node) {
		/* DT-enabled */
		printk("Device Registration from node");
		lirc_rpi_dev = of_find_device_by_node(node);
		WARN_ON(lirc_rpi_dev->dev.of_node != node);
		of_node_put(node);
	}
	else {
		lirc_rpi_dev = platform_device_alloc(LIRC_DRIVER_NAME, 0);
		if (!lirc_rpi_dev) {
			result = -ENOMEM;
			goto exit_driver_unregister;
		}

		result = platform_device_add(lirc_rpi_dev);
		if (result)
			goto exit_device_put;
	}
	return 0;

	exit_device_put:
	platform_device_put(lirc_rpi_dev);

	exit_driver_unregister:
	platform_driver_unregister(&lirc_rpi_driver);

	exit_buffer_free:
	lirc_buffer_free(&rbuf);
	return result;
}

static void lirc_rpi_exit(void)
{
//	printk("lirc_rpi_exit");
	if (!lirc_rpi_dev->dev.of_node)
		platform_device_unregister(lirc_rpi_dev);
	platform_driver_unregister(&lirc_rpi_driver);
	lirc_buffer_free(&rbuf);
}

//Used to send pulses using the IR emitter
void irsend(void){
	int i;
	unsigned long flags;
	long delta = 0;

	printk("%d",pulse_count);

	spin_lock_irqsave(&lock, flags);

	for (i = 0; i < pulse_count; i++) {
		if (i%2)
			send_space(code_data[i] - delta);
		else
			delta = send_pulse(code_data[i]);
	}
	gpiochip->set(gpiochip, gpio_out_pin, invert);

	spin_unlock_irqrestore(&lock, flags);
	printk("Transmission done\n");
}

//Used to receive pulses using the IR receiver
int irw(void)
{
	int result;
	/* initialize timestamp */
	do_gettimeofday(&lasttv);
	result = request_irq(irq_num,
			     (irq_handler_t) irq_handler,
			     IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING,
			     LIRC_DRIVER_NAME, (void*) 0);
	switch (result)
	{
		case -EBUSY:
			printk(KERN_ERR LIRC_DRIVER_NAME
			       ": IRQ %d is busy\n",
			       irq_num);
			return -EBUSY;
		case -EINVAL:
			printk(KERN_ERR LIRC_DRIVER_NAME
			       ": Bad irq number or handler\n");
			return -EINVAL;
		default:
			dprintk("Interrupt %d obtained\n",
				irq_num);
			break;
	};
	/* initialize pulse/space widths */
	init_timing_params(duty_cycle, freq);
	return 0;
}

static int __init lirc_rpi_init_module(void)
{
	printk("lirc_rpi_init_module");
	int result;

	result = lirc_rpi_init();
	if (result)
		return result;

	result = init_port();
	if (result < 0)
		goto exit_rpi;

	driver.features = LIRC_CAN_SET_SEND_DUTY_CYCLE |
			  LIRC_CAN_SET_SEND_CARRIER |
			  LIRC_CAN_SEND_PULSE |
			  LIRC_CAN_REC_MODE2;

	driver.dev = &lirc_rpi_dev->dev;
	driver.minor = lirc_register_driver(&driver);

	if (driver.minor < 0) {
		printk(KERN_ERR LIRC_DRIVER_NAME
		       ": device registration failed with %d\n", result);
		result = -EIO;
		goto exit_rpi;
	}

	printk(KERN_INFO LIRC_DRIVER_NAME ": driver registered!\n");

	return 0;

	exit_rpi:
	lirc_rpi_exit();

	return result;
}

static void __exit lirc_rpi_exit_module(void)
{
	printk("lirc_rpi_exit_module");
	lirc_unregister_driver(driver.minor);
	int i=0;
	for(i=0;i<100;i++)
	printk("code data is %d",code_data[i]);
//	set_use_dec();
	printk("set_use_dec is called");
	gpio_free(gpio_out_pin);
	gpio_free(gpio_in_pin);

	lirc_rpi_exit();

	printk(KERN_INFO LIRC_DRIVER_NAME ": cleaned up module\n");
}

module_init(lirc_rpi_init_module);
module_exit(lirc_rpi_exit_module);

MODULE_DESCRIPTION("Infra-red receiver and blaster driver for Raspberry Pi GPIO.");
MODULE_AUTHOR("Aron Robert Szabo <aron@reon.hu>");
MODULE_AUTHOR("Michael Bishop <cleverca22@gmail.com>");
MODULE_LICENSE("GPL");

module_param(gpio_out_pin, int, S_IRUGO);
MODULE_PARM_DESC(gpio_out_pin, "GPIO output/transmitter pin number of the BCM"
		 " processor. (default 17");

module_param(gpio_in_pin, int, S_IRUGO);
MODULE_PARM_DESC(gpio_in_pin, "GPIO input pin number of the BCM processor."
		 " (default 18");

module_param(gpio_in_pull, int, S_IRUGO);
MODULE_PARM_DESC(gpio_in_pull, "GPIO input pin pull configuration."
		 " (0 = off, 1 = up, 2 = down, default down)");

module_param(sense, int, S_IRUGO);
MODULE_PARM_DESC(sense, "Override autodetection of IR receiver circuit"
		 " (0 = active high, 1 = active low )");

module_param(softcarrier, bool, S_IRUGO);
MODULE_PARM_DESC(softcarrier, "Software carrier (0 = off, 1 = on, default on)");

module_param(invert, bool, S_IRUGO);
MODULE_PARM_DESC(invert, "Invert output (0 = off, 1 = on, default off");

module_param(debug, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Enable debugging messages");
