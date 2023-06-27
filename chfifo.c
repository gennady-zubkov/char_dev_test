#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include<linux/uaccess.h>
#include <linux/list.h>
#include <linux/sched.h>

/*
** Functions Declarations:
*/
static int fifo_open(struct inode *pinode, struct file *pfile);
static int fifo_release(struct inode *pinode, struct file *pfile);
static ssize_t fifo_read(struct file *pfile, char *ubuf, size_t length, loff_t *poffset);
static ssize_t fifo_write(struct file *pfile, const char *ubuf, size_t length, loff_t *poffset);


#define DEVICE_NAME	"chfifo"      //Char device name
#define FIFO_MAX_SIZE   1000        //MAX fifo capacity

dev_t dev = 0;
static struct class *dev_class;
static struct cdev fifo_cdev;

unsigned char *kernel_buffer;		// Buffer to keep data for read/write operations

// The driver's file operations structure
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = fifo_open,
    .release = fifo_release,
    .read = fifo_read,
    .write = fifo_write,
};

/*FIFO (List) Node*/
struct fifo_item {
     struct list_head list;
     unsigned char data;  // data stored in the fifo(queue) item
};
//struct list_head list;

struct fifo_list {
    struct list_head headlist;  // the head of the list (fifo)
    int length;                 // the fifo size
};

static struct fifo_list fifo; // The one queue to keep the fifo items

/*
** Module Init function
*/
static int __init fifo_dev_init(void)
{ 
    /*Allocating Major number*/
    if((alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME)) <0){
    	pr_err("fifo_dev: Cannot allocate major number\n");
    	return -1;
    }
    pr_info("fifo_dev: Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
    /*Creating cdev structure*/
    cdev_init(&fifo_cdev,&fops);
    fifo_cdev.owner = THIS_MODULE;

    /*Adding character device to the system*/
    if((cdev_add(&fifo_cdev,dev,1)) < 0){
    	pr_err("fifo_dev: Cannot add the device to the system\n");
        goto r_class;
    }
    /*Creating struct class*/
    if(IS_ERR(dev_class = class_create(THIS_MODULE, DEVICE_NAME))){
    	pr_err("fifo_dev: Cannot create the struct class\n");
        goto r_class;
    }
    /*Creating device*/
    if(IS_ERR(device_create(dev_class,NULL,dev,NULL, DEVICE_NAME))){
    	pr_err("fifo_dev: Cannot create the Device 1\n");
        goto r_device;
    }

    /*Allocating temporary buffer in kerenel*/
    if((kernel_buffer = kzalloc(FIFO_MAX_SIZE, GFP_KERNEL)) == 0){
        pr_info("Cannot allocate memory in kernel\n");
        goto r_device;
    }


    // Initialize the FIFO list
    INIT_LIST_HEAD(&(fifo.headlist));
    fifo.length = 0;

    pr_info("fifo_dev: Module Char-Queue has been started!\n");
    return 0;

r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev,1);
	return -1;
} 

/*
** Module release function. Closing the device
*/
static void __exit fifo_dev_exit(void)
{
   kfree(kernel_buffer);
   device_destroy(dev_class,dev);
   class_destroy(dev_class);
   cdev_del(&fifo_cdev);
   unregister_chrdev_region(dev, 1);
   pr_info("fifo_dev: Module Char-Queue finished. Exit.\n"); 
} 

/*
** Function: Open the Device file
*/
static int fifo_open(struct inode *pinode, struct file *pfile)
{
    pr_info("fifo_dev: Open Function Called...\n");
    pr_info("fifo_dev: pid = %d\n", current->pid);

    // Initialize the FIFO
    fifo.length = 0;
    


    return 0;
}

/*
** Function: Release the Device file
*/
static int fifo_release(struct inode *pinode, struct file *pfile)
{
    pr_info("fifo_dev: Release Function Called...\n");

    struct list_head *pos;
    struct list_head *q;
    struct fifo_item *item;

    list_for_each_safe(pos, q, &(fifo.headlist))
    {
        item=list_entry(pos, struct fifo_item, list);
        list_del(pos);
        kfree(item);
    }

    return 0;
}

/*
** Function: Read from the Device file
** Return value: number of read items from the queue(fifo) or error = -ENODATA if fifo is empty
*/
static ssize_t fifo_read(struct file *pfile, char *ubuf, size_t length, loff_t *poffset)
{
   int res = 0;   // number of read items from the list
   int i = 0;     // buffer index

   if(list_empty(&(fifo.headlist))){
       // The fifo list is empty
       pr_info("fifo_dev: Data Read : Queue is Empty!\n");
       return -ENODATA; // FIFO is empty no data is available
   }

   //pr_info("fifo_dev: Data Read :  length = %d\n", (int)length);

    while(length > 0){
        struct fifo_item* item;
        item=list_entry(fifo.headlist.next, struct fifo_item, list);
    	if(!list_empty(&(fifo.headlist)))
    	{
	    kernel_buffer[i] = item->data; 
	    list_del(&(item->list));
	    kfree(item);
	    fifo.length--;
	    //pr_info("fifo_dev: Data Read - list item [%d] = %c \n", i, kernel_buffer[i]);
	    i++;
        }else{
	    // The fifo list is empty
	    pr_info("fifo_dev: Data Read - Queue is Empty!\n");
	    break;     
        }
        length--;
	res++;
    }

    //Copy the data from the kernel space to the user-space
    if( copy_to_user(ubuf, kernel_buffer, res) )
    {
        pr_err("fifo_dev: Data Read - Error!\n");
	return -EFAULT;
    }


    pr_info("fifo_dev: Data Read - Done!\n");
    return res;
}

/*
** Function: Write to the Device file
** Return value: number of written items to the queue(fifo)
*/
static ssize_t fifo_write(struct file *pfile, const char *ubuf, size_t length, loff_t *poffset)
{
    int i = 0; // buffer index
    int res = 0; // retrns number of written bytes

    //Copy the data to kernel space from the user-space
    if( copy_from_user(kernel_buffer, ubuf, length) )
    {
        pr_err("fifo_dev: Data Write - Error!\n");
	return -EFAULT;
    }
    

    while(length > 0)
    {
	struct fifo_item* item;
    	item=kmalloc(sizeof(struct fifo_item), GFP_KERNEL);
        item->data = kernel_buffer[i++];
        list_add_tail(&(item->list), &(fifo.headlist));
        fifo.length++;
	length--;
	res++;
	
	if(fifo.length >= FIFO_MAX_SIZE){
            pr_err("fifo_dev: Data Write - FIFO is Full!\n");
            break;
	}
    }

    pr_info("fifo_dev: Data Write - Done!\n");
    return res;
}


module_init(fifo_dev_init);
module_exit(fifo_dev_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Character Device Driver - Chars Queue");
MODULE_AUTHOR("Gennady Zubkov");
MODULE_VERSION("1.0");

