#include "webserver.hpp"
#include "./parser/Parser.hpp"

void printConfig(Config &Config);

int main(int ac, char **av)
{
	try
	{
		Config &Config = Config::getinstance();
		Parser parser(ac, av);

		printConfig(Config);

		webserver server;
		server.thread_pool();
		server.eventlisten();
		server.eventloop();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return 0;
}

void printConfig(Config &Config)
{
	std::cout << "Config initialized with " << Config.servers.size() << " servers." << std::endl;
	for (size_t i = 0; i < Config.servers.size(); ++i)
	{
		std::cout << "Server " << i + 1 << ": " << Config.servers[i].server_name << " on ports: ";
		std::cout<< "ports nbr: " << Config.servers[i].ports.size() << std::endl;
		for (size_t j = 0; j < Config.servers[i].ports.size(); j++) {
			std::cout << Config.servers[i].ports[j];
			if (j < Config.servers[i].ports.size() - 1)
				std::cout << ", ";
		}
		std::cout << " with root: " << Config.servers[i].root_directory;
		std::cout << " and index: " << Config.servers[i].default_file;
		std::cout << " and client body size: " << Config.servers[i].client_body_size;
		std::cout << " and error pages: ";
		for (std::unordered_map<int, std::string>::iterator it = Config.servers[i].error_pages.begin(); it != Config.servers[i].error_pages.end(); ++it) {
			std::cout << it->first << "->" << it->second << " ";
		}
		std::cout << std::endl;

		std::vector<RouteConfig> routes = Config.servers[i].routes;
		for (size_t j = 0; j < routes.size(); ++j)
		{
			const RouteConfig& route = routes[j];
			std::cout << "Route " << j+1 << ":\n";

			if (!route.path.empty())
				std::cout << "  Path: " << route.path << "\n";

			if (!route.root_directory.empty())
				std::cout << "  Root Directory: " << route.root_directory << "\n";

			if (!route.index_file.empty())
				std::cout << "  Index File: " << route.index_file << "\n";

			if (!route.redirect.empty())
				std::cout << "  Redirect: " << route.redirect << "\n";

			if (!route.allowed_methods.empty()) {
				std::cout << "  Allowed Methods: ";
				for (size_t k = 0; k < route.allowed_methods.size(); ++k) {
					std::cout << route.allowed_methods[k];
					if (k + 1 < route.allowed_methods.size()) std::cout << ", ";
				}
				std::cout << "\n";
			}

			if (!route.cgi_extensions.empty()) {
				std::cout << "  CGI Extensions: ";
				for (size_t k = 0; k < route.cgi_extensions.size(); ++k) {
					std::cout << route.cgi_extensions[k];
					if (k + 1 < route.cgi_extensions.size()) std::cout << ", ";
				}
				std::cout << "\n";
			}

			if (!route.cgi_interpreters.empty()) {
				std::cout << "  CGI Interpreters: ";
				for (size_t k = 0; k < route.cgi_interpreters.size(); ++k) {
					std::cout << route.cgi_interpreters[k];
					if (k + 1 < route.cgi_interpreters.size()) std::cout << ", ";
				}
				std::cout << "\n";
			}

			std::cout << "  Directory Listing: " 
					<< (route.directory_listing ? "on" : "off") << "\n";

			std::cout << "------------------------\n";
		}
		std::cout << std::endl;
	}
}