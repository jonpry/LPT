/* linux/drivers/parport/parport_bone.c
 *
 * (c) 2013 Karltech Inc
 *	Jon Pry <jonpry@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/parport.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <asm/io.h>
#include <asm/irq.h>

#define BONE_SPR_BUSY		(1<<7)
#define BONE_SPR_ACK		(1<<6)
#define BONE_SPR_PO		(1<<5)
#define BONE_SPR_SLCT		(1<<4)
#define BONE_SPR_ERR		(1<<3)

#define BONE_CPR_nDOE		(1<<5)
#define BONE_CPR_SLCTIN		(1<<3)
#define BONE_CPR_nINIT		(1<<2)
#define BONE_CPR_ATFD		(1<<1)
#define BONE_CPR_STRB		(1<<0)

#define PARPORT_BONE_MAX_PORTS	8

struct bone_drvdata {
	struct parport		*parport;

	struct device		*dev;
	struct resource		*io;

	void __iomem		*base;
	void __iomem		*spp_data;
	void __iomem		*spp_spr;
	void __iomem		*spp_cpr;

	struct list_head list;
};

static LIST_HEAD(ports_list);
static DEFINE_SPINLOCK(ports_lock);

static int __initdata io[PARPORT_BONE_MAX_PORTS+1] = {
	[0 ... PARPORT_BONE_MAX_PORTS] = 0
};

static int nports = 0;

MODULE_PARM_DESC(io, "Base I/O address (SPP regs)");
module_param_array(io, int, &nports, 0);

static int delay = 1000;
MODULE_PARM_DESC(delay,
	"Control port readback delay(ns)");
module_param(delay, int, 0);

static inline struct bone_drvdata *pp_to_drv(struct parport *p)
{
	return p->private_data;
}

static unsigned char
parport_bone_read_data(struct parport *p)
{
	struct bone_drvdata *dd = pp_to_drv(p);

	return readb(dd->spp_data);
}

static void
parport_bone_write_data(struct parport *p, unsigned char data)
{
	struct bone_drvdata *dd = pp_to_drv(p);

	writeb(data, dd->spp_data);
}

static unsigned char
parport_bone_read_control(struct parport *p)
{
	struct bone_drvdata *dd = pp_to_drv(p);
	unsigned int cpr = readb(dd->spp_cpr);
	unsigned int ret = 0;

	if (cpr & BONE_CPR_STRB)
		ret |= PARPORT_CONTROL_STROBE;

	if (cpr & BONE_CPR_ATFD)
		ret |= PARPORT_CONTROL_AUTOFD;

	if (cpr & BONE_CPR_nINIT)
		ret |= PARPORT_CONTROL_INIT;

	if (cpr & BONE_CPR_SLCTIN)
		ret |= PARPORT_CONTROL_SELECT;

	return ret;
}

static void
parport_bone_write_control(struct parport *p, unsigned char control)
{
	struct bone_drvdata *dd = pp_to_drv(p);
	unsigned int cpr = readb(dd->spp_cpr);

	cpr &= BONE_CPR_nDOE;

	if (control & PARPORT_CONTROL_STROBE)
		cpr |= BONE_CPR_STRB;

	if (control & PARPORT_CONTROL_AUTOFD)
		cpr |= BONE_CPR_ATFD;

	if (control & PARPORT_CONTROL_INIT)
		cpr |= BONE_CPR_nINIT;

	if (control & PARPORT_CONTROL_SELECT)
		cpr |= BONE_CPR_SLCTIN;

//	printk(KERN_DEBUG "write_control: ctrl=%02x, cpr=%02x\n", control, cpr);
	writeb(cpr, dd->spp_cpr); ndelay(delay);

	if (parport_bone_read_control(p) != cpr) {
		printk(KERN_ERR "write_control: read != set (%02x, %02x)\n",
			parport_bone_read_control(p), cpr);
	}
}

static unsigned char
parport_bone_read_status(struct parport *p)
{
	struct bone_drvdata *dd = pp_to_drv(p);
	unsigned int status = readb(dd->spp_spr);
	unsigned int ret = 0;

	if (status & BONE_SPR_BUSY)
		ret |= PARPORT_STATUS_BUSY;

	if (status & BONE_SPR_ACK)
		ret |= PARPORT_STATUS_ACK;

	if (status & BONE_SPR_ERR)
		ret |= PARPORT_STATUS_ERROR;

	if (status & BONE_SPR_SLCT)
		ret |= PARPORT_STATUS_SELECT;

	if (status & BONE_SPR_PO)
		ret |= PARPORT_STATUS_PAPEROUT;

	return ret;
}

static unsigned char
parport_bone_frob_control(struct parport *p, unsigned char mask,
			     unsigned char val)
{
	//struct bone_drvdata *dd = pp_to_drv(p);
	//TODO: local mirror
	unsigned char old = parport_bone_read_control(p);

	//printk(KERN_DEBUG "frob: mask=%02x, val=%02x, old=%02x\n",
	//	mask, val, old);

	parport_bone_write_control(p, (old & ~mask) | val);
	return old;
}

static void
parport_bone_init_state(struct pardevice *d, struct parport_state *s)
{
	struct bone_drvdata *dd = pp_to_drv(d->port);

	memset(s, 0, sizeof(struct parport_state));

//	printk(KERN_DEBUG "init_state: %p: state=%p\n", d, s);
	s->u.ax88796.cpr = readb(dd->spp_cpr);
}

static void
parport_bone_save_state(struct parport *p, struct parport_state *s)
{
	struct bone_drvdata *dd = pp_to_drv(p);

//	printk(KERN_DEBUG "save_state: %p: state=%p\n", p, s);
	s->u.ax88796.cpr = readb(dd->spp_cpr);
}

static void
parport_bone_restore_state(struct parport *p, struct parport_state *s)
{
	struct bone_drvdata *dd = pp_to_drv(p);

//	printk(KERN_DEBUG "restore_state: %p: state=%p\n", p, s);
	writeb(s->u.ax88796.cpr, dd->spp_cpr);
}

static void
parport_bone_data_forward(struct parport *p)
{
	struct bone_drvdata *dd = pp_to_drv(p);
	void __iomem *cpr = dd->spp_cpr;

	writeb((readb(cpr) & ~BONE_CPR_nDOE), cpr);
}

static void
parport_bone_data_reverse(struct parport *p)
{
	struct bone_drvdata *dd = pp_to_drv(p);
	void __iomem *cpr = dd->spp_cpr;

	writeb(readb(cpr) | BONE_CPR_nDOE, cpr);
}


static void 
parport_bone_enable_irq(struct parport *p)
{
}

static void 
parport_bone_disable_irq(struct parport *p)
{
}

static struct parport_operations parport_bone_ops = {
	.write_data	= parport_bone_write_data,
	.read_data	= parport_bone_read_data,

	.write_control	= parport_bone_write_control,
	.read_control	= parport_bone_read_control,
	.frob_control	= parport_bone_frob_control,

	.read_status	= parport_bone_read_status,

	.enable_irq	= parport_bone_enable_irq,
	.disable_irq	= parport_bone_disable_irq,

	.data_forward	= parport_bone_data_forward,
	.data_reverse	= parport_bone_data_reverse,

	.init_state	= parport_bone_init_state,
	.save_state	= parport_bone_save_state,
	.restore_state	= parport_bone_restore_state,

	.epp_write_data	= parport_ieee1284_epp_write_data,
	.epp_read_data	= parport_ieee1284_epp_read_data,
	.epp_write_addr	= parport_ieee1284_epp_write_addr,
	.epp_read_addr	= parport_ieee1284_epp_read_addr,

	.ecp_write_data	= parport_ieee1284_ecp_write_data,
	.ecp_read_data	= parport_ieee1284_ecp_read_data,
	.ecp_write_addr	= parport_ieee1284_ecp_write_addr,

	.compat_write_data	= parport_ieee1284_write_compat,
	.nibble_read_data	= parport_ieee1284_read_nibble,
	.byte_read_data		= parport_ieee1284_read_byte,

	.owner		= THIS_MODULE,
};

static int parport_bone_probe(struct platform_device *pdev)
{
	struct bone_drvdata *dd;
	struct parport *pp = NULL;
	unsigned long size;
	int ret;

	dd = kzalloc(sizeof(struct bone_drvdata), GFP_KERNEL);
	if (dd == NULL) {
		printk(KERN_ERR "no memory for private data\n");
		return -ENOMEM;
	}

	size = SZ_4K;

	printk(KERN_DEBUG "Mapping 0x%8.8X pg: 0x%8.8X\n", pdev->id, (unsigned)(pdev->id & PAGE_MASK));  

	dd->base = ioremap(pdev->id & PAGE_MASK, size);
	if (dd->base == NULL) {
		printk(KERN_ERR "cannot ioremap region\n");
		ret = -ENXIO;
		goto exit_mem;
	}

	pp = parport_register_port(pdev->id, PARPORT_IRQ_NONE,
				   PARPORT_DMA_NONE,
				   &parport_bone_ops);

	if (pp == NULL) {
		printk(KERN_ERR "failed to register parallel port\n");
		ret = -ENOMEM;
		goto exit_unmap;
	}

	pp->private_data = dd;
	dd->parport = pp;
	pp->dev = &pdev->dev;

	dd->spp_data = dd->base + (pdev->id & ~PAGE_MASK);
	dd->spp_spr  = dd->base + (pdev->id & ~PAGE_MASK) + 2;
	dd->spp_cpr  = dd->base + (pdev->id & ~PAGE_MASK) + 4;

	INIT_LIST_HEAD(&dd->list);

	platform_set_drvdata(pdev, pp);

	/* initialise the port controls */
	writeb(BONE_CPR_STRB, dd->spp_cpr);

	spin_lock(&ports_lock);
	list_add(&dd->list, &ports_list);
	spin_unlock(&ports_lock);

	printk(KERN_INFO "attached parallel port driver\n");
	parport_announce_port(pp);

	return 0;

 exit_unmap:
	iounmap(dd->base);
 exit_mem:
	kfree(dd);
	return ret;
}

