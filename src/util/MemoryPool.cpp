#include "MemoryPool.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <random>
#include <stdlib.h>
#include <unordered_map>

#include "core/Logger.h"

struct Vec3
{
	float x, y, z;
};

void Test_MemoryPool ()
{

	int pool_size = 5000;

	Pool<float> p_f (pool_size);

	std::vector<int> nums;
	for (int i = 0; i < pool_size; i++)
	{
		nums.push_back (i);
	}

	std::random_device rd;
	std::mt19937 g (rd ());

	std::shuffle (nums.begin (), nums.end (), g);

	std::unordered_map<int, Pool<float>::ID> order;
	float x = 0;
	for (int i = 0; i < pool_size; i++)
	{

		Pool<float>::ID id = p_f.allocate ();
		p_f.at (id) = x;
		x += 0.01f;

		order[nums.at (i)] = id;
	}
	for (auto& [i, id] : order)
	{
		p_f.deallocate (id);
	}

	Log.Debug (fmt::format ("pool size = {}", p_f.current_load ()));
	Log.Debug (" \n");
}
