#pragma once


#include <atomic>
#include <vector>
#include <variant>
#include <algorithm>

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