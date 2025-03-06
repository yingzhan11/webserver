#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
struct ServerConfig
{
	std::string host = "127.0.0.1";
	std::string server_name = "localhost";
	std::vector<int> port = {8080,9090};
	std::string root_directory = "/var/www/html";
	std::string default_file = "index.html";
	std::unordered_map<int, std::string> error_pages = {{404, "404.html"}, {500, "500.html"}};
	size_t client_body_size = 1024 * 1024; // 1MB
	bool directory_listing = false;
};

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
	std::vector<RouteConfig> routes;
	void	addServer(const ServerConfig& server);
	void	addRoute(const RouteConfig& route);
};
