#include "http_request.hpp"
http_request::http_request() {}
http_request::~http_request() {}
int http_request::w_epollfd = -1;
int http_request::w_user_count = 0;

void addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
	epoll_event event;
	event.data.fd = fd;

	if (1 == TRIGMode)
		event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	else
		event.events = EPOLLIN | EPOLLRDHUP;

	if (one_shot)
		event.events |= EPOLLONESHOT;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
}
int setnonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

void http_request::init(int clientfd, const sockaddr_in &addr, int tri,int sockfd)
{
	w_clientfd = clientfd;
	w_sockfd = sockfd;
	w_address = addr;
	w_TRIGMode = tri;
	addfd(w_epollfd, sockfd, true, w_TRIGMode);
	w_user_count++;

	init();
}
void http_request::init()
{

    bytes_to_send = 0;
    bytes_have_send = 0;
    w_check_state = CHECK_STATE_REQUESTLINE;
    w_linger = false;
    w_method = GET;
    w_url = 0;
    w_version = 0;
    w_content_length = 0;
    w_host = 0;
    w_start_line = 0;
    w_checked_idx = 0;
    w_read_idx = 0;
    w_write_idx = 0;
    cgi = 0;
    w_state = 0;

    improv = 0;

    memset(w_read_buf, '\0', READ_BUFFER_SIZE);
    memset(w_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(w_real_file, '\0', FILENAME_LEN);
}
