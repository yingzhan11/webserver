#pragma once
class http_request
{
private:
	/* data */
public:
	static int w_epollfd;
	http_request();
	~http_request();
};

