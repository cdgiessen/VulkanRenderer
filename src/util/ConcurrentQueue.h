#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>


template <typename T>
class ConcurrentQueue
{
public:
	ConcurrentQueue();
	~ConcurrentQueue();

	//Gets the front of the Queue, waits if there isn't anything
	T front();

	//Pops the front of the queue, if it exists
	void pop();

	//Optionally returns the front value if it exists, else returns nothing
	std::optional<T> pop_if();

	void push_back(const T& item);

	void push_back(T&& item);

	void remove(T& item);

	int size();

	bool empty();

private:
	std::deque<T> m_queue;
	std::mutex m_mutex;
	std::condition_variable m_cond;
};


template <typename T>
ConcurrentQueue<T>::ConcurrentQueue() {};

template <typename T>
ConcurrentQueue<T>::~ConcurrentQueue() {};

template <typename T>
T ConcurrentQueue<T>::front()
{
	std::unique_lock<std::mutex> mlock(m_mutex);
	while (m_queue.empty())
	{
		m_cond.wait(mlock);
	}
	return m_queue.front();
}

template <typename T>
void ConcurrentQueue<T>::pop()
{
	std::unique_lock<std::mutex> mlock(m_mutex);
	if (!m_queue.empty());
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
	mlock.unlock();     // unlock before notificiation to minimize mutex con
	m_cond.notify_one(); // notify one waiting thread

}

template <typename T>
void ConcurrentQueue<T>::push_back(T&& item)
{
	std::unique_lock<std::mutex> mlock(m_mutex);
	m_queue.push_back(std::move(item));
	mlock.unlock();     // unlock before notificiation to minimize mutex con
	m_cond.notify_one(); // notify one waiting thread

}

template <typename T>
void ConcurrentQueue<T>::remove(T& item){
	std::unique_lock<std::mutex> mlock(m_mutex);
	auto iter = std::find(std::begin(m_queue), std::end(m_queue), item);
	m_queue.erase(iter);
}

template <typename T>
int ConcurrentQueue<T>::size()
{
	std::unique_lock<std::mutex> mlock(m_mutex);
	int size = m_queue.size();
	mlock.unlock();
	return size;
}

template <typename T>
bool ConcurrentQueue<T>::empty()
{
	std::unique_lock<std::mutex> mlock(m_mutex);
	bool empty = m_queue.empty();
	mlock.unlock();
	return empty;
}