#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

#define DEMO_MAJOR	55
#define DEMO_MINOR	0
#define DEMO_COUNT	1
#define BUF_SIZE	100

dev_t dev = 0;
u32 demo_major = 0;
u32 demo_minor = 0;

struct class *dev_class = NULL;

struct demo_cdev{
	struct cdev cdev;
	struct device *dev_device;
	u8 my_value_test;
	
	char kbuf[BUF_SIZE];
};
 
struct demo_cdev *demo_cdevp = NULL;
 
int demo_open(struct inode* inode, struct file *filp)
{
	struct demo_cdev *pcdevp = NULL;
	printk("enter demo_open!\n");
	
	pcdevp = container_of(inode->i_cdev, struct demo_cdev, cdev);
	printk("my_value_test = %d\n", pcdevp->my_value_test);
	
	filp->private_data = pcdevp;
	
	return 0;
}
 
int demo_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
	int ret = 0;
	
	struct demo_cdev *cdevp = filp->private_data;
	printk("enter demo_read!\n");
	ret = copy_to_user(buf, cdevp->kbuf, count);
	printk("kernel kbuf content:%s\n", cdevp->kbuf);
	return ret;
}
 
int demo_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
	int ret = 0;
	struct demo_cdev *cdevp = filp->private_data;	
	printk("enter demo_write!\n");
	
	ret = copy_from_user(cdevp->kbuf, buf, count);
	
	return ret;
}
 
int demo_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long data)
{
	printk("enter demo_ioctl!\n");
	return 0;
}
 
int demo_release(struct inode *inode, struct file *filp)
{
	printk("enter demo_release!\n");
	return 0;
}
 
struct file_operations demo_fops = {
	.owner = THIS_MODULE,
	.open = demo_open,
	.read = demo_read,
	.write = demo_write,
	.ioctl = demo_ioctl,
	.release = demo_release,
	};
 
int __init demo_init(void)
{
	int ret = 0;
	int i = 0;
	
	if(demo_major){
		dev = MKDEV(DEMO_MAJOR, DEMO_MINOR);//生成设备号
		//注册设备号;1、要注册的起始设备号2、连续注册的设备号个数3、名字
		ret = register_chrdev_region(dev, DEMO_COUNT, "cdd_demo");
	}else{
		// 动态分配设备号
		ret = alloc_chrdev_region(&dev, demo_minor, DEMO_COUNT, "cdd_demo02");
	}
	
	if(ret < 0){
		printk("register_chrdev_region failed!\n");
		goto failure_register_chrdev;
	}
	//获取主设备号
	demo_major = MAJOR(dev);
	printk("demo_major = %d\n", demo_major);
	
	demo_cdevp = kzalloc(sizeof(struct demo_cdev)*DEMO_COUNT, GFP_KERNEL);
	if(IS_ERR(demo_cdevp)){
		printk("kzalloc failed!\n");
		goto failure_kzalloc;
	}
	/*创建设备类*/
	dev_class = class_create(THIS_MODULE, "cdd_class");
	if(IS_ERR(dev_class)){
		printk("class_create failed!\n");
		goto failure_dev_class;
	}
	for(i=0; i<DEMO_COUNT; i++){
		/*初始化cdev*/
		cdev_init(&(demo_cdevp[i].cdev), &demo_fops);
		/*添加cdev到内核*/
		cdev_add(&(demo_cdevp[i].cdev), dev+i, 1);
		/* “/dev/xxx” */
		device_create(dev_class, NULL, dev+i, NULL, "cdd%d", i);
		
		demo_cdevp[i].my_value_test = i;
		
	}
	
	return 0;
failure_dev_class:
	kfree(demo_cdevp);
failure_kzalloc:
	unregister_chrdev_region(dev, DEMO_COUNT);
failure_register_chrdev:
	return ret;
}
 
void __exit demo_exit(void)
{
/*逆序消除*/
	int i = 0;
	for(; i < DEMO_COUNT; i++){
		device_destroy(dev_class, dev+i);
		cdev_del(&(demo_cdevp[i].cdev));
		//cdev_del(&((demo_cdevp+i)->cdev));
	}
	class_destroy(dev_class);
	kfree(demo_cdevp);
	unregister_chrdev_region(dev, DEMO_COUNT);
	
}	
 
module_init(demo_init);


module_exit(demo_exit)






