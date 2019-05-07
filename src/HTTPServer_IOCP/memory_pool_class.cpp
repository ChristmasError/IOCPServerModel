#pragma once 

#include"memory_pool_class.h"

template<typename T, size_t Blocksize>
inline typename MemoryPool<T, Blocksize>::size_type 
MemoryPool<T, Blocksize>::padPointer(data_pointer_ p, size_type align) const noexcept
{
	size_t result = reinterpret_cast<size_t>(p);
	return ((align - result) % align);
}
// 构造函数
template<typename T, size_t Blocksize>
MemoryPool<T, Blocksize>::MemoryPool() noexcept
{
	BlockListHead_ = 0;
	SlotListHead_  = 0;
	lastSlot_      = 0;
	FreeSlotHead   = 0;
} 

// 析构函数，delete内存池中所有的block
template<typename T, size_t Blocksize>
MemoryPool<T, Blocksize>::~MemoryPool()
noexcept
{
	slot_pointer_ curr = BlockListHead_;
	while (curr != nullptr)//curr!=NULL
	{
		slot_pointer_ prev = curr->next;
		// 转化为void* 只释放空间不调用析构函数
		operator delete (reinterpret_cast<void*>(curr));
		curr = prev;
	}
}
// 返回地址
template<typename T, size_t Blocksize >
inline typename MemoryPool<T, Blocksize>::pointer
MemoryPool<T, Blocksize>::address(reference x)
const noexcept
{
	return &x;
}
// 返回地址 const 重载版本
template<typename T, size_t Blocksize>
inline typename MemoryPool<T, Blocksize>::const_pointer
MemoryPool<T, Blocksize>::address(const_reference x)
const noexcept
{
	return &x;
}
// 申请一块空闲的Block放入内存池
template<typename T, size_t Blocksize>
void MemoryPool<T, Blocksize>::allocateBlock()
{
	// operator new（）申请一块Blocksize大小的内存
	data_pointer_ newBlock = reinterpret_cast<data_pointer_>(operator new(Blocksize));
	// 新Block链表头 BlockListHead_
	reinterpret_cast<slot_pointer_>(newBlock)->next = BlockListHead_;
	BlockListHead_ = reinterpret_cast<slot_pointer_>(newBlock);
	// 计算为了对齐应该空出多少位置
	data_pointer_ body = newBlock + sizeof(slot_pointer_);
	size_type bodyPadding = padPointer(body, sizeof(slot_type_));
	// currentslot_ 为该 block 开始的地方加上 bodypadding 个 char* 空间
	SlotListHead_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
	// 计算最后一个能放下slot_type_的位置
	lastSlot_ = reinterpret_cast<slot_pointer_>(newBlock + Blocksize - sizeof(slot_type_) + 1);
}

// 返回指向分配新元素所需内存的指针
template<typename T, size_t Blocksize>
inline typename MemoryPool<T, Blocksize>::pointer
MemoryPool<T, Blocksize>::allocate(size_type n, const_pointer hint)
{
	// 如果freeSlot_非空，就在freeSlot_中去取内存
	if (FreeSlotHead != nullptr)
	{
		pointer pElement = reinterpret_cast<pointer>(FreeSlotHead);
		FreeSlotHead = FreeSlotHead->next;
		return pElement;
	}
	else
	{
		// Block中内存用完的情况
		if (SlotListHead_ >= lastSlot_)
			allocateBlock();
		return reinterpret_cast<pointer>(SlotListHead_++);
	}
}
// 将元素内存归还free内存链表
template<typename T, size_t Blocksize>
inline void
MemoryPool<T, Blocksize>::deallocate(pointer p, size_type n)
{
	if (p != nullptr)
	{
		// 转换成slot_pointer_指针，next指向freeslots_链表
		reinterpret_cast<slot_pointer_>(p)->next = FreeSlotHead;
		FreeSlotHead = reinterpret_cast<slot_pointer_>(p);
	}
}

// 计算最大元素上限数 
template<typename T, size_t Blocksize>
inline typename MemoryPool<T, Blocksize>::size_type
MemoryPool<T, Blocksize>::max_size()
const noexcept
{
	size_type maxBlocks = -1 / Blocksize;
	return ((Blocksize - sizeof(data_pointer_)) / sizeof(slot_type_)*maxBlocks);
}

// 在已分配内存上构造对象
template <typename T, size_t BlockSize>
template <class U, class... Args>
inline void
MemoryPool<T, BlockSize>::construct(U* p, Args&&... args)
{
	new (p) U(std::forward<Args>(args)...);
}

// 销毁对象
template<typename T, size_t Blocksize>
template <class U>
inline void
MemoryPool<T, Blocksize>::destroy(U* p)
{
	p->~U();
	// 调用析构
}

// 创建新元素
template<typename T, size_t Blocksize>
template <class... Args>
inline typename  MemoryPool<T, Blocksize>::pointer
MemoryPool<T, Blocksize>::newElement(Args&&... args)
{
	// 申请内存
	pointer pElement = allocate();
	// 在内存上构造对象
	construct<value_type>(pElement, std::forward<Args>(args)...);
	return pElement;
}
// 删除元素
template<typename T, size_t Blocksize>
inline void
MemoryPool<T, Blocksize>::deleteElement(pointer p)
{
	if (p != nullptr)
	{
		p->~value_type();
		deallocate(p);
	}
}
