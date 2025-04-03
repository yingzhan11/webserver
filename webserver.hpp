#pragma once
#include "./config/Config.hpp"
#include "./threadpool/threadpool.hpp"
#include "./http/http_request.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cassert>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

const int MAX_FD = 65536;
class webserver
{
private:
	threadpool<http_request> *w_pool;
	int m_pipefd[2];
    int m_epollfd;
    http_request *users;
	Config&	config;
	std::vector<int> listenfd;
public:
	webserver();
	~webserver();
	void	thread_pool();
	void	eventlisten();
	void	eventloop();
	void	listenMutiableIp();
public:
	int	w_threadnum;   // number of threads
	int w_trimode;     // trigger model
	int w_maxrequest;
};

int createListenSocket(const char* ip, int port, std::unordered_map<int, std::unordered_set<std::string>> *boundIPs);
std::vector<std::string> checkifaddr();
