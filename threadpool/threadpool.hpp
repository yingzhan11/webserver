
#pragma once
#include <iostream>
#include <pthread.h>
#include <list>
#include <exception>
#include "locker.hpp"
template <typename T>
class threadpool
{
private:
	int w_threadnum; // number of threads
	int w_trimode;	 // trigger model
	int w_maxrequest;
	pthread_t *w_threads;	// work threads array
	std::list<T *> w_queue; // work queue
	locker	w_queuelocker;	//queue locker
	sem		w_queuestas;//queue status define in locker.hpp
	static	void *worker(void *arg);
	void	run();
public:
	threadpool(int tri_mode, int threadnum = 8, int maxrequest = 10000);
	~threadpool();

};
template <typename T>
threadpool<T>::threadpool(int tri_mode, int threadnum, int maxrequest) : w_trimode(tri_mode), w_threadnum(threadnum), w_maxrequest(maxrequest), w_threads(nullptr)
{

	if (w_threadnum <= 0 || w_maxrequest <= 0)
		throw std::runtime_error("invalid threadnum or maxrequest");
	w_threads = new pthread_t[w_threadnum];
	if (!w_threads)
		throw std::runtime_error("Failed to allocate space for threads");
	for (int i = 0; i < w_threadnum; ++i)
	{
		if (pthread_create(w_threads + i, NULL, worker, this) != 0)
		{
			delete[] w_threads;
			throw std::runtime_error("Threads create error");
		}
		if (pthread_detach(w_threads[i]))
		{
			delete[] w_threads;
			throw std::runtime_error("Threads detach error");
		}
	}
}
template <typename T>
void *threadpool<T>::worker(void *arg)
{
	threadpool *pool = (threadpool *)arg;
	pool->run();
	return pool;
}
template <typename T>
void	threadpool<T>::run()
{

}
template <typename T>
threadpool<T>::~threadpool()
{
	if (w_threads)
		delete[] w_threads;
}
