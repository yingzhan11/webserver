#include "Config.hpp"
Config::Config()
{
	ServerConfig server;
	RouteConfig  route_config;
	this->addServer(std::move(server));
	ServerConfig server2;
	this->addServer(std::move(server2));
	ServerConfig server3;
	this->addServer(std::move(server3));
	// ServerConfig server4;
	// this->addServer(std::move(server4));
	// ServerConfig server5;
	// this->addServer(std::move(server5));
	this->servers[1].ip = "";
	this->servers[1].server_name = "server1aa";
	this->servers[2].ip = "";
	this->servers[2].server_name = "server2hh";
	// this->servers[3].ip = "";
	// this->servers[4].ip = "";

	// this->servers[1].ports[0] = 8080;
	// this->servers[1].ports[1] = 8019;
	this->addRoute(std::move(route_config),0);


}
Config::~Config(){}
Config& Config::getinstance()
{
 	static Config instance;
	return instance;
}
void	Config::addServer(ServerConfig&& server)
{
	this->servers.push_back(std::move(server));
}
void	Config::addRoute(RouteConfig&& route,int i)
{
	this->servers[i].routes.push_back(std::move(route));
}

