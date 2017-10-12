#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/kthread.h> 
#include <linux/rwsem.h>

MODULE_LICENSE("GPL");

#define THREAD_CNT	8

struct task_struct *t[THREAD_CNT];
static struct rw_thread {
	int n;
	int locks;
	long nsec;
} rw_threads[THREAD_CNT];
static volatile int global_counter=0;

struct rw_semaphore sem;


void timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

static int threadfn(void* data)
{
	int i=0, local_counter=0, locks=0, consecutive_locks=0, n=*(int*)data;
	struct timespec tspec, tstart, tend;
	long nsec;

	if (n) { //read thread
		getrawmonotonic(&tstart);
		while (1) {
			i++;
			down_read(&sem);
			if (local_counter == global_counter) {
				consecutive_locks++;
				// pr_info("%d: consecutive read locks!\n", n);
			} else {
				locks++;
				local_counter = global_counter;
			}
			ndelay(10);
			if (local_counter != global_counter) {
				local_counter = global_counter;
				pr_alert("%d: read lock was interrupted with write!\n", n);
			}
			if (global_counter > 999) {
				up_read(&sem);
				break;
			}
			up_read(&sem);
		}
		getrawmonotonic(&tend);
		timespec_diff(&tstart, &tend, &tspec);
		if (tspec.tv_sec)
			pr_alert("%d: too long execution\n", n);
		nsec = tspec.tv_nsec;
		((struct rw_thread *)data)->locks = locks; //unique reads
		((struct rw_thread *)data)->nsec = nsec;
		pr_info("%d: finished %d iterations\n\tuniq.locks %d, dupl. locks %d) reads\n",
			n, i, locks, consecutive_locks);
	} else { //write thread
		getrawmonotonic(&tstart);
		for (i=0; i<1000; i++) {
			down_write(&sem);
			locks++;
			global_counter++;
			up_write(&sem);
			ndelay(10);
		}
		getrawmonotonic(&tend);
		timespec_diff(&tstart, &tend, &tspec);
		if (tspec.tv_sec)
			pr_alert("%d: too long execution\n", n);
		nsec = tspec.tv_nsec;
		((struct rw_thread *)data)->locks = locks; //unique reads
		((struct rw_thread *)data)->nsec = nsec;
		pr_info("%d: finished %d iterations, %d writes\n", n, i, locks);
	}

	// pr_info("threadfn %d exited after %d: %d(%d)\n", n, i, local_counter, global_counter);
	return 0;
}

static int __init rw_threads_init(void)
{
	int i;
	pr_info("%s\n", __FUNCTION__);

	init_rwsem(&sem);

	for (i=0; i<THREAD_CNT; i++) {
		rw_threads[i].n = i;
		rw_threads[i].locks = 0, rw_threads[i].nsec = 0;
		t[i] = kthread_create(&threadfn, &rw_threads[i], "rw_thread%d", rw_threads[i].n);
		if (IS_ERR(t[i])) {
			pr_err("kthread_run failed on thread %d", i);
		}
	}

	for (i=0; i<THREAD_CNT; i++) {
		if (!IS_ERR(t[i]))
			wake_up_process(t[i]);
	}
	// atomic_set(&m, 0);

	return 0;
}

static void __exit rw_threads_exit(void)
{
	int i, r_locks=0;
	long r_nsecs=0;
	for (i=1; i<THREAD_CNT; i++) {
		r_locks += rw_threads[i].locks;
		r_nsecs += rw_threads[i].nsec;
	}
	pr_info("%s: w = %d (%ld ns), r = %d (%ld ns)\n", __FUNCTION__,
		rw_threads[0].locks, rw_threads[0].nsec, r_locks, r_nsecs);
}

module_init(rw_threads_init);
module_exit(rw_threads_exit);