#ifndef _UNIQUE_FD_H
#define _UNIQUE_FD_H

#include <stdio.h>
#include <unistd.h>

class unique_fd 
{
	public:
		inline unique_fd();
		inline unique_fd(int fd);
		inline ~unique_fd();
		unique_fd(const unique_fd& fd)=delete;
		unique_fd operator = (const unique_fd& fd)=delete;

		inline unique_fd(unique_fd &&fd);
		inline unique_fd& operator=(unique_fd &&fd);
	public:
		inline operator bool ()const;
		inline void reset(int fd);
		inline int get()const;
		inline int release();
	private: 
		int _fd;
};

unique_fd::unique_fd()
	:_fd(-1)
{
}

unique_fd::unique_fd(int fd)
	:_fd(fd)
{
}

unique_fd::~unique_fd()
{
	if (this->_fd >= 0)
		close(this->_fd);
	this->_fd=-1;
}

unique_fd::unique_fd(unique_fd &&fd)
{
	this->_fd=fd._fd;
	fd._fd=-1;
}

unique_fd& unique_fd::operator=(unique_fd &&fd)
{
	if(this==&fd)
		return *this;
	this->_fd=fd._fd;
	fd._fd=-1;
}

inline unique_fd::operator bool ()const
{
	return _fd>=0;
}

void unique_fd::reset(int fd)
{
	close(this->_fd);
	this->_fd = fd;
}

int unique_fd::get()const
{
	return this->_fd;
}

int unique_fd::release()
{
	int temp = this->_fd;
	this->_fd = -1;
	return temp;
}
#endif
