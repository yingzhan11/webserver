#include "webserver.hpp"

webserver::webserver() : w_pool(nullptr), config(Config::getinstance())
{
	w_trimode = 1;
	w_threadnum = 8;
	w_maxrequest = 10000;
}
webserver::~webserver()
{
	if (w_pool)
		delete w_pool;
}

void webserver::thread_pool()
{
	w_pool = new threadpool<http_request>(w_trimode, w_threadnum, w_maxrequest);
}
bool isAlreadyBound(const std::string &ip, int port,
					const std::unordered_map<int, std::unordered_set<std::string>> &boundIPs)
{
	auto it = boundIPs.find(port);
	if (it != boundIPs.end())
		return it->second.count(ip) > 0;
	return false;
}
void webserver::eventlisten()
{
	int linger = 0;
	std::unordered_map<int, std::unordered_set<std::string>> boundIPs;
		std::vector<std::string> globalIpset = checkifaddr();
	for (const ServerConfig &server : this->config.servers)
	{
		for (int port : server.ports)
		{
			if (!server.ip.empty())
			{
				if (isAlreadyBound(server.ip, port, boundIPs))
				{
					std::cout<< server.ip <<":" <<port << " Aready bound.Move to next!" << std::endl;
					continue;
				}
				int socket = createListenSocket(server.ip.c_str(), port, &boundIPs);
				this->listenfd.push_back(std::move(socket));
			}
			else
			{
				std::vector<std::string> ipset = globalIpset;
				ipset.erase(std::remove_if(ipset.begin(), ipset.end(), [&](const std::string &ip)
										   { return isAlreadyBound(ip, port, boundIPs); }),
							ipset.end());
				if (ipset.size() == 0)
				{
					std::cout << "Current All:port already bound.Move to next!" << std::endl;
					continue;
				}
				for (const std::string &ip : ipset)
				{
					int socket = createListenSocket(ip.c_str(), port, &boundIPs);
					this->listenfd.push_back(std::move(socket));
				}
			}
		}
	}
}
void webserver::eventloop()
{
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
	std::cout<< ip <<":" <<port << " Successed!" << std::endl;
	return listenfd;
}
