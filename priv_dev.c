#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#include <linux/sched.h>
#include <linux/cred.h>

MODULE_LICENSE("GPL");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Roman Ponomarenko <r.e.p@yandex.ru>");

#define MY_MODULE_NAME "priv_dev"
#define DEVICE_NAME "my_super_device"
#define MODULE_NAME "priv_dev_module"
#define DEVICE_FIRST 0
#define DEVICE_COUNT 1
#define CLASS_NAME "priv_dev_class"

static ssize_t device_write(struct file *filp, const char __user *buffer,
				size_t size, loff_t *offset);

static const struct file_operations hello_fops = {
.owner = THIS_MODULE,
.write = device_write,
};

static dev_t dev;
static struct cdev hcdev;
static struct class *devclass;

static int __init hello_init(void)
{
	int ret = alloc_chrdev_region(&dev, DEVICE_FIRST, DEVICE_COUNT,
					MODULE_NAME);
	if (ret < 0) {
		pr_err("[%s] Registering char device failed with %d\n"
				, MY_MODULE_NAME, MAJOR(dev));
		return ret;
	}
	cdev_init(&hcdev, &hello_fops);
	hcdev.owner = THIS_MODULE;
	ret = cdev_add(&hcdev, dev, DEVICE_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(dev, DEVICE_COUNT);
		pr_err("[%s] Can not add char device\n", MY_MODULE_NAME);
		return ret;
	}
	devclass = class_create(THIS_MODULE, CLASS_NAME);
	device_create(devclass, NULL, dev, "%s", DEVICE_NAME);
	pr_info("[%s] init %d:%d\n", MY_MODULE_NAME, MAJOR(dev), MINOR(dev));
	return 0;
}

static void __exit hello_exit(void)
{
	device_destroy(devclass, dev);
	class_destroy(devclass);
	cdev_del(&hcdev);
	unregister_chrdev_region(dev, DEVICE_COUNT);
	pr_info("[%s] exit\n", MY_MODULE_NAME);
}

static ssize_t device_write(struct file *filp, const char __user *buffer,
				size_t size, loff_t *offset)
{
	long pid;
	char *kbuffer;
	struct task_struct *p;

	kbuffer = kmalloc(size+1, GFP_KERNEL);
	pr_info("[%s] write %ld bytes", MY_MODULE_NAME, size);

	copy_from_user(kbuffer, buffer, size);
	kbuffer[size] = 0;
	kstrtol(kbuffer, 10, &pid);

	pr_info("[%s] pid: %ld str: %s\n", MY_MODULE_NAME, pid, kbuffer);
	p = pid_task(find_vpid(pid), PIDTYPE_PID);
	if (p != NULL) {
		struct cred *new = prepare_kernel_cred(p);
		const struct cred *old = p->cred;

		pr_info("[%s] I found it!\n", MY_MODULE_NAME);
		pr_info("[%s] uid: %d euid: %d suid %d fsuid %d\n",
				MY_MODULE_NAME,
				p->cred->uid.val, p->cred->euid.val,
				p->cred->suid.val, p->cred->fsuid.val);
		if (!new) {
			pr_err("[%s] get_task_cred failed\n", MY_MODULE_NAME);
			return -ENOMEM;
		}
		new->fsuid.val = 0;
		rcu_assign_pointer(p->cred, new);
		put_cred(old);
	} else
		pr_err("[%s] I lost it :-(\n", MY_MODULE_NAME);
	kfree(kbuffer);
	return size;
}

module_init(hello_init);
module_exit(hello_exit);
