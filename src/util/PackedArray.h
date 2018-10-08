#pragma once

#include <array>
#include <mutex>

template <typename T, size_t size = 1024> using PackedArrayPool = std::array<T, size>;

// using ID = unsigned;

// unsigned int MAX_OBJECTS = 64 * 1024;
// unsigned int INDEX_MASK = 0xffff;
// unsigned int NEW_OBJECT_ID_ADD = 0x10000;

// struct Index
// {
// 	ID id;
// 	unsigned short index;
// 	unsigned short next;
// };

// template <typename T> class PackedArrayPool
// {
// 	public:
// 	PackedArrayPool ()
// 	{
// 		std::lock_guard<std::mutex> lg (lock);
// 		_num_objects = 0;
// 		for (unsigned i = 0; i < MAX_OBJECTS; ++i)
// 		{
// 			indices[i].id = i;
// 			indices[i].next = i + 1;
// 		}
// 		_freelist_dequeue = 0;
// 		_freelist_enqueue = MAX_OBJECTS - 1;
// 	}

// 	template <typename T> bool has (ID id)
// 	{
// 		std::lock_guard<std::mutex> lg (lock);
// 		Index& in = indices[id & INDEX_MASK];
// 		return in.id == id && in.index != USHRT_MAX;
// 	}

// 	template <typename T> T& lookup (ID id)
// 	{
// 		std::lock_guard<std::mutex> lg (lock);
// 		return objects[indices[id & INDEX_MASK].index];
// 	}

// 	template <typename T> ID add ()
// 	{
// 		std::lock_guard<std::mutex> lg (lock);
// 		Index& in = indices[_freelist_dequeue];
// 		_freelist_dequeue = in.next;
// 		in.id += NEW_OBJECT_ID_ADD;
// 		in.index = _num_objects++;
// 		T& o = objects[in.index];
// 		o.id = in.id;
// 		return o.id;
// 	}

// 	template <typename T> void remove (ID id)
// 	{
// 		std::lock_guard<std::mutex> lg (lock);
// 		Index& in = indices[id & INDEX_MASK];

// 		T& o = objects[in.index];
// 		o = objects[--_num_objects];
// 		indices[o.id & INDEX_MASK].index = in.index;

// 		in.index = USHRT_MAX;
// 		indices[_freelist_enqueue].next = id & INDEX_MASK;
// 		_freelist_enqueue = id & INDEX_MASK;
// 	}

// 	private:
// 	unsigned _num_objects;
// 	std::array<T, MAX_OBJECTS> objects;
// 	std::array<Index, MAX_OBJECTS> indices;
// 	unsigned short _freelist_enqueue;
// 	unsigned short _freelist_dequeue;

// 	std::mutex lock;
// };