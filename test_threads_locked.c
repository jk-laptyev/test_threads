#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/kthread.h> 
#include <linux/err.h>
#include <linux/atomic.h>

MODULE_LICENSE("GPL");

#define THREAD_CNT	2

struct task_struct *t[THREAD_CNT];
static int num[]={0,1,2,3,4,5,6,7}, global_counter=0;
static atomic_t m = ATOMIC_INIT(0);

#define LOCK(a) atomic_cmpxchg(a, 0, 1)
#define UNLOCK(a) atomic_xchg(a, 0)

// static DEFINE_SPINLOCK(sp_lock);

static void lock(atomic_t *m)
{
// 	while (atomic_xchg(m, 1))
// 		ndelay(1);
	while (1)
		if (atomic_xchg(m, 1)==0)
			break;
}

static void unlock(atomic_t *m)
{
	// atomic_set(m, 0);
	atomic_xchg(m, 0);
}

static int threadfn(void* data)
{
	int i, local_counter, n=*(int*)data;

	for (i = 0; i < 10000000; i++) {
		lock(&m);
		// while(LOCK(&m));
		// spin_lock(&sp_lock);
		local_counter = ++global_counter;
		unlock(&m);
		// UNLOCK(&m);
		// spin_unlock(&sp_lock);
		// ndelay(1);
	}

	pr_info("threadfn_locked %d exited after %d: %d(%d)\n", n, i, local_counter, global_counter);
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
	// atomic_set(&m, 0);

	return 0;
}

static void __exit test_threads_exit(void)
{
	pr_info("test_threads exit\n");
}

module_init(test_threads_init);
module_exit(test_threads_exit);