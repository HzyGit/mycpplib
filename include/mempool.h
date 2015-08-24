#ifndef _MEMPOOL_H
#define _MEMPOOL_H

#include <cstddef>
#include <stdexcept>

#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "unix_error.h"
#include "unique_fd.h"

class MemPool
{
	public:
		virtual ~MemPool(){}
		/// 内存分配与释放
		virtual void *allocate(size_t size)=0;
		virtual void deallocate(void*)=0;
		/// 内存批量分配与释放
		virtual size_t allocate(void**addr ,size_t size,size_t len)=0;
		virtual size_t deallocate(void **addr,size_t len)=0;
		/// 支持的最大内存数
		virtual size_t get_max_item_num()=0;
		/// 返回剩余内存数
		virtual size_t get_free_item_num()=0;
		///@brief 获取
		virtual size_t get_item_size()=0;
};

class MemPoolError: std::bad_alloc
{
	public:
		MemPoolError(const char *err)
			:_err_str(err)
		{ }
		MemPoolError(const std::string &err)
			:_err_str(err)
		{ }
		virtual const char *what()const noexcept
		{
			return _err_str.c_str();
		}
	private:
		std::string _err_str;
};

class BufMemPool : public MemPool
{
	public:
		BufMemPool()
			:_len(0),_item_size(0),_item_num(0),_buf(nullptr)
		{
			sem_init(&_sem,1,1);
		}
		BufMemPool(size_t len,size_t item_size)
			:_len(len),_item_size(item_size)
		{
			ssize_t left=_len-sizeof(BufMemPool)-sizeof(size_t);
			if(left<0)
				throw MemPoolError("buf_len is too short to contruct BufMemPool");
			_item_num=left/(unit_size()+sizeof(size_t));
			/// 初始化index
			for(int i=0;i<_item_num;++i)
				_index[i]=i;
			/// 初始化buf
			if(_item_num==0)
				_buf=nullptr;
			else
				_buf=static_cast<char*>(static_cast<void*>(_index+_item_num+1));
			for(int i=0;i<_item_num;++i)
			{
				init_unit(i);
			}
			/// 初始化队列相关标志
			if(_item_num>0)
			{
				sem_init(&_sem,1,1);
				_head=0;
				_tail=_item_num;
			}
		}

		BufMemPool(const BufMemPool &vec)=delete;
		BufMemPool& operator=(const BufMemPool &vec)=delete;
		//void* operator new(size_t size)=delete;
	public:
		virtual void *allocate(size_t size)override;
		virtual void deallocate(void* addr)override;
		/// 内存批量分配与释放
		virtual size_t allocate(void**addr ,size_t size,size_t len)override;
		virtual size_t deallocate(void **addr,size_t len)override;
		/// 支持的最大内存数
		virtual size_t get_max_item_num()override;
		/// 返回剩余内存数
		virtual size_t get_free_item_num()override;
		/// @brief 获取item大小
		virtual size_t get_item_size()override;
	private:
		/// unit单元大小，unit单元由[index0,mem,index1]组成
		size_t unit_size()const
		{
			return sizeof(int)*2+_item_size;
		}
		/// 获取unit中index
		int* get_unit_index0(int i)
		{
			char *h=_buf+unit_size()*i;
			return static_cast<int*>(static_cast<void*>(h));
		}
		int* get_unit_index0(void *addr)
		{
			int *p=static_cast<int*>(addr);
			return p-1;
		}
		int* get_unit_index1(int i)
		{
			char *h=_buf+unit_size()*i+_item_size+sizeof(int);
			return static_cast<int*>(static_cast<void*>(h));
		}
		int* get_unit_index1(void *addr)
		{
			void *p=static_cast<char*>(addr)+_item_size;
			return static_cast<int*>(p);
		}
		void* get_unit_addr(int i)
		{
			char *h=_buf+unit_size()*i;
			return h+sizeof(int);
		}

		int addr_to_index(void *addr)
		{
			char *p=static_cast<char*>(addr);
			p-=sizeof(int);
			return (p-_buf)/unit_size();
		}
		/// 初始化unit
		void init_unit(int i)
		{
			*get_unit_index0(i)=i;
			*get_unit_index1(i)=0;
		}
		bool is_valid_unit(int i)
		{
			return (*get_unit_index0(i))==i;
		}

		bool is_valid_unit(void *addr)
		{
			return addr_to_index(addr)==*get_unit_index0(addr);
		}

		bool is_free_unit(int i)
		{
			return is_valid_unit(i)&&(*get_unit_index1(i))==0;
		}
		bool is_busy_unit(int i)
		{
			return is_valid_unit(i)&&(*get_unit_index1(i))==i;
		}

	private:
		size_t _len;
		size_t _item_size;
		size_t _item_num;
		sem_t _sem;
		int _head,_tail;
		char *_buf;
		size_t _index[];
};

class ShmMemVecPool : public MemPool
{
	public:
		// 构造/析构
		ShmMemVecPool(void *buf=nullptr,size_t buf_len=0,size_t item_size=4,bool is_unmap=true)
			:_buf(buf),_buf_len(buf_len),_is_unmap(is_unmap)
		{
			char temp[10];
			if(nullptr==_buf)
			{
				_buf_len=0;
				_buf=temp;
			}
			_mem_pool=new(buf)BufMemPool(_buf_len,item_size);
		}
		ShmMemVecPool(const char *shmpath,size_t item_size=4,bool is_unmap=true)
			:_is_unmap(is_unmap)
		{
			unique_fd fd(shm_open(shmpath,O_RDWR,0));
			if(!fd)
				throw UnixError(std::string("shm_open ")+shmpath+" error");
			struct stat st;
			fstat(fd.get(),&st);
			_buf_len=st.st_size;
			_mem_pool=new(_buf)BufMemPool(_buf_len,item_size);
		}
		ShmMemVecPool(const ShmMemVecPool &mem)=delete;
		ShmMemVecPool & operator=(const ShmMemVecPool &mem)=delete;
		~ShmMemVecPool()
		{
			_mem_pool=nullptr;
			if(_is_unmap&&nullptr!=_buf)
				munmap(_buf,_buf_len);
		}
	public:
		/// 内存分配与释放
		virtual void *allocate(size_t size) override;
		virtual void deallocate(void*)override;
		/// 内存批量分配与释放
		virtual size_t allocate(void**addr ,size_t size,size_t len)override;
		virtual size_t deallocate(void **addr,size_t size)override;
		/// 支持的最大内存数
		virtual size_t get_max_item_num()override;
		/// 返回剩余内存数
		virtual size_t get_free_item_num()override;
		/// @brief 获取item大小
		virtual size_t get_item_size()override;
	private:
		bool _is_unmap;   ///< 在对象析构时释放自动unmap内存
		void *_buf;       ///< 共享内存地址
		size_t _buf_len;  ///< 共享内存长度
		BufMemPool *_mem_pool;    ///< buf内存池
};

#endif
