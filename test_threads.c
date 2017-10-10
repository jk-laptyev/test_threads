#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/kthread.h> 
#include <linux/err.h>

MODULE_LICENSE("GPL");

#define THREAD_CNT	2

struct task_struct *t[THREAD_CNT];
static int num[]={0,1,2,3,4,5,6,7}, global_counter=0;

static int threadfn(void* data)
{
	int i, local_counter, n=*(int*)data;

	for (i = 0; i < 10000000; i++) {
		local_counter = ++global_counter;
		ndelay(1);
	}

	pr_info("threadfn %d exited after %d: %d(%d)\n", n, i, local_counter, global_counter);
	return 0;
}

static int __init test_threads_init(void)
{
	int i;
	pr_info("test_threads loaded\n");

	for (i=0; i<THREAD_CNT; i++) {
		t[i] = kthread_create(&threadfn, &num[i], "test_thread%d", num[i]);
		if (IS_ERR(t[i])) {
			pr_info("kthread_run failed on thread %d", i);
		}
	}

	for (i=0; i<THREAD_CNT; i++) {
		if (!IS_ERR(t[i]))
			wake_up_process(t[i]);
	}

	return 0;
}

static void __exit test_threads_exit(void)
{
	pr_info("test_threads exit\n");
}

module_init(test_threads_init);
module_exit(test_threads_exit);