#include "webserver.hpp"

webserver::webserver() : w_pool(nullptr), config(Config::getinstance())
{
	users = new http_request[MAX_FD];
}

//TODO cgi .inputFile and cgi.html need to be delete when connection end, should be here?
webserver::~webserver()
{
	std::cout << "Delete\n";
	if (w_pool)
		delete w_pool;
	delete[] users;
}

void webserver::thread_pool()
{
	w_pool = new threadpool<http_request>(1);
}
// bool isAlreadyBound(const std::string &ip, int port,
// 					const std::unordered_map<int, std::unordered_set<std::string>> &boundIPs)
// {
// 	auto it = boundIPs.find(port);
// 	if (it != boundIPs.end())
// 		return it->second.count(ip) > 0;
// 	return false;
// }

void webserver::eventlisten()
{
	std::unordered_map<int, std::unordered_set<std::string>> boundIPs;
	std::vector<std::string> globalIpset = checkifaddr();

	for (const ServerConfig &server : config.servers)
	{
		for (int port : server.ports)
		{
			std::string ipport = server.ip + ":" + std::to_string(port);
			std::cout << "Iport:" << ipport << std::endl;
			if (!server.ip.empty())
			{
				if (boundIPs[port].count(server.ip))
				{
					std::cout << server.ip << ":" << port << " Aready bound.Move to next!" << std::endl;
					CurrentIpMemberServer[listenfd[ipport]].insert(server);
					std::cout << "---------------------------------fd:" << listenfd[ipport] << std::endl
							  << std::endl;
					continue;
				}
				int socket = createListenSocket(server.ip.c_str(), port, &boundIPs);
				listenfd[ipport] = socket;
				std::cout << "listenfd:" << socket << "  server:" << server.server_name << std::endl
				 << std::endl;
				CurrentIpMemberServer[socket].insert(server);
				std::cout << "---------------------------------fd:" << socket << std::endl
						  << std::endl;
			}
			else
			{
				std::vector<std::string> ipset = globalIpset;
				ipset.erase(std::remove_if(ipset.begin(), ipset.end(), [&](const std::string &ip)
										   { return boundIPs[port].count(ip) > 0; }),
							ipset.end());
				if (ipset.size() == 0)
				{
					// std::cout << "Current All:port already bound.Move to next!" << std::endl;
					for (std::string ip : globalIpset)
					{
						std::string gloipport = ip + ":" + std::to_string(port);
						// std::cout << "gloipport:" << gloipport << "  server:" << server.server_name << std::endl
						// 		  << std::endl;
						CurrentIpMemberServer[listenfd[gloipport]].insert(server);
						// std::cout << "---------------------------------glofd1:" << listenfd[gloipport] << std::endl
						// 		  << std::endl;
					}
					continue;
				}
				for (const std::string &ip : ipset)
				{
					ipport = ip + ":" + std::to_string(port);
					std::cout << "Iport:" << ipport << std::endl;
					int socket = createListenSocket(ip.c_str(), port, &boundIPs);
					listenfd[ipport] = socket;
					// std::cout << "listenfd:" << socket << "  server:" << server.server_name << std::endl
					// 		  << std::endl;
					// CurrentIpMemberServer[socket].insert(server);
				}
				for (std::string ip : globalIpset)
				{
					std::string gloipport = ip + ":" + std::to_string(port);
					// std::cout << "gloipport:" << gloipport << "  server:" << server.server_name << std::endl
					// 		  << std::endl;
					CurrentIpMemberServer[listenfd[gloipport]].insert(server);
					// std::cout << "---------------------------------glofd2:" << listenfd[gloipport] << std::endl
					// 		  << std::endl;
				}
			}
		}
	}
	for (auto it = CurrentIpMemberServer.begin(); it != CurrentIpMemberServer.end(); ++it)
	{
		int fd = it->first;
		const std::unordered_set<ServerConfig> &serverSet = it->second;

		std::cout << "Socket FD: " << fd << " -> Servers:\n";
		for (const ServerConfig &server : serverSet)
			std::cout << server.server_name << "\n";
	}
	epollrigster();
}

