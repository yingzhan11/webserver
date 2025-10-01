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

//p
void Config::addConfigToServers(RawSetting serverSetting, std::map<std::string, RawSetting>locations)
{
	//todo 需要检查

	ServerConfig server;
	//RouteConfig route;

	
	//check server setting TODO
	//check location

	//add location to server
	addLocationToRoutes(server, locations);

	//add setting to server
	server.server_name = serverSetting["server_name"]; // check unique name
	server.ports.push_back(std::stoi((serverSetting["listen"])));
	server.ip = serverSetting["host"];
	server.root_directory = serverSetting["root"];
	server.default_file = serverSetting["index"];
	std::string sizeStr = serverSetting["client_body_size"];
	server.client_body_size = static_cast<size_t>(std::stoul(sizeStr));
	std::string errorPage = serverSetting["error_page"];
	int errorCode = std::stoi(errorPage.substr(0, errorPage.find('.')));
	server.error_pages[errorCode] = errorPage;

	this->addServer(std::move(server));
}

void Config::addLocationToRoutes(ServerConfig &server, std::map<std::string, RawSetting>locations)
{
	//RouteConfig route;
	
	//route.path = "here is location test";
	//add value in lactions into route
	std::map<std::string, RawSetting>::iterator it = locations.begin();

	for (; it != locations.end(); ++it)
	{
		RouteConfig route;

		route.path = it->first;

		if (it->second.find("allow_methods") != it->second.end())
		{
			std::string methods = it->second.at("allow_methods");
			std::istringstream ss(methods);
			std::string method;
			while (ss >> method)
				route.allowed_methods.push_back(method);
			//todo check if it is get post delete
		}
		if (it->second.find("return") != it->second.end())
			route.redirect = it->second.at("return");
		if (it->second.find("root") != it->second.end())
			route.root_directory = it->second.at("root");
			//todo check is there a /
		if (it->second.find("autoindex") != it->second.end())
			route.directory_listing = it->second.at("autoindex") == "on";
		if (it->second.find("try_file") != it->second.end())
			route.index_file = it->second.at("try_file");
		if (it->second.find("cgi_ext") != it->second.end())
			route.cgi_extensions.push_back(it->second.at("cgi_ext"));
		if (it->second.find("cgi_path") != it->second.end())
			route.cgi_interpreters.push_back(it->second.at("cgi_path"));
			//todo check is ther a /
		//there is no upload to in routeconfig? TODO
		server.routes.push_back(std::move(route));
	}
}

void	Config::addServer(ServerConfig&& server)
{
	this->servers.push_back(std::move(server));
}

//void	Config::addRoute(RouteConfig&& route,int i)
//{
//	this->servers[i].routes.push_back(std::move(route));
//}

