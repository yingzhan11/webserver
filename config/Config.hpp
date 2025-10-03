#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <sys/stat.h>

#define MAX_SIZE_LIMIT		10737418240.0//TODO

// struct RouteConfig
// {
// 	std::string path = "/";
// 	std::vector<std::string> allowed_methods = {"GET", "POST"};
// 	std::string redirect = "";
// 	// std::string root_directory = "/var/www/html";
// 	std::string root_directory = "www/site2";
// 	bool directory_listing = false;
// 	std::string index_file = "index.html";
// 	std::vector<std::string> cgi_extensions = {".php", ".py"};
// 	std::vector<std::string> cgi_interpreters = {"/usr/bin/php", "/usr/bin/python3"};
// };

// struct ServerConfig
// {
// 	std::string ip = "127.0.0.1";
// 	std::string server_name = "localhost";
// 	std::vector<int> ports = {8080};
// 	//port should greater than 1024 in unix!
// 	std::string root_directory = "www/site2";
// 	std::string default_file = "index.html";
// 	std::unordered_map<int, std::string> error_pages = {{404, "404.html"}, {500, "500.html"}};
// 	size_t client_body_size = 1024 * 1024; // 1MB
// 	bool directory_listing = false;
// 	std::vector<RouteConfig> routes;
// 	bool operator==(const ServerConfig &other) const {
// 	return  server_name == other.server_name;
// 	}
// };

using RawSetting = std::map<std::string, std::string>;

struct RouteConfig
{
	std::string path;
	std::vector<std::string> allowed_methods;
	std::string redirect;
	std::string root_directory;
	bool directory_listing;
	std::string index_file;
	std::vector<std::string> cgi_extensions;
	std::vector<std::string> cgi_interpreters;
	std::string upload_to;
};

struct ServerConfig
{
	std::string ip;
	std::string server_name;
	std::vector<int> ports;
	//port should greater than 1024 in unix!
	std::string root_directory;
	std::string default_file;
	std::unordered_map<int, std::string> error_pages;
	size_t client_body_size; // 1MB
	bool directory_listing;  //??
	std::vector<RouteConfig> routes;
	bool operator==(const ServerConfig &other) const {
	return  server_name == other.server_name;
	}
};

namespace std {
	template <>
	struct hash<ServerConfig> {
		std::size_t operator()(const ServerConfig& config) const {
			return std::hash<std::string>{}(config.server_name);
		}
	};
}

class Config
{
private:
	Config();
	Config(int ac, char **av);

	void	_checkServerSettingKeyword(RawSetting &serverSetting);
	void	_addLocationToRoutes(ServerConfig &server, std::map<std::string, RawSetting>locations);
	std::string	_checkServerName(std::string const &name);
	std::vector<int>	_checkPorts(std::string const &portStr);
	bool	_checkRoot(std::string const &path);
	std::string	_checkPage(std::string const &root, std::string const &page);
	size_t	_checkClientBodySize(std::string const &size);
	std::unordered_map<int, std::string> _parseErrorPage(std::string const &root, std::string const &pages);
		
	void	_addServer(ServerConfig&& server);

public:
	static Config &getinstance();
	static Config &getinstance(int ac, char **av);
	Config(Config &other) = delete;
	Config &operator=(const Config &) = delete;
	~Config();

	std::vector<ServerConfig> servers;

	void	addConfigToServers(RawSetting serverSetting, std::map<std::string, RawSetting>locations);

	//void	addRoute(RouteConfig&& route,int i);
	};
