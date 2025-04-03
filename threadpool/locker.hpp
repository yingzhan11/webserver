#pragma once
#include <semaphore.h>
class locker
{
private:
	pthread_mutex_t w_mutex;

public:
	locker()
	{
		if (pthread_mutex_init(&w_mutex, NULL) != 0)
			throw std::exception();
	}
	~locker()
	{
		pthread_mutex_destroy(&w_mutex);
	}
	bool lock()
	{
		return pthread_mutex_lock(&w_mutex) == 0;
	}
	bool unlock()
	{
		return pthread_mutex_unlock(&w_mutex) == 0;
	}
	pthread_mutex_t *get()
	{
		return &w_mutex;
	}
};

class sem
{
private:
	sem_t w_sem;
public:
	sem()
	{
		if(sem_init(&w_sem,0,0) != 0)
			throw std::exception();
	}
	sem(int num)
	{
		if(sem_init(&w_sem,0,num) != 0)
			throw std::exception();
	}
	~sem()
	{
		sem_destroy(&w_sem);
	}
	bool wait()
	{
		return sem_wait(&w_sem) == 0;
	}
	bool post()
	{
		return sem_post(&w_sem) == 0;
	}
};



class cond
{
private:
	pthread_cond_t w_cond;
public:
	cond()
	{
		if(pthread_cond_init(&w_cond,NULL) != 0)
		{
			throw std::exception();
		}
	}
	~cond()
	{
		pthread_cond_destroy(&w_cond);
	}
	bool wait(pthread_mutex_t *w_mutex)
	{
		return pthread_cond_wait(&w_cond,w_mutex) == 0;
	}
	bool timewait(pthread_mutex_t *w_mutex,struct timespec t)
	{
		return pthread_cond_timedwait(&w_cond,w_mutex,&t);
	}
	bool signal()
	{
		return pthread_cond_signal(&w_cond) == 0;
	}
	bool broadcast()
	{
		return pthread_cond_broadcast(&w_cond) == 0;
	}

};
