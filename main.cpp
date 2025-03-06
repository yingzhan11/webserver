#include "webserver.hpp"
int main(int ac, char **av)
{
	try
	{
		Config &Config = Config::getinstance();

		webserver server;
		server.thread_pool();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return 0;
}
