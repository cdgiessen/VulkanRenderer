#pragma once

#include <assert.h>
#include <limits.h>
#include <mutex>
#include <stdint.h>
#include <type_traits>
#include <vector>

template <typename T>
// makes sure uint16_t is a integer type.
class Pool
{
	struct Node
	{
		Node () { data = 0; }
		Node (uint32_t next, uint8_t gen)
		{
			uint32_t gen32 = gen << 24;
			this->data = (0xFFFFFF & next) | (gen32);
		}

		uint32_t next () const { return 0xFFFFFF & data; }
		uint8_t gen () const { return data >> 24; }

		void set_next_node (uint32_t next_node)
		{
			uint32_t i32 = (0xFFFFFF & next_node);
			data = gen () | i32;
		}

		void increment_gen ()
		{
			uint8_t g = gen ();
			g++;
			uint32_t gen32 = g << 24;
			data = g | next ();
		}

		private:
		uint32_t data = 0;
	};

	public:
	struct ID
	{
		ID () { data = 0; }
		ID (uint32_t id, uint8_t gen)
		{
			uint32_t gen32 = gen << 24;
			data = (0xFFFFFF & id) | (gen32);
		}

		uint32_t id () const { return 0xFFFFFF & data; }
		uint8_t gen () const { return data >> 24; }

		private:
		uint32_t data = 0;
	};

	Pool (uint16_t size = UINT16_MAX)
	{
		data.reserve (size);
		for (uint32_t i = 0; i < size - 1; i++)
		{
			data.emplace_back (i + 1);
		}
		data.emplace_back (0);
		next_node_id = 0;
	}


	ID allocate ()
	{
		std::lock_guard<std::mutex> l (lock);

		// pop next node of free_list
		uint32_t ret_id = next_node_id;
		uint8_t ret_gen = data[next_node_id].node.gen ();

		next_node_id = data[next_node_id].node.next ();
		in_use++;
		return ID{ ret_id, ret_gen };
	}

	void deallocate (ID const& item)
	{
		std::lock_guard<std::mutex> l (lock);

		// push next node at begining of free_list
		data[next_node_id].node.set_next_node (next_node_id);
		next_node_id = item.id ();
		data[next_node_id].node.increment_gen ();

		in_use--;
	}

	T& at (ID const& index)
	{
		assert (index.gen () == data[next_node_id].node.gen ());
		return data.at (index.id ()).item;
	}

	uint32_t current_load () { return in_use; }

	private:
	union union_data_node {
		T item;
		Node node;
		union_data_node (uint32_t next_node) { node = Node{ next_node, 0 }; }
	};
	std::vector<union_data_node> data;

	uint32_t next_node_id;

	std::mutex lock;
	uint32_t in_use = 0;
};

void Test_MemoryPool ();