void webserver::epollrigster()
{
	w_epollfd = epoll_create(5);
	assert(w_epollfd != -1);
	for (auto it = listenfd.begin(); it != listenfd.end(); it++)
	{
		// std::cout << "fd:" << it->second << std::endl;
		util.addfd(w_epollfd, it->second, false, w_trimode);
	}
	http_request::w_epollfd = w_epollfd;
	int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, w_pipefd);
	assert(ret != -1);
	util.setnonblocking(w_pipefd[1]);
	util.addfd(w_epollfd, w_pipefd[0], false, 0);
	util.addsig(SIGPIPE, SIG_IGN);
	util.addsig(SIGINT, util.sig_handler, false);
	utils::u_pipefd = w_pipefd;
	utils::u_epollfd = w_epollfd;
	// int number = epoll_wait(w_epollfd, events, MAX_EVENTNUMBER, -1);
	// std::cout << "Number:" << number << std::endl;
}
bool webserver::isListenfd(int sockfd)
{
	for (const auto &fd : listenfd)
	{
		if (sockfd == fd.second)
			return true;
	}
	return false;
}
bool webserver::dealclientdata(int sockfd)
{
	struct sockaddr_in client_add;
	socklen_t client_addrlength = sizeof(client_add);
	if (0 == w_trimode) // LT
	{
		int connfd = accept(sockfd, (struct sockaddr *)&client_add, &client_addrlength);
		{	std::cout << "[dealclientdata] accep1t11 new fd=" << connfd << std::endl;
			if (connfd < 0)
			{
				perror("accept error");
				return false;
			}
			if (http_request::w_user_count >= MAX_FD)
			{
				util.show_error(connfd, "Internal server busy");
				return false;
			}
		}
		users[connfd].init(connfd, client_add, 0);
		auto it = CurrentIpMemberServer.find(sockfd);
		if (it != CurrentIpMemberServer.end())
		{
			users[connfd].serverconfig = &it->second;
		}
	}
	else // ET
	{
		while (1)
		{
			int connfd = accept(sockfd, (struct sockaddr *)&client_add, &client_addrlength);
			std::cout << "[dealclientdata] accept new fd=" << connfd << std::endl;

			if (connfd < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{

					break;
				}
				perror("accept error");
				break;
			}
			if (http_request::w_user_count >= MAX_FD)
			{
				util.show_error(connfd, "Internal server busy");
				break;
			}
			users[connfd].init(connfd, client_add, 1);

			auto it = CurrentIpMemberServer.find(sockfd);
			if (it != CurrentIpMemberServer.end())
			{
				users[connfd].serverconfig = &it->second;
			}
		}
		return false;
	}
	return true;
}
bool webserver::dealwithsignal(bool &stop_server)
{
	int ret = 0;
	int sig;
	char signals[1024];
	std::cout << "Ddelete\n";
	ret = recv(w_pipefd[0], signals, sizeof(signals), 0);
	if (-1 == ret || 0 == ret)
		return false;
	else
	{
		for (int i = 0; i < ret; ++i)
		{
			switch (signals[i])
			{
			// case SIGALRM:
			// {
			//     timeout = true;
			//     break;
			// }
			case SIGINT:
			{
				std::cout << "Ddelete\n";
				stop_server = true;
				break;
			}
			}
		}
	}
	return true;
}
void webserver::dealwithread(int sockfd)
{
	std::cout << "read\n" << std::endl;
	w_pool->append(users + sockfd, 0);

	while (true)
	{
		if (1 == users[sockfd].improv)
		{
			users[sockfd].improv = 0;
			break;
		}
	}
}
void webserver::dealwithwrite(int sockfd)
{
	std::cout << "write\n"
			  << std::endl;
	w_pool->append(users + sockfd, 1);
	while (true)
	{
		if (1 == users[sockfd].improv)
		{
			// may timer
			break;
		}
	}
}
void webserver::eventloop()
{
	bool stop = false;
	while (!stop)
	{
		int number = epoll_wait(w_epollfd, events, MAX_EVENTNUMBER, -1);
		if (number < 0 && errno != EINTR)
		{
			perror("Epoll failed");
			break;
		}
		for (int i = 0; i < number; i++)
		{
			int sockfd = events[i].data.fd;
			std::cout << "Event on fd: " << sockfd << ", event mask: " << events[i].events << std::endl;

			if (isListenfd(sockfd))
			{
				std::cout << "data\n"
						  << std::endl;
				if (!dealclientdata(sockfd))
					continue;
			}
			else if ((sockfd == w_pipefd[0]) && (events[i].events & EPOLLIN))
			{
				std::cout << "signallll\n";
				if (!dealwithsignal(stop))
					perror("dealclientdata failure");
			}
			else if (events[i].events & EPOLLIN)
			{
				dealwithread(sockfd);
			}
			else if (events[i].events & EPOLLOUT)
			{
				std::cout << "write\n"
						  << std::endl;
				dealwithwrite(sockfd);
			}
		}
	}
}

std::vector<std::string> checkifaddr()
{
	struct ifaddrs *ifaddr, *ifa;
	std::vector<std::string> ipset;
	if (getifaddrs(&ifaddr) == -1)
		throw std::runtime_error("Failed to getifaddrs!");
	for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == nullptr)
			continue;
		if (ifa->ifa_addr->sa_family == AF_INET)
		{
			char ip[INET_ADDRSTRLEN];
			struct sockaddr_in *sa = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
			inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);

			ipset.push_back(ip);
		}
	}
	freeifaddrs(ifaddr);
	return ipset;
}

int createListenSocket(const char *ip, int port, std::unordered_map<int, std::unordered_set<std::string>> *boundIPs)
{

	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);
	int flag = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = inet_addr(ip);
	int ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
	assert(ret >= 0);
	ret = listen(listenfd, 5);
	assert(ret >= 0);
	if (ip)
		(*boundIPs)[port].insert(std::string(ip));

	return listenfd;
}
