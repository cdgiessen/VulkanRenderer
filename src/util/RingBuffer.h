#pragma once


#include <vector>
#include <mutex>
#include <optional>

template <typename T>
class RingBuffer {
public:
	RingBuffer();

	T top();
	void pop();
	std::optional<T> pop_if();

	void push(T data);

private:





	std::vector<T> buffer;


};