#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H
#pragma once
#include<climits>
#include<cstddef>
//#include<stddef.h>
template<typename T, size_t Blocksize = 4096>
class MemoryPool
{
public:
	typedef       T				 value_type;
	typedef       T*		     pointer;
	typedef       T&			 reference;
	typedef const T*			 const_pointer;
	typedef const T&			 const_reference;
	typedef       size_t		 size_type;
	typedef		  ptrdiff_t		 difference_type;
	// ptrdiff_t: 是两个指针相减的结果的无符号整数类型
	// size_t : 是sizeof操作符的结构的无符号类型

	template<typename U>struct rebind {
		typedef MemoryPool<U>other;
	};
	/*构造&析构函数*/
	MemoryPool() noexcept;
	MemoryPool(const MemoryPool&memorypool)noexcept;
	template<class U>MemoryPool(const MemoryPool<U>&memorypool)noexcept;
	~MemoryPool()noexcept;
	//返回地址
	pointer address(reference x)const noexcept;
	const_pointer address(const_reference x)const noexcept;

	//返回最多容纳元素数量
	size_type max_size()const noexcept;
	//自带 申请内存 和 释放内存 的构造与析构
	template <class... Args> pointer newElement(Args&&... args);
	void deleteElement(pointer p);

	pointer allocate(size_type n = 1, const_pointer hint = 0);						//分配一个内存空间
	void deallocate(pointer p, size_type n = 1);									//回收一个内存空间			
	//内存池元素的构造&析构
	template <class U, class... Args>void construct(U* p, Args&&... args);
	template <class U> void destroy(U* p);

private:
	//内存池为线性链表，分Blok块储存，Slot_存放元素或next指针
	union Slot_
	{
		value_type element;
		Slot_* next;
	};
	typedef char*       data_pointer_;      //char*指向内存池首地址
	typedef Slot_       slot_type_;
	typedef Slot_*		slot_pointer_;

	slot_pointer_	currentBlock_;			//内存块链表头指针
	slot_pointer_	currentSlot_;			//元素链表头指针
	slot_pointer_	lastSlot_;				//可存放元素的最后的指针
	slot_pointer_	freeSlots_;				//元素构造后释放掉的内存链表头指针

	size_type padPointer(data_pointer_ p, size_type align) const noexcept;  // 计算对齐所需空间
	void allocateBlock();													//申请将内存块放进内存池

};


#endif // !MEMORY_POOL_H

