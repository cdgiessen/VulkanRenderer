#pragma once

#include <vector>
#include <utility>
#include <optional>
#include <algorithm>

template <typename T> class Registry
{
	public:
	size_t Add (T object)
	{
		reg.emplace_back (_id, std::move (object));
		size++;
		return _id++;
	}

	void Remove (size_t id)
	{
		auto p = std::lower_bound (std::begin (reg), std::end (reg), id, [] (auto const& a, auto const& b) {
			return a.first < b;
		});
		if (p == std::end (reg) || p->first != id || !p->second) return;
		p->second.reset ();
		size--;

		// compaction
		if (size < (reg.size () / 2))
		{
			reg.erase (std::remove_if (
			               std::begin (reg), std::end (reg), [] (auto const& e) { return !e.second; }),
			    std::end (reg));
		}
	}

	template <typename F> void apply (F func) const
	{
		for (auto& [k, v] : reg)
		{
			func (v);
		}
	}

	private:
	std::vector<std::pair<size_t, std::optional<T>>> reg;
	size_t size = 0;
	size_t _id = 0;
};