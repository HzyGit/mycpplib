#ifndef _AUTO_LOCK_H
#define _AUTO_LOCK_H

#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

class AutoLock
{
	public:
		virtual int lock()noexcept=0;
		virtual int unlock()noexcept=0;
		virtual ~AutoLock()noexcept
		{
		}
};

class SemAutoLock : public AutoLock
{
	public:
		SemAutoLock(sem_t *sem=nullptr,int locked=0)
			:_sem(sem),_locked(locked)
		{
		}
		virtual ~SemAutoLock()
		{
			if(nullptr==_sem||_locked==0)
				return;
			for(int i=0;i<_locked;++i)
				sem_post(_sem);
			for(int i=_locked;i<0;++i)
				sem_wait(_sem);
			_locked=0;
		}
		SemAutoLock(const SemAutoLock &&sem)=delete;
		SemAutoLock& operator=(const SemAutoLock& lock)=delete;
	public:
		virtual int lock()noexcept override
		{
			if(nullptr==_sem)
				return -EINVAL;
			int res=0;
			for(;;)
			{
				if((res=sem_wait(_sem))<0)
					if(EINTR==errno)
						continue;
				break;
			}
			if(res==0)
				_locked++;
			return -errno;
		}

		virtual int unlock()noexcept override
		{
			if(nullptr==_sem)
				return -EINVAL;
			errno=0;
			if(_locked>0)
				if(sem_post(_sem)==0)
					_locked--;
			return -errno;
		}
	private:
		sem_t *_sem;
		int _locked;
};

class ThreadMutexAutoLock : public AutoLock
{
	public:
		ThreadMutexAutoLock(pthread_mutex_t *lock=nullptr,bool locked=false);
		~ThreadMutexAutoLock();
		virtual int lock()noexcept override;
		virtual int unlock()noexcept override;
	private:
		pthread_mutex_t *_lock;
		bool _locked;
};

#endif
