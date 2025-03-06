#include "webserver.hpp"

webserver::webserver():w_pool(nullptr),config(Config::getinstance())
{
	w_trimode = 1;
	w_threadnum = 8;
	w_maxrequest = 10000;
}
webserver::~webserver()
{
	if(w_pool)
		delete w_pool;
}

void	webserver::thread_pool()
{
	w_pool = new threadpool<http_request>(w_trimode,w_threadnum,w_maxrequest);

}
