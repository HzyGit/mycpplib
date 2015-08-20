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
		SemAutoLock(sem_t *sem=nullptr,bool locked=false)
			:_sem(sem),_is_locked(locked)
		{
		}
		virtual ~SemAutoLock()
		{
			if(nullptr!=_sem&&_is_locked)
				sem_post(_sem);
		}

		SemAutoLock(const SemAutoLock &&sem)=delete;
		SemAutoLock& operator=(const SemAutoLock& lock)=delete;
	public:
		virtual int lock()noexcept override
		{
			if(nullptr==_sem)
				return -EINVAL;
			int res=sem_wait(_sem);
			if(res==0)
				_is_locked=true;
			return -errno;
		}

		virtual int unlock()noexcept override
		{
			if(nullptr==_sem)
				return -EINVAL;
			int res=sem_post(_sem);
			if(res==0)
				_is_locked=false;
			return -errno;
		}
	private:
		sem_t *_sem;
		bool _is_locked;
};

#endif
