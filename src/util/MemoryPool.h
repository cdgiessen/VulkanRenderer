#pragma once

#include <vector>
#include <atomic>

template<typename T>
class MemoryPool {
public:
	MemoryPool(int count);

	T * allocate();
	void deallocate(T* del);

};

template<typename T>
MemoryPool<T>::MemoryPool(int count) {};

template<typename T>
T* MemoryPool<T>::allocate() {
	return new T();
}

template<typename T>
void MemoryPool<T>::deallocate(T* del) {
	delete del;
}