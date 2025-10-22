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
	Config &config;
	std::unordered_map<int, std::unordered_set<ServerConfig>> CurrentIpMemberServer;
	std::unordered_map<std::string, int> listenfd;
	epoll_event events[MAX_EVENTNUMBER];
	  int findExistingFd(const std::string& ip, int port);
public:
	webserver();
	~webserver();
	void thread_pool();
	void eventlisten();
	void eventloop();
	void epollrigster();
	bool isListenfd(int sockfd);
	bool dealclientdata(int sockfd);
	bool dealwithsignal(bool &stop_server);
	void dealwithread(int sockfd);
	void dealwithwrite(int sockfd);
public:
	int w_threadnum; // number of threads
	int w_trimode = 1;	 // trigger model
	int w_maxrequest;
	utils util;
	http_request *users;
};

int createListenSocket(const char *ip, int port, std::unordered_map<int, std::unordered_set<std::string>> *boundIPs);
std::vector<std::string> checkifaddr();
