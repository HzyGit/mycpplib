#include "autolock.h"
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
#endif
