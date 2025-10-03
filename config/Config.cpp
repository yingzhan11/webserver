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

Config& Config::getinstance()
{
 	static Config instance;
	return instance;
}

/**
 * main funcs
 */
//add setting and location into server, and add server into servers
void Config::addConfigToServers(RawSetting serverSetting, std::map<std::string, RawSetting>locations)
{
	ServerConfig server;

	//check server setting
	_checkServerSettingKeyword(serverSetting);
	//check location??? TODO

	//add location to server
	_addLocationToRoutes(server, locations);

	server.server_name = _checkServerName(serverSetting["server_name"]); // check unique name
	server.ports = _checkPorts(serverSetting["listen"]);
	server.ip = _checkHost(serverSetting["host"]);
	
	if (!_checkRoot(serverSetting["root"]))
        throw std::runtime_error("Didn't find the root directory '" + serverSetting["root"] + "'");
	server.root_directory = serverSetting["root"];

	server.default_file = _checkPage(server.root_directory, serverSetting["index"]);
	//serval error page, if no error page, use default error page? TODO
	server.error_pages = _parseErrorPage(server.root_directory, serverSetting["error_page"]);
	server.client_body_size = _checkClientBodySize(serverSetting["client_body_size"]);

	//this->_addServer(std::move(server));
	this->servers.push_back(std::move(server));
}

//add value in lactions into route
void Config::_addLocationToRoutes(ServerConfig &server, std::map<std::string, RawSetting>locations)
{
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
		if (it->second.find("upload_to") != it->second.end())
			route.upload_to = it->second.at("upload_to");
			//todo check is ther a /
		server.routes.push_back(std::move(route));
	}
}

/**
 * chckers
 */
void Config::_checkServerSettingKeyword(RawSetting &serverSetting)
{
	std::string const mandatory[] = {"host", "index", "listen", "root", "client_body_size"};
	std::string const forbidden[] = {"allow_methods", "autoindex", "cgi_ext", "cgi_path", "try_file", "upload_to"};

	//check if all mandatory key is here
	for (int i = 0; i < 5; i++)
	{
		if (serverSetting.find(mandatory[i]) == serverSetting.end())
			throw std::runtime_error("Error: Server lacks keyword '" + mandatory[i] + "'.");
	}
	//check if there is any forbidden key in server out of location? need this?? TODO
	for (int i = 0; i < 6; i++)
	{
		if (serverSetting.find(forbidden[i]) != serverSetting.end())
			throw std::runtime_error("Error: Server has forbidden keyword '" + forbidden[i] + "'.");
	}	
}

std::string Config::_checkServerName(std::string const &name)
{
	//check is there any servers name are same
	std::vector<ServerConfig>::const_iterator it;
	
	for (it = this->servers.begin(); it != this->servers.end(); it++)
	{
		if (!name.empty() && it->server_name == name)
			throw std::runtime_error("Error: Same server name");
	}
	return (name);
}

std::vector<int> Config::_checkPorts(std::string const &portStr)
{
	//check port number
	int portNbr = atoi(portStr.c_str());
	// port nbr should greater than 1024??? TODO
	if (portStr.find_first_not_of(" 0123456789") != std::string::npos || portNbr < 0 || portNbr > 65535)
		throw std::runtime_error("Invalid port number: " + portStr);
	if (portNbr <= 1024)
		throw std::runtime_error("Port number must be > 1024 (privileged ports not allowed): " + portStr);
	
		std::istringstream iss(portStr);
	int port;
	std::vector<int> ports;
	while (iss >> port) {
		ports.push_back(port);
	}
	return ports;
}

std::string Config::_checkHost(std::string const &host)
{
	std::string ip;

	if (host == "localhost")
		ip = "127.0.0.1";
	else if (host.find_first_not_of(".0123456789") != std::string::npos)
		throw std::runtime_error("Invalid port number: " + host);
	else
		ip = host;

	return ip;
}

bool Config::_checkRoot(std::string const &path)
{
	struct stat info;

	if (stat(path.c_str(), &info) != 0)
		return false;
	else if (info.st_mode & S_IFDIR)
		return true;
	return false;
}

std::string Config::_checkPage(std::string const &root, std::string const &page)
{
	if (!page.empty())
	{
		size_t find_dot = page.find_last_of(".");

		if (find_dot == std::string::npos || page.substr(find_dot) != ".html" || page.length() <= std::string(".html").length())
			throw std::runtime_error("index page must be end by '.html'");
		//std::string path
		std::string path = root + "/" + page;
		std::ifstream file(path.c_str());
		if (!file.good())
			throw std::runtime_error("Page not exist or cannot be opened '" + page + "'");
		file.close();
	}
	return page;
}

size_t	Config::_checkClientBodySize(std::string const &size)
{
	std::string nbrStr;
	std::string unit;
	double value = 0.0;

	if (isdigit(size[size.length() - 1]))
		nbrStr = size;
	else
	{
		nbrStr = size.substr(0, size.length() - 1);
		unit = size.substr(size.length() - 1, 1);
	}
	if ((!unit.empty() && unit.find_first_not_of("bBkKmMgG") != std::string::npos) || nbrStr.find_first_not_of("0123456789.") != std::string::npos)
		throw std::runtime_error("Invaid client body size '" + size + "'");

	value = atof(nbrStr.c_str());
	if (unit == "k" || unit == "K")
		value *= 1024;
	else if (unit == "m" || unit == "M")
		value *= 1024 * 1024;
	else if (unit == "g" || unit == "G")
		value *= 1024 * 1024 *1024;
	if (value > MAX_SIZE_LIMIT)
		throw std::runtime_error("Clinet body size '" + size +"' is invalid");

	return static_cast<size_t>(value);
}

std::unordered_map<int, std::string> Config::_parseErrorPage(std::string const &root, std::string const &pages)
{
	std::unordered_map<int, std::string> errorPages;
	std::istringstream iss(pages);
	std::string page;

	while (iss >> page) {
		int errorCode = std::stoi(page.substr(0, page.find('.')));
		if (errorCode < 100 || errorCode > 599)
    		throw std::runtime_error("Invalid HTTP error code in file: " + page);
		std::string validPage = _checkPage(root, page);
		errorPages[errorCode] = validPage;
	}
	return errorPages;
}

//void	Config::_addServer(ServerConfig&& server)
//{
//	this->servers.push_back(std::move(server));
//}

//void	Config::addRoute(RouteConfig&& route,int i)
//{
//	this->servers[i].routes.push_back(std::move(route));
//}

