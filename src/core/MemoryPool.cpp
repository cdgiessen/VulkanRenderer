#include "MemoryPool.h"

template<typename T>
MemoryPool<T>::MemoryPool(int chunkCount):
	data(chunkCount)
{
	if (chunkCount > 1) {

		for (int i = 0; i < chunkCount - 1; i++) {
			data[i] = &(data[i + 1]);
		}
		freeChunk = &data[0];
		lastFreeChunk = &data[chunkCount - 1];
	}

}

template<typename T>
T* MemoryPool<T>::allocate() {
	if (usedCount == data.size()) {
		//increase pool size?
	}
	usedCount++;
	T* thingToReturn = freeChunk;
	freeChunk = &(*freeChunk);
	return thingToReturn;
}

template<typename T>
void MemoryPool<T>::deallocate(T* chunkToBeFreed) {
	if (usedCount == 0) {
		return;//nothing to deallocate!
	}
	usedCount--;
	*lastFreeChunk = chunkToBeFreed;
	lastFreeChunk = chunkToBeFreed;
}

