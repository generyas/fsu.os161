#include <types.h>
#include <lib.h>
#include <test.h>
#include <synch.h>
#include <thread.h>
#include <wchan.h>
#include <clock.h>

#define NTHREADS 32
/*
static void locktest_fail(unsigned long i, const char *message);
static void locktest_individual(void* ignore, unsigned long i);
static void locktest_all();
static void initialize_cv(void);
static void thread_cvtest(void* ignore, unsigned long i);
static int threadJoin_send(void *ignore, unsigned long i);
*/

static struct semaphore *semaphore;
static struct semaphore *semaphore_l;
static struct lock *lock_t1;
static struct cv *cv_t1;

static volatile unsigned long number_check;

/*
int asst1_tests(int i_args, char **arguments)
{
	KASSERT(locktest_all() == 0);
	KASSERT(wait_cvtest(i_args, arguments) == 0);
	KASSERT(wakeIndividual_cvtest(i_args, arguments) == 0);
	KASSERT(wakeAll_cvtest(i_args, arguments) == 0);
	KASSERT(threadJoin_test(i_args, arguments) == 0);

	return 0;
}
*/
static void inititems(void)
{
	if (semaphore==NULL) {
		semaphore = sem_create("testsem", 2);
		if (semaphore == NULL) {
			panic("synchtest: sem_create failed\n");
		}
	}
	if (semaphore_l==NULL) {
		semaphore_l = sem_create("testsem_l", 2);
		if (semaphore_l == NULL) {
			panic("synchtest: sem_create failed\n");
		}
	}
	if (lock_t1==NULL) {
		lock_t1 = lock_create("testlock");
		if (lock_t1 == NULL) {
			panic("synchtest: lock_create failed\n");
		}
	}
	if (cv_t1==NULL) {
		cv_t1 = cv_create("cvtest");
		if (cv_t1 == NULL) {
			panic("synchtest: cv_create failed\n");
		}
	}
}

//////////////////
/// Lock Tests ///
//////////////////

static void locktest_fail(unsigned long i, const char *message)
{
	kprintf("Thread %lu: %s\n", i, message);
	kprintf("Test failed here\n");
	lock_release(lock_t1);
	V(semaphore_l);
}

//changed to void return type
static void locktest_individual(void* ignore, unsigned long i)
{
	(void)ignore;
	//checks if lock test is holding lock at start
	if(lock_do_i_hold(lock_t1))
		locktest_fail(i, "lock should not be be held by this thread at start");

	//checks if thread has the lock after acquire
	lock_acquire(lock_t1);
	if(!lock_do_i_hold(lock_t1))
		locktest_fail(i, "lock should be held by this thread after acquire");

	thread_yield(); //yeild so that other processes can occur

	//check which thread is holding
	if(!lock_do_i_hold(lock_t1))
		locktest_fail(i, "lock should be held by this thread after yield");

	//check if we are holding the lock after releasing it
	lock_release(lock_t1);

	if(lock_do_i_hold(lock_t1))
		locktest_fail(i, "this thread should not hold lock after release");

	V(semaphore_l);
	//return 0;
}

static void locktest_all()
{
	int test = 0;
	semaphore_l = sem_create("semaphore_l", 0);
	kprintf("->Lock Tests beginning:\n");
	//testing for 32 threads
	for(int j=0; j < NTHREADS; j++)
	{
		test = thread_fork("lock_test",NULL,locktest_individual,NULL,j);
		if(test)
			panic("thread_fork FAILED: %s\n", strerror(test));
	}
	for(int j=0; j < NTHREADS; j++)
		P(semaphore_l);

	kprintf("Lock Tests done.\n");
	//return 0;
}

//////////////////////////////////
/// Conditional Variable Tests ///
//////////////////////////////////

static void initialize_cv(void)
{
	number_check=0;
	if(lock_t1==NULL)
	{
		lock_t1 = lock_create("test_l");
		if(lock_t1==NULL)
			panic("lock_create() FAILED\n");
	}
	if(semaphore==NULL)
	{
		semaphore=sem_create("semaphore_test",0);
		if(semaphore==NULL)
			panic("sem_create() FAILED\n");
	}
	if(cv_t1==NULL)
	{
		cv_t1 = cv_create("test_cv");
		if(cv_t1 == NULL)
			panic("cv_create() FAILED\n");
	}
}

