#include "autolock.h"
ThreadMutexAutoLock::ThreadMutexAutoLock(pthread_mutex_t *lock,
		bool locked)
	:_lock(lock),_locked(lock)
{
}

ThreadMutexAutoLock::~ThreadMutexAutoLock()
{
	if(_lock==nullptr||!_locked)
		return;
	pthread_mutex_unlock(_lock);
	_locked=false;
}

int ThreadMutexAutoLock::lock()noexcept
{
	int err=pthread_mutex_lock(_lock);
	if(err!=0)
		_locked=false;
	else
		_locked=true;
	return -err;
}

int ThreadMutexAutoLock::unlock()noexcept
{
	int err=pthread_mutex_unlock(_lock);
	if(err!=0)
		_locked=true;
	else
		_locked=false;
	return -err;
}

#ifdef GTEST_TEST
#include <gtest/gtest.h>
TEST(TestAutoLock,TestLock)
{
	sem_t sem;
	sem_init(&sem,0,2);
	{
		SemAutoLock semlock(&sem);
		AutoLock &lock=semlock;
		lock.lock();
		lock.lock();
		int i;
		sem_getvalue(&sem,&i);
		ASSERT_EQ(0,i);
		lock.unlock();
	}
	int i;
	sem_getvalue(&sem,&i);
	ASSERT_EQ(2,i);
}

TEST(TestThreadMutexAutoLock,TestLock)
{
	pthread_mutex_t lock;
	pthread_mutex_init(&lock,nullptr);
	{
		ThreadMutexAutoLock mutex(&lock);
		EXPECT_EQ(0,mutex.lock());
	}
	{
		ThreadMutexAutoLock mutex(&lock);
		EXPECT_EQ(0,mutex.lock());
		EXPECT_EQ(0,mutex.unlock());
	}
	pthread_mutex_destroy(&lock);
}

#endif
