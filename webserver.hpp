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
#include <sys/epoll.h>
#include "./Utils/utils.hpp"

const int MAX_FD = 65536;
const int MAX_EVENTNUMBER = 10000;
class webserver
{
private:
	threadpool<http_request> *w_pool;
	int w_pipefd[2];
	int w_epollfd;
	http_request *users;
	Config &config;
	std::unordered_map<int, std::unordered_set<ServerConfig>> CurrentIpMemberServer;
	std::unordered_map<std::string, int> listenfd;
	epoll_event events[MAX_EVENTNUMBER];
public:
	webserver();
	~webserver();
	void thread_pool();
	void eventlisten();
	void eventloop();
	void epollrigster();
public:
	int w_threadnum; // number of threads
	int w_trimode;	 // trigger model
	int w_maxrequest;
	utils util;
};

int createListenSocket(const char *ip, int port, std::unordered_map<int, std::unordered_set<std::string>> *boundIPs);
std::vector<std::string> checkifaddr();
