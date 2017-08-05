#include <vector>

template<typename T>
class MemoryPool {
public:
	
	typedef T* pointer;

	MemoryPool(int chunkCount);
	~MemoryPool();

	pointer allocate();
	void deallocate(pointer chunkToBeFreed);
	void soft_deallocate(pointer chunkToBeFreed); //for when you know you are going to be doing a lot of erasing (so it can be done all at once later)
	void flush();


	pointer GetDataPtr();
	int ChunksUsed();
	int ChunksFree();
	int MaxChunks();

private:
	std::vector<T> data;
	
	int maxChunks = 0;
	int usedChunks = 0;
	
	pointer freeChunk;

	std::vector<pointer> freeChunkList;

};

template<typename T>
MemoryPool<T>::MemoryPool(int chunkCount)
{
	this->maxChunks = static_cast<uint32_t>(chunkCount);
	this->data.resize(chunkCount);
	this->freeChunk = data.data();
}

template<typename T>
MemoryPool<T>::~MemoryPool()
{
	
}

template<typename T>
T* MemoryPool<T>::allocate() {
	if (maxChunks == usedChunks) {
		std::cerr << "Memory Pool Empty!" << std::endl;
		return nullptr;
	}
	else {
		T* dataToReturn = freeChunk;
		this->freeChunk++;
		this->usedChunks++;
		return dataToReturn;
	}
}

template<typename T>
void MemoryPool<T>::deallocate(pointer chunkToBeFreed) {
	this->usedChunks--;
	std::swap(chunkToBeFreed, freeChunk);
}


template<typename T>
void MemoryPool<T>::soft_deallocate(pointer chunkToBeFreed) {
	this->freeChunkList.push_back(chunkToBeFreed);
}

template<typename T>
void MemoryPool<T>::flush() {
	for (pointer ptr : freeChunkList) {
		this->freeChunk --;
		std::swap(ptr, freeChunk);
		this->usedChunks--;
	}
}

template<typename T>
T* MemoryPool<T>::GetDataPtr() {
	return data.data();
}

template<typename T>
int MemoryPool<T>::ChunksUsed() {
	return usedChunks;
}

template<typename T>
int MemoryPool<T>::ChunksFree() {
	return maxChunks - usedChunks;
}

template<typename T>
int MemoryPool<T>::MaxChunks() {
	return maxChunks;
}