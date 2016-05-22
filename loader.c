#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cpu.h>
#include <linux/memblock.h>

#include <asm/cacheflush.h>

#define RT_BASE_ADDR 0x10000000
#define RT_MEM_SIZE  0x00400000
#define BUFF_SZ		(4 * 1024)

int do_load_fw(const char* filename,
        unsigned long base_addr,
                size_t mem_size)
{
    mm_segment_t oldfs = {0};
	ssize_t len;
	unsigned long file_sz;
	loff_t pos = 0;
	struct file *flp = NULL;
	unsigned long buf_ptr = base_addr;

    printk("loading u-boot:%s to %08lx....\n",
               filename, buf_ptr);

    flp = filp_open(filename, O_RDONLY, S_IRWXU);
    if(IS_ERR(flp)) {
        printk("loader: open file failed");
        return -1;
    }

    file_sz = vfs_llseek(flp, 0, SEEK_END);
    if (file_sz > mem_size) {
		printk("rtloader: bin file too big. "
			"mem size: 0x%08x, bin file size: 0x%08lx\n",
			mem_size, file_sz);
		filp_close(flp, NULL);
		return -1;
	}
	printk("loader: bin file size: 0x%08lx\n", file_sz);
	vfs_llseek(flp, 0, SEEK_SET);

	oldfs = get_fs();
	set_fs(get_ds());
	while (file_sz > 0) {
		len = vfs_read(flp, (void __user __force*)buf_ptr, BUFF_SZ, &pos);
		if (len < 0) {
			pr_err("read %08lx error: %d\n", buf_ptr, len);
			set_fs(oldfs);
			filp_close(flp, NULL);
			return -1;
		}
		file_sz -= len;
		buf_ptr += len;
	}
	set_fs(oldfs);

	printk("done!\n");
	
	flush_cache_vmap(base_addr, mem_size);

	return 0;
}
   
static int __init loader_init(void)
{
    int ret;
	void *va;
	void (*theUboot) (void);

    //va = ioremap_nocache(RT_BASE_ADDR, RT_MEM_SIZE);
	va = phys_to_virt(RT_BASE_ADDR);
    pr_info("get mapping :%p -> %08x, size: %08x\n", va, RT_BASE_ADDR, RT_MEM_SIZE);

    if(do_load_fw("/mnt/boot/u-boot.bin", (unsigned long)va, RT_MEM_SIZE) == 0){	
    	theUboot = (void (*)(void)) (va);
		printk("start boot zImage...\n");
		theUboot();
		printk("end boot zImage...\n");
    }

    return 0;
}

static void __exit loader_exit(void)
{

}

module_init(loader_init);
module_exit(loader_exit);

MODULE_DESCRIPTION("LOADER");
MODULE_LICENSE("GPL");


