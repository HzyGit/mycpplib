#include "mempool.h"
#include "autolock.h"

#include <iostream>
void *BufMemPool::allocate(size_t size)
{
	if(size>_item_size)
		return nullptr;
	SemAutoLock lock(&_sem);
	lock.lock();
	if(get_free_item_num()==0)
		return nullptr;
	void *addr=nullptr;
	if(_index[_head]>=_item_num)
		return nullptr;
	if(is_free_unit(_index[_head]))
	{
		addr=get_unit_addr(_index[_head]);
		*(get_unit_index1(_index[_head]))=_index[_head];
	}
	_head=(_head+1)%(_item_num+1);
	return addr;
}

void BufMemPool::deallocate(void* addr)
{
	int i=addr_to_index(addr);
	if(i>=_item_num)
		return;
	if(i!=*(get_unit_index0(addr)))
		return;
	SemAutoLock lock(&_sem);
	lock.lock();
	if(get_free_item_num()==get_max_item_num())
		return;
	if(is_busy_unit(i))
	{
		*(get_unit_index1(addr))=0;
		_index[_tail]=i;
		_tail=(_tail+1)%(_item_num+1);
	}
}

/// 内存批量分配与释放
size_t BufMemPool::allocate(void**addr ,size_t size,size_t len)
{
	if(nullptr==addr)
		return 0;
	if(get_free_item_num()==0||size>_item_size)
		return 0;
	size_t alloc_num=0;
	int minlen=len>=get_free_item_num()?get_free_item_num():len;
	SemAutoLock lock(&_sem);
	lock.lock();
	for(int i=0;i<minlen;i++)
	{
		int cur=_index[_head];
		if(cur>=_item_num)
		{
			_head=(_head+1)%(_item_num+1);
			continue;
		}
		if(is_free_unit(cur))
		{
			*(get_unit_index1(cur))=cur;
			addr[alloc_num++]=get_unit_addr(cur);
			_head=(_head+1)%(_item_num+1);
		}
	}
	lock.unlock();
	return alloc_num;
}

size_t BufMemPool::deallocate(void **addr,size_t len)
{
	if(nullptr==addr)
		return 0;
	int left_len=get_max_item_num()-get_free_item_num();
	int minlen=left_len>=len?len:left_len;
	/// 加锁
	SemAutoLock lock(&_sem);
	/// 释放内存
	int dealloc_num=0;
	lock.lock();
	for(int i=0;i<minlen;++i)
	{
		int cur=addr_to_index(addr[i]);
		if(cur>=_item_num)
			continue;
		if(is_busy_unit(cur))
		{
			*get_unit_index1(cur)=0;
			_index[_tail]=cur;
			_tail=(_tail+1)%(_item_num+1);
		}
	}
	return minlen;
}

/// 支持的最大内存数
size_t BufMemPool::get_max_item_num()
{
	return _item_num;
}

/// 返回剩余内存数
size_t BufMemPool::get_free_item_num()
{
	return (_tail-_head+_item_num+1)%(_item_num+1);
}
size_t BufMemPool::get_item_size()
{
	return _item_size;
}

#ifdef GTEST_TEST
#include <gtest/gtest.h>
class TestBufMemPool : public testing::Test
{
	protected:
		void SetUp()
		{
			mem=new(buf) BufMemPool(1024,sizeof(int));
			size=(1024-sizeof(BufMemPool)-sizeof(size_t))/(sizeof(int)*3+sizeof(size_t));
		}
		void TearDown()
		{
		}
		BufMemPool *mem;
		char buf[1024];
		int size;
};

TEST_F(TestBufMemPool,TestContruct)
{
	EXPECT_EQ(size,mem->get_max_item_num());
	EXPECT_EQ(size,mem->get_free_item_num());
	EXPECT_EQ(sizeof(int),mem->get_item_size());
/// 异常构造
	BufMemPool mem0;
	EXPECT_EQ(0,mem0.get_max_item_num());
	EXPECT_EQ(0,mem0.get_free_item_num());
	EXPECT_EQ(0,mem0.get_item_size());
	
	char t[2];
	BufMemPool *mem1=new(t) BufMemPool(2,sizeof(int));
	EXPECT_EQ(0,mem1->get_max_item_num());
	EXPECT_EQ(0,mem1->get_free_item_num());
	EXPECT_EQ(4,mem1->get_item_size());
}

TEST_F(TestBufMemPool,TestSimpleAlloc)
{
	int *p=static_cast<int*>(mem->allocate(sizeof(int)));
	char *c=static_cast<char*>(mem->allocate(sizeof(char)));
	int *q=static_cast<int*>(mem->allocate(sizeof(int)));
	EXPECT_NE(nullptr,p);
	EXPECT_NE(nullptr,q);
	EXPECT_NE(nullptr,c);
	ASSERT_EQ(size,mem->get_max_item_num());
	ASSERT_EQ(size-3,mem->get_free_item_num());
	*p=*q=1230;
	*c='a';
	EXPECT_EQ(1230,*p);
	EXPECT_EQ(1230,*q);
	EXPECT_EQ('a',*c);
	/// 校验index0,index1
	EXPECT_EQ(p[-1],p[1]);
	EXPECT_EQ(q[-1],q[1]);
	/// 校验free_size
	EXPECT_EQ(size,mem->get_max_item_num());
	mem->deallocate(c);
	EXPECT_EQ(size-2,mem->get_free_item_num());
	mem->deallocate(p);
	EXPECT_EQ(size-1,mem->get_free_item_num());
	mem->deallocate(q);
	EXPECT_EQ(size,mem->get_free_item_num());
	mem->deallocate(p);
	EXPECT_EQ(size,mem->get_free_item_num());
}

TEST_F(TestBufMemPool,TestSimpleBatchAlloc)
{
	int len=3;
	int **q=new int*[len];
	EXPECT_EQ(len,mem->allocate((void**)q,sizeof(int),len));
	for(int i=0;i<len;i++)
	{
		int *t=q[i];
		EXPECT_EQ(t[-1],t[1]);
	}
	EXPECT_EQ(size,mem->get_max_item_num());
	EXPECT_EQ(size-len,mem->get_free_item_num());
	mem->deallocate((void**)q,len);
	EXPECT_EQ(size,mem->get_max_item_num());
	EXPECT_EQ(size,mem->get_free_item_num());
	delete[] q;
}

#endif
