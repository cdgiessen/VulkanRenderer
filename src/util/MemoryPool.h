#pragma once

#include <foonathan/memory/container.hpp> // vector, list, list_node_size
#include <foonathan/memory/memory_pool.hpp> // memory_pool

#include <atomic>

template<typename T, size_t count>
class MemoryPool {
public:
	MemoryPool();

	T * allocate();
	void deallocate(T* del);
private:

};

template<typename T, size_t count>
MemoryPool<T, count>::MemoryPool()

{
}

template<typename T, size_t count>
T* MemoryPool<T, count>::allocate() {
	return new T();
}

template<typename T, size_t count>
void MemoryPool<T, count>::deallocate(T* del) {
	delete del;
}