static void thread_cvtest(void* ignore, unsigned long i)
{
	(void)ignore;
	lock_acquire(lock_t1);
	V(semaphore);
	while(i != number_check)
	{
		cv_wait(cv_t1, lock_t1);
	}
	V(semaphore);
	lock_release(lock_t1);

}

void wait_cvtest(int i_args, char **arguments)
{
	(void)arguments;
	(void)i_args;
	initialize_cv();
	kprintf("->Beginning wait_cvtest:\n");
	int test = 0;
	test = thread_fork("test_cv",NULL,thread_cvtest, NULL, 1);
	if(test)
		panic("thread_fork %s failed during cv_test\n", strerror(test));
	P(semaphore);
	lock_acquire(lock_t1);
	number_check = number_check+1; //increment to wake up thread
	cv_signal(cv_t1, lock_t1);
	lock_release(lock_t1);
	P(semaphore);
	kprintf("->Ending wait_cvtest\n");
}

void wakeIndividual_cvtest(int i_args, char **arguments)
{
	int test;
	(void)i_args;
	(void)arguments;
	initialize_cv();
	kprintf("->Starting wakeIndividual_cvtest:\n");

	//creates threads that wait for number_check to be 1
	for(int j=0; j<2; j++)
	{
		test = thread_fork("test_cv", NULL, thread_cvtest, NULL, 1);
		if(test)
			panic("thread_fork %s failed during cvtest\n",strerror(test));
		P(semaphore);
	}
	//increments number_check to wake a thread, only signals one at a time
	for(int j=0; j<2; j++)
	{
		lock_acquire(lock_t1);
		number_check = number_check+1;
		cv_signal(cv_t1, lock_t1);
		lock_release(lock_t1);
		P(semaphore);
	}

	kprintf("->Ending wakeIndividual_cvtest:\n");
}

void wakeAll_cvtest(int i_args, char **arguments)
{
	int test;
        (void)i_args;
        (void)arguments;
        initialize_cv();
        kprintf("->Starting wakeAll_cvtest:\n");

        //creates threads that wait for number_check to be 1
        for(int j=0; j<2; j++)
        {
		test = thread_fork("test_cv", NULL, thread_cvtest, NULL, 1);
                if(test)
                        panic("thread_fork %s failed during cvtest\n",strerror(test));
                P(semaphore);
        }

	//creates threads that wait for number_check to be 2
        for(int j=0; j<2; j++)
        {
		test = thread_fork("test_cv", NULL, thread_cvtest, NULL, 2);
                if(test)
                        panic("thread_fork %s failed during cvtest\n",strerror(test));
                P(semaphore);
        }

	//increments number_check to wake all thread with number_check equivalent
        for(int j=0; j<2; j++)
        {
                lock_acquire(lock_t1);
                number_check = number_check+1;
                cv_broadcast(cv_t1, lock_t1);
                lock_release(lock_t1);
                P(semaphore);
		P(semaphore);
        }

	kprintf("->Ending wakeAll_cvtest:\n");
}

/////////////////////////
/// Thread Join Test ////
/////////////////////////
static void threadJoin_send(void *ignore, unsigned long i)
{
	(void)ignore;

	kprintf("Child thread Number: %ld\n", i);
	i = 100 + i;

	//return i;
}

void threadJoin_test(int i_args, char **arguments)
{
	(void)i_args;
        (void)arguments;

	//struct thread *child[NTHREADS]; 
	int returned;

	kprintf("->Beginning of Thread Join Test\n");
	for(int j = 0; j < NTHREADS; j++)
	{
		//result = thread_fork_joinable("synchtest", NULL, cvtestthread, NULL, i);
		//may be incorrect 2nd parameter &(child[j])
		returned=thread_fork_joinable("child thread", NULL, threadJoin_send, NULL, j);
		if(returned)
			panic("thread join failed %s\n", strerror(returned));
	}

	for(int j = 0; j < NTHREADS; j++)
	{
		kprintf("Thread ID: %d\n", thread_join());
	}

	kprintf("->Thread Join Test Done.\n");
}

int asst1_tests(int i_args, char **arguments)
{
	inititems();
        locktest_all();
        wait_cvtest(i_args, arguments);
        wakeIndividual_cvtest(i_args, arguments);
        wakeAll_cvtest(i_args, arguments);
        threadJoin_test(i_args, arguments);
	return 0;
}