static int parport_bone_remove(struct platform_device *pdev)
{
	struct parport *pp = platform_get_drvdata(pdev);
	struct bone_drvdata *dd = pp_to_drv(pp);

	parport_remove_port(pp);
	iounmap(dd->base);
	
	platform_set_drvdata(pdev, 0);

	return 0;
}

static struct platform_driver parport_bone_driver = {
	.remove = parport_bone_remove,
	.probe = parport_bone_probe,
	.driver   = {
		.name	= "bone-parallel",
		.owner	= THIS_MODULE,
	},
};

static int __init parport_bone_init(void)
{
	int i;
	if(nports <= 0){
		printk(KERN_ERR "No ports specified\n");
		return -1;
	}

	for(i=0; i < nports; i++){
		struct platform_device *pdev = platform_device_register_simple("bone-parallel",
						       io[i], NULL, 0);
		if(!pdev){
			printk(KERN_ERR "Could not allocate device\n");
			return -1;
		}
		//TODO: free all previous pdevs
	}

	if(platform_driver_register(&parport_bone_driver)){
		printk(KERN_ERR "Could not register driver\n");
		//TODO: free all previous pdevs
		return -1;
	}
	return 0;
}

static void __exit parport_bone_exit(void)
{
	platform_driver_unregister(&parport_bone_driver);

	while (!list_empty(&ports_list)) {
		struct bone_drvdata *dd;
		struct parport *port;
		dd = list_entry(ports_list.next,
				  struct bone_drvdata, list);
		port = dd->parport;
		platform_device_unregister(
				to_platform_device(port->dev));
		spin_lock(&ports_lock);	
		list_del_init(&dd->list);
		spin_unlock(&ports_lock);

		kfree(dd);

	}
}

module_init(parport_bone_init)
module_exit(parport_bone_exit)

MODULE_AUTHOR("Jon Pry <jonpry@gmail.com>");
MODULE_DESCRIPTION("BeagleBone LPT Cape parallel port driver");
MODULE_LICENSE("GPL");
