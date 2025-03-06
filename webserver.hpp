#pragma once
#include "./config/Config.hpp"
#include "./threadpool/threadpool.hpp"
#include "./http/http_request.hpp"
const int MAX_FD = 65536;
class webserver
{
private:
	threadpool<http_request> *w_pool;
	int m_pipefd[2];
    int m_epollfd;
    http_request *users;
	Config&	config;
public:
	webserver();
	~webserver();
	void	thread_pool();
	void	eventlisten();
public:
	int	w_threadnum;   // number of threads
	int w_trimode;     // trigger model
	int w_maxrequest;
};
