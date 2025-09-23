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

Config::Config(int ac, char **av)
{
	Parser parser(ac, av);
	_addConfigToServers(parser);
}

Config::~Config(){}

//public
Config& Config::getinstance()
{
 	static Config instance;
	return instance;
}

Config& Config::getinstance(int ac, char **av)
{
 	static Config instance(ac, av);
	return instance;
}

//private
void Config::_addConfigToServers(Parser &parser)
{
	//TODO需要修改，保存多个value的情况
	//todo 需要检查parser的结果
	std::vector<RawServer> server_settings = parser.serverVec;
	for (size_t i = 0; i < server_settings.size(); ++i)
	{
		ServerConfig server;
		RawServer settings = server_settings[i];
		RawServer::iterator it;
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
	std::cout << "Config initialized with " << this->servers.size() << " servers." << std::endl;
	for (size_t i = 0; i < this->servers.size(); ++i) {
		std::cout << "Server " << i + 1 << ": " << this->servers[i].server_name << " on ports: ";
		for (size_t j = 0; j < this->servers[i].ports.size(); ++j) {
			std::cout << this->servers[i].ports[j];
			if (j < this->servers[i].ports.size() - 1)
				std::cout << ", ";
		}
		std::cout << " with root: " << this->servers[i].root_directory;
		std::cout << " and index: " << this->servers[i].default_file;
		std::cout << " and client body size: " << this->servers[i].client_body_size;
		std::cout << " and error pages: ";
		for (std::unordered_map<int, std::string>::iterator it = this->servers[i].error_pages.begin(); it != this->servers[i].error_pages.end(); ++it) {
			std::cout << it->first << "->" << it->second << " ";
		}
		std::cout << std::endl;
	}
}


void	Config::addServer(ServerConfig&& server)
{
	this->servers.push_back(std::move(server));
}

void	Config::addRoute(RouteConfig&& route,int i)
{
	this->servers[i].routes.push_back(std::move(route));
}