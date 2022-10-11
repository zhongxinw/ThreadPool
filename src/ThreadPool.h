// ThreadPool.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once

#include <iostream>
#include <queue>
#include <mutex>
#include <functional>
#include <future>
#include <atomic>
#include <cassert>
// 一个线程安全的队列
template<typename T>
class SafeQueue
{
public:
	// 推入队列
	void enqueue(T &t) {
		std::unique_lock<std::mutex> lock(m_mutex);
		m_queue.emplace(t);
	}
	bool empty()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_queue.empty();
	}
	int32_t size() 
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_queue.size();
	}
	// 出队列
	bool dequeue(T &t) 
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_queue.empty())
		{
			return false;
		}
		t = m_queue.front();
		m_queue.pop();
		return true;
	}

private:
	std::queue<T> m_queue;
	std::mutex m_mutex;
};



class ThreadPool 
{
	// 线程工作对象
	class ThreadWorker {
	public:
		ThreadWorker(ThreadPool* pool, int32_t id) :m_pool(pool), m_id(id)
		{
			assert(m_pool != nullptr);
		}

		// 重载()符号，使其作为仿函数使用
		void operator()()
		{
			// 工作对象一直执行
			while (!m_pool->IsShutDown())
			{
				// 从队列中获取任务执行
				std::function<void()> func;
				// 获取任务标准
				bool bQueue = false;
				{
					// 线程池任务队列空时，阻塞等待唤醒
					if (m_pool->IsEmpty())
					{
						std::unique_lock<std::mutex> lock(m_pool->GetMutex());
						m_pool->GetCondition().wait(lock);
					}

					bQueue = m_pool->GetQueue().dequeue(func);
				}
				if (bQueue)
				{
					func();
				}
			}
		}

	private:
		ThreadPool* m_pool;		///< 线程池指针
		int32_t			m_id;		///< 工作对象在线程池中的
	};

public:
	// 发布任务的函数
	// 函数指针和可变模板参数
	// ->尾返回类型推导 auto推导无法用于推导函数f的返回类型，因此需要使用尾返回类型推导的方式指定返回值类型
	// 具体方式为decltype()指定
	// &&普通函数类型为右值引用，而在模板参数中则为万能引用，可指代左值或右值引用，在使用时需要std::forward(完美转发)配合使用
	template<typename F,typename ...Args>
	auto submit(F && f,Args && ...args) ->std::future<decltype(f(args...))>
	{
		// 把函数f的调用封装为一个函数指针
		std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f),std::forward<Args>(args)...);
		// 把封装函数封装为一个任务包
		// 个人理解任务包是用于记录当前函数调用状态，包含函数指针和执行参数
		auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
		std::function<void()> void_func = [task_ptr]()
		{
			(* task_ptr)();
		};

		m_queue.enqueue(void_func);
		// 通知工作对象取出任务
		m_condition.notify_one();

		return task_ptr->get_future();
	}

	ThreadPool(int32_t size) :m_vecThread(std::vector<std::thread>(size)),m_shutDown(false)
	{
		for (int32_t index = 0; index < m_vecThread.size(); ++index)
		{
			m_vecThread[index] = std::thread(ThreadWorker(this, index));
		}
	}

	void ShutDown() {
		m_shutDown = true;
		m_condition.notify_all();
		for (auto& thread : m_vecThread)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}
	}

	bool IsShutDown() {
		return m_shutDown;
	}

	bool IsEmpty() {
		return m_queue.empty();
	}
	std::condition_variable& GetCondition() {
		return m_condition;
	}
	std::mutex& GetMutex() {
		return m_mutex;
	}

	SafeQueue<std::function<void()>>& GetQueue() {
		return m_queue;
	}
private:
	// 把执行函数封装为一个void()函数
	SafeQueue<std::function<void()>>	m_queue;			///< 任务队列
	std::vector<std::thread>			m_vecThread;		///< 工作线程队列
	std::atomic_bool					m_shutDown;			///< 线程池退出标准
	std::condition_variable				m_condition;		///< 线程唤醒信号量
	std::mutex							m_mutex;			///< 线程唤醒/休眠锁
};

