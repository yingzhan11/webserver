#include "Config.hpp"

Config::Config()
{
// 	ServerConfig server;
// 	RouteConfig  route_config;
// 	this->addServer(std::move(server));
// 	ServerConfig server2;
// 	this->addServer(std::move(server2));
// 	ServerConfig server3;
// 	this->addServer(std::move(server3));
// 	// ServerConfig server4;
// 	// this->addServer(std::move(server4));
// 	// ServerConfig server5;
// 	// this->addServer(std::move(server5));
// 	this->servers[1].ip = "127.0.0.1";
// 	this->servers[1].ports[0] = 9090;

// 	this->servers[1].server_name = "server1dumb";
// 	this->servers[2].ip = "127.0.0.1";
// 	this->servers[2].server_name = "server2hh";

// 	// this->servers[3].ip = "";
// 	// this->servers[4].ip = "";

// 	// this->servers[1].ports[0] = 8080;
// 	// this->servers[1].ports[1] = 8019;
// 	std::cout << "++++path+++" << servers[1].root_directory << "++++++\n";
// 	this->addRoute(std::move(route_config),0);
}

Config::~Config(){}

//public
Config& Config::getinstance()
{
 	static Config instance;
	return instance;
}



//private
void Config::addConfigToServers(RawSetting serverSetting, std::map<std::string, RawSetting>locations)
{
	//TODO需要修改，保存多个value的情况
	//todo 需要检查parser的结果

	ServerConfig server;
	RawSetting settings = serverSetting;
	RawSetting::iterator it;
	for (it = settings.begin(); it != settings.end(); ++it) 
	{
		if (it->first == "server_name") {
			server.server_name = it->second;
		}
		else if (it->first == "listen") {
			int port = std::stoi(it->second);
			server.ports.push_back(port);
		}
		else if (it->first == "host") {
			server.ip = it->second;
		}
		else if (it->first == "root") {
			server.root_directory = it->second;
		}
		else if (it->first == "index") {
			server.default_file = it->second;
		}
		else if (it->first == "client_body_size") {
			std::string sizeStr = it->second;
			// Convert sizeStr to size_t, assuming it's in bytes for simplicity
			server.client_body_size = static_cast<size_t>(std::stoul(sizeStr));
		}
		else if (it->first == "error_page") {
			std::string errorPage = it->second;
			// Assuming error pages are named like "404.html"
			int errorCode = std::stoi(errorPage.substr(0, errorPage.find('.')));
			server.error_pages[errorCode] = errorPage;
		}
	}
	this->addServer(std::move(server));
}


void	Config::addServer(ServerConfig&& server)
{
	this->servers.push_back(std::move(server));
}

void	Config::addRoute(RouteConfig&& route,int i)
{
	this->servers[i].routes.push_back(std::move(route));
}