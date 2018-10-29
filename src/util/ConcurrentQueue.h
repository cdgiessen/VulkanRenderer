#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <algorithm>
#include <condition_variable>
#include <optional>


template <typename T>
class ConcurrentQueue
{
public:
	ConcurrentQueue();
	~ConcurrentQueue();

	void pop();

	//Optionally returns the front value if it exists, else returns nothing
	std::optional<T> pop_if();

	void push_back(const T& item);

	void push_back(T&& item);

	void remove(T& item);

	int size();

	bool empty();

	void wait_on_value();

	void notify_all();

private:
	std::deque<T> m_queue;
	std::mutex m_mutex;
	std::condition_variable m_cond;
	std::mutex m_cond_lock;
};


template <typename T>
ConcurrentQueue<T>::ConcurrentQueue() {};

template <typename T>
ConcurrentQueue<T>::~ConcurrentQueue() {};

template <typename T>
void ConcurrentQueue<T>::pop()
{
	std::unique_lock<std::mutex> mlock(m_mutex);
	if (!m_queue.empty())
		m_queue.pop_front();
}

template <typename T>
std::optional<T> ConcurrentQueue<T>::pop_if()
{
	std::unique_lock<std::mutex> mlock(m_mutex);
	if (!m_queue.empty()) {
		auto ret = m_queue.front();
		m_queue.pop_front();
		return std::move(ret);
	}
	return {};
}

template <typename T>
void ConcurrentQueue<T>::push_back(const T& item)
{

	std::unique_lock<std::mutex> mlock(m_mutex);
	m_queue.push_back(item);
	m_cond.notify_one(); // notify one waiting thread

}

template <typename T>
void ConcurrentQueue<T>::push_back(T&& item)
{

	std::unique_lock<std::mutex> mlock(m_mutex);
	m_queue.push_back(std::move(item));
	m_cond.notify_one(); // notify one waiting thread

}

template <typename T>
void ConcurrentQueue<T>::remove(T& item) {
	std::unique_lock<std::mutex> mlock(m_mutex);
	auto iter = std::find(std::begin(m_queue), std::end(m_queue), item);
	m_queue.erase(iter);
}

template <typename T>
int ConcurrentQueue<T>::size()
{
	std::unique_lock<std::mutex> mlock(m_mutex);
	return (int)m_queue.size();
}

template <typename T>
bool ConcurrentQueue<T>::empty()
{
	std::unique_lock<std::mutex> mlock(m_mutex);
	return m_queue.empty();
}

template <typename T>
void ConcurrentQueue<T>::wait_on_value() {
	std::unique_lock<std::mutex> lk(m_cond_lock);
	m_cond.wait(lk);
}

template <typename T>
void ConcurrentQueue<T>::notify_all() {
	std::unique_lock<std::mutex> lk(m_cond_lock);
	m_cond.notify_all();
}