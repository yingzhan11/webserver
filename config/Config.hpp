#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
struct RouteConfig
{
	std::string path = "/";
	std::vector<std::string> allowed_methods = {"GET", "POST"};
	std::string redirect = "";
	std::string root_directory = "/var/www/html";
	bool directory_listing = false;
	std::string index_file = "index.html";
	std::vector<std::string> cgi_extensions = {".php", ".py"};
	std::vector<std::string> cgi_interpreters = {"/usr/bin/php", "/usr/bin/python3"};
};

struct ServerConfig
{
	std::string ip = "127.0.0.1";
	std::string server_name = "localhost";
	std::vector<int> ports = {8080,9090};
	//port should greater than 1024 in unix!
	std::string root_directory = "/var/www/html";
	std::string default_file = "index.html";
	std::unordered_map<int, std::string> error_pages = {{404, "404.html"}, {500, "500.html"}};
	size_t client_body_size = 1024 * 1024; // 1MB
	bool directory_listing = false;
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

public:
	static Config &getinstance();
	Config(Config &other) = delete;
	Config &operator=(const Config &) = delete;
	~Config();
	std::vector<ServerConfig> servers;
	void	addServer(ServerConfig&& server);
	void	addRoute(RouteConfig&& route,int i);
};
