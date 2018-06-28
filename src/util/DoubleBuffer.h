#pragma once

#include <cstddef>
#include <vector>
#include <array>
#include <mutex>
#include <atomic>
#include <shared_mutex>

template<typename T, int size>
class DoubleBufferArray {
public:
	DoubleBufferArray();

	int Allocate();
	void Free(int index);

	//read element at index from current read buffer
	T Read(int index);

	//mostly for transfering to gpu
	std::array<T, size>* ReadAll();

	//write element at index in current write buffer, wait if swap is inprogress
	void Write(int index, T data);

	//Swap current read and write buffers, wait for all writes to finish
	void Swap();

private:

	std::atomic_bool which = false;
	std::array<T, size> a; //which == false, read a, write b;
	std::array<T, size> b; //which == true,  read b, write a;
	std::array<T, size>* currentRead; //ptr references
	std::array<T, size>* currentWrite;//ptr references
	std::array<bool, size> freeElems;
	int lastFreed;

	std::shared_mutex writeLock;
};

template<typename T, int size>
DoubleBufferArray<T, size>::DoubleBufferArray() :
	which(false)
{
	std::fill(std::begin(freeElems), std::end(freeElems), false);
}

template<typename T, int size>
int DoubleBufferArray<T, size>::Allocate()
{
	std::unique_lock<std::shared_mutex> lock(writeLock);
	for (int i = 0; i < size; i++)
		if (freeElems[i] == false) {
			freeElems[i] == true;
			return i;
		}
	throw std::runtime_error("Ran out of indicies to give!");
}

template<typename T, int size>
void DoubleBufferArray<T, size>::Free(int index)
{
	std::unique_lock<std::shared_mutex> lock(writeLock);

	if (freeElems[index] == true) {
		freeElems[index] == false;
	}
	else {
		throw std::runtime_error("Trying to free already freed indices!");
	}
}


template<typename T, int size>
T DoubleBufferArray<T, size>::Read(int index)
{
	std::shared_lock<std::shared_mutex> lock(writeLock);
	return (*currentRead)[index];
}

template<typename T, int size>
std::array<T, size>* DoubleBufferArray<T, size>::ReadAll()
{
	std::shared_lock<std::shared_mutex> lock(writeLock);
	return currentRead;
}

template<typename T, int size>
void DoubleBufferArray<T, size>::Write(int index, T data)
{

	std::unique_lock<std::shared_mutex> lock(writeLock);


	(*currentWrite)[index] = data;

}

template<typename T, int size>
void DoubleBufferArray<T, size>::Swap()
{

	std::unique_lock<std::shared_mutex> lock(writeLock);

	which = !which;

	if (which) {
		currentRead = &a;
		currentWrite = &b;
	}
	else {
		currentRead = &b;
		currentWrite = &a;
	}
}
