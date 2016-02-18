#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/irqflags.h>
#include <linux/kthread.h>
#include <linux/cpumask.h>
#include <linux/preempt.h>
#include <asm/uaccess.h>
#include <asm/mwait.h>
#include <asm/page_types.h>
#include <asm/cpufeature.h>
#include <linux/ktime.h>
#include <linux/sort.h>
#include <asm/tsc.h>

#include "thread_fn_util.h"
#include "../../IPC/ipc.h"
#define CHAN_NUM_PAGES 4
#define CHAN1_CPU 1
#define CHAN2_CPU 3

MODULE_LICENSE("GPL");

static struct ttd_ring_channel *chan1;
static struct ttd_ring_channel *chan2;

static void setup_tests(void)
{
    chan1 = create_channel(CHAN_NUM_PAGES);
	if (!chan1) {
		pr_err("Failed to create channel 1");
		return;
	}
	chan2 = create_channel(CHAN_NUM_PAGES);
	if (!chan2) {
		pr_err("Failed to create channel 2");
		free_channel(chan1);
		return;
	}
	connect_channels(chan1, chan2);

        /* Create a thread for each channel to utilize, pin it to a core.
         * Pass a function pointer to call on wakeup.
         */
        struct task_struct *thread1 = attach_thread_to_channel(chan1, CHAN1_CPU, thread1_fn1);
        struct task_struct *thread2 = attach_thread_to_channel(chan2, CHAN2_CPU, thread2_fn1);
        if ( thread1 == NULL || thread2 == NULL ) {
                ttd_ring_channel_free(chan1);
                ttd_ring_channel_free(chan2);
                kfree(chan1);
                kfree(chan2);
                return;
        }
	ipc_start_thread(thread1);
	ipc_start_thread(thread2);
}

static int __init async_dispatch_start(void)
{
	#if defined(USE_MWAIT)
        	if (!this_cpu_has(X86_FEATURE_MWAIT))
		{
			printk(KERN_ERR "CPU does not have X86_FEATURE_MWAIT ");
                	return -EPERM;
		}
	#endif
        setup_tests();

        return 0;
}

static int __exit async_dispatch_end(void)
{
    printk(KERN_ERR "done\n");
    return 0;
}

module_init(async_dispatch_start);
module_exit(async_dispatch_end);
