#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/of.h>
/* functions def */
//设备号为<unsigned int>类型 提取主设备号 副设备号
#define MINNUMS         20    
//#define MAJOR(x)        (unsigned int)(x >> MINNUMS)
//#define MINOR(x)        (unsigned int)(x & ( (0x1 << MINNUMS) - 1) )
//#define MKDEV(mj,mi)    (unsigned int)( (mj << MINNUMS) | mi )

#define DEV_NAME   "dev_mod"
#define DEV_NUMS    1

/* 寄存器物理地址 
 *  eg:#define  CCM_CCGR1_BASE               (0X020C406C)
 */
/* 映射后的寄存器虚拟地址指针 
 *  eg: static void __iomem *IMX6U_CCM_CCGR1; 
 */


static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *GPIO1_IO3_PAD;
static void __iomem *GPIO1_IO3_MUX;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDR;

/* values types defines */
struct dev_mod{
    dev_t devid;
    struct cdev   dev;
    struct class  *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *dnode;
};

struct dev_mod chdev;


/* led drv */
void led_on(void)
{
    writel( readl(GPIO1_DR) & ( ~(0x1<<3) ) ,GPIO1_DR);
}

void led_off(void)
{
    writel( readl(GPIO1_DR) | (0x1<<3) ,GPIO1_DR);
}

/* operations */
/* 驱动层open -> 应用层 open */
static int dev_open(struct inode *inode,struct file *filp)
{
    filp->private_data = &chdev;/* 将设备结构体传给用户空间 */
    return 0;
}

/* 驱动层read -> 应用层 read */
static ssize_t dev_read(struct file *filp,char __user *buf,size_t cnt,loff_t *offt)
{
    /* code */
    return 0;
}

/* 驱动层write -> 应用层 write */
static ssize_t dev_write(struct file *filp,const char __user *buf,size_t cnt,loff_t *offt)
{
    int ret;
    unsigned char data_buf[1];
    /* code */
    ret = copy_from_user(data_buf,buf,cnt);
    if(ret < 0)
    {
        printk("write error!\r\n");
        return EFAULT;
    }
    if(data_buf[0] == '1')
    {
        printk("led off!\r\n");
        led_off();
    }
    else if(data_buf[0] == '2')
    {
        printk("led on!\r\n");
        led_on();
    }
    return 0;
}

/* 驱动层release -> 应用层 close */
static int dev_release(struct inode *inode,struct file *filp)
{
    return 0;
}

/* 设备操作函数 */
static struct file_operations dev_op = {
    .owner = THIS_MODULE,
    .open  = dev_open,
    .read  = dev_read,
    .write = dev_write,
    .release = dev_release
};

static int __init dev_init(void)//驱动入口
{
    /* 初始化设备 */
    /* 寄存器映射 */
    int ret;
    u32 array[10];
    struct property *proper;
    const char* str;
    u32 val;
    /*
     * EG:IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4); 
     */
    /* 是能时钟 */
    /* EG: unsigned int val;
     * val = readl(IMX6U_CCM_CCGR1);
     * val &= x<<xx;
     * cal |= x<<xx;
     *       writel(val,IMX6U_CCM_CCGR1)
     */

    //获取设备数张树的节点
    chdev.dnode = of_find_node_by_path("/ts");
    if(chdev.dnode == NULL)
    {
        printk("can not find node!\r\n");
        return EFAULT;
    }
    else
    {
        /* code */
        printk("node has been found!\r\n");
    }
    //获取适配信息
    proper = of_find_property(chdev.dnode,"compatible",NULL);
    if(proper == NULL)
    {
        printk("compatible property find failed!\r\n");
        return EFAULT;
    }
    else
    {
        printk("compatible = %s\r\n", (char*)proper->value);
    }
    //获取状态信息
    ret = of_property_read_string(chdev.dnode,"status",&str);
    if(ret < 0)
    {
        printk("status get failed!\r\n");
        return EFAULT;
    }
    else
    {
        printk("status is :%s",str);
    }
    //获取reg
    ret = of_property_read_u32_array(chdev.dnode,"reg",array,10);
    if(ret < 0)
    {
        printk("reg get failed!\r\n");
        return EFAULT;
    }
    else
    {
        u8 i;
        for(i=1;i<6;i++)
        {
            printk("addr<%d>:%#x lenth:%d\r\n",i,array[(i-1)*2],array[2*i-1]);
        }
    }    

    /* 地址映射 */
    IMX6U_CCM_CCGR1 = ioremap(array[0], array[1]);
    GPIO1_IO3_PAD   = ioremap(array[2], array[3]);
    GPIO1_IO3_MUX   = ioremap(array[4], array[5]);
    GPIO1_DR        = ioremap(array[6], array[7]);
    GPIO1_GDR       = ioremap(array[8], array[9]);
// IMX6U_CCM_CCGR1 = of_iomap(dtsled.nd, 0); 
// SW_MUX_GPIO1_IO03 = of_iomap(dtsled.nd, 1); 
// SW_PAD_GPIO1_IO03 = of_iomap(dtsled.nd, 2); 
// GPIO1_DR = of_iomap(dtsled.nd, 3); 
// GPIO1_GDIR = of_iomap(dtsled.nd, 4); 
//
    /* 使能时钟 */
    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(0x3<<26);//to be zero
    writel(val,IMX6U_CCM_CCGR1);
    val |=  (0x3<<26);//enable clk gpio1 
    writel(val,IMX6U_CCM_CCGR1);
    /* 配置管脚 */
    writel(0x05,GPIO1_IO3_MUX);//AF GPIO1_IO3
    writel(0x10B0,GPIO1_IO3_MUX);//IO format out
    writel(readl(GPIO1_GDR)|(0x1<<3),GPIO1_GDR);//IO format out
    writel(readl(GPIO1_DR)&(~(0x1<<3)),GPIO1_DR);//init on

    /* 申请设备号 */
    alloc_chrdev_region(&chdev.devid,0,DEV_NUMS,DEV_NAME); 
    chdev.major = MAJOR(chdev.devid);//获取主设备号
    chdev.minor = MINOR(chdev.devid);//获取副设备号
    printk("newcheled major=%d,minor=%d\r\n", chdev.major, chdev.minor);

    /* 初始化 struct dev_mod chdev */
    chdev.dev.owner = THIS_MODULE;
    cdev_init(&chdev.dev,&dev_op);
    cdev_add(&chdev.dev,chdev.devid,DEV_NUMS);

    /* 注册设备 */
    chdev.class = class_create(THIS_MODULE,DEV_NAME);
    if(IS_ERR(chdev.class))
    {
        return PTR_ERR(chdev.class);
    }
    chdev.device = device_create(chdev.class,NULL,chdev.devid,NULL,DEV_NAME);
    if(IS_ERR(chdev.device))
    {
        return PTR_ERR(chdev.device);
    }
    return 0;
}

static void __exit dev_exit(void)//驱动出口
{
    /* 取消映射 */
    /* EG:
     *  iounmap(IMX6U_CCM_CCGR1); 
     */

    /* 注销设备 */
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(GPIO1_IO3_MUX); 
    iounmap(GPIO1_IO3_PAD); 
    iounmap(GPIO1_GDR); 
    iounmap(GPIO1_DR); 

    cdev_del(&chdev.dev);
    unregister_chrdev_region(chdev.devid,DEV_NUMS);
    device_destroy(chdev.class,chdev.devid);
    class_destroy(chdev.class);
}

/* module end */
module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xny");
