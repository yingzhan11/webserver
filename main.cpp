#include "webserver.hpp"
#include "./parser/Parser.hpp"

int main(int ac, char **av)
{
	try
	{
		Config &Config = Config::getinstance();
		Parser parser(ac, av);

		std::cout << "Config initialized with " << Config.servers.size() << " servers." << std::endl;
		for (size_t i = 0; i < Config.servers.size(); ++i) {
			std::cout << "Server " << i + 1 << ": " << Config.servers[i].server_name << " on ports: ";
			for (size_t j = 0; j < Config.servers[i].ports.size(); ++j) {
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
		}

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
