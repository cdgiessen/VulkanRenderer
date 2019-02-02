#pragma once

#include <assert.h>
#include <mutex>
#include <vector>
#include <stdint.h>
#include <limits.h>
#include <type_traits>

template<typename T>
	//makes sure uint16_t is a integer type.
class Pool {

	struct Node {
		uint16_t next;
		uint16_t generation = 0;
	};

public:
	struct ID {
		uint16_t id;
		uint16_t generation = 0;
	};

	Pool (uint16_t size = UINT16_MAX){
		data.resize(size);
		free_list.resize(size);
		for(uint16_t i = 0; i < size - 1; i++){
			free_list[i].next = i + 1;
		}
		free_list[size - 1].next = 0;
		next_node_id = 0;
	}

	
	ID allocate(){
		std::lock_guard<std::mutex> l(lock);

		//pop next node of free_list
		uint16_t ret_id = next_node_id;
		uint16_t ret_gen = free_list[next_node_id].generation;

		next_node_id = free_list[next_node_id].next;
		in_use++;
		return {ret_id, ret_gen};
	}

	void deallocate(ID item){
		std::lock_guard<std::mutex> l(lock);

		//push next node at begining of free_list
		free_list[item.id].next = next_node_id;
		next_node_id = item.id;
		free_list[item.id].generation++;
		in_use--;
	}

	T& at(ID index){
		assert(index.generation == free_list[index.id].gen);
		return data.at(index.id);
	}

private:

	std::vector<T> data;
	std::vector<Node> free_list;
	
	uint16_t next_node_id;
	
	std::mutex lock;
	uint16_t in_use = 0;
};

void Test_MemoryPool();