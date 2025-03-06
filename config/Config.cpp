#include "Config.hpp"
Config::Config()
{
	ServerConfig server;
	RouteConfig  route_config;
	this->addServer(server);
	this->addRoute(route_config);
}
Config::~Config(){}
Config& Config::getinstance()
{
 	static Config instance;
	return instance;
}
void	Config::addServer(const ServerConfig& server)
{
	this->servers.push_back(std::move(server));
}
void	Config::addRoute(const RouteConfig& route)
{
	this->routes.push_back(std::move(route));
}
