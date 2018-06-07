#pragma once

#include <cstddef>
#include <vector>
#include <array>
#include <mutex>
#include <atomic>
#include <shared_mutex>

template<typename T, size_t size>
class DoubleBufferArray {
public:
	DoubleBufferArray();

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

	//std::atomic_int writeUsers;
	//std::atomic_bool swapInProgress = false;
	std::shared_mutex writeLock;
};

template<typename T, size_t size>
DoubleBufferArray<T, size>::DoubleBufferArray() :
	which(false)
{

}


template<typename T, size_t size>
T DoubleBufferArray<T, size>::Read(int index)
{
	std::shared_lock<std::shared_mutex> lock(writeLock);
	return currentRead[index];
}

template<typename T, size_t size>
std::array<T, size>* DoubleBufferArray<T, size>::ReadAll()
{
	std::shared_lock<std::shared_mutex> lock(writeLock);
	return currentRead;
}

template<typename T, size_t size>
void DoubleBufferArray<T, size>::Write(int index, T data)
{
	//while (swapInProgress) { } //spin-lock (I know its bad)
	std::unique_lock<std::shared_mutex> lock(writeLock);
	//this->writeUsers++;

	currentWrite[index] = data;

	//this->writeUsers--;
}

template<typename T, size_t size>
void DoubleBufferArray<T, size>::Swap()
{
	//swapInProgress = true;
	//while (writeUsers > 0) {} //spin-lock (I know its bad)
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
	//swapInProgress = false;
}
