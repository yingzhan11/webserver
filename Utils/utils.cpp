/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzheng <yzheng@student.hive.fi>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/14 13:37:23 by yzheng            #+#    #+#             */
/*   Updated: 2025/05/20 17:18:54 by yzheng           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include "./utils.hpp"
void utils::setnonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd,F_SETFL,new_option);
}
void utils::addfd(int epollfd,int fd,bool one_shot, int trigmode)
{
	epoll_event event;
	event.data.fd = fd;
	if(1 == trigmode)
		event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	else
		event.events = EPOLLIN | EPOLLRDHUP;
	 if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}
int* utils::u_pipefd = 0;
int  utils::u_epollfd = 0;
void utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}
void utils::addsig(int sig,void(handler)(int),bool restart)
{
	struct sigaction sa;
	memset(&sa,'\0',sizeof(sa));
	sa.sa_handler = handler;
	if(restart)
		sa.sa_flags |= SA_RESTART;
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig,&sa,NULL) != -1);
}
void utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);

    errno = save_errno;
}
std::string utils::itoa(int n)
{
	std::stringstream ss;
	ss << n;
	return ss.str();
}

time_t utils::nowTime()
{
    return (std::time(NULL));
}

std::string utils::getTimeStamp(const char *format)
{
	time_t		now = nowTime();
	struct tm	tstruct;
	char		buf[80];

	tstruct = *localtime(&now); 
	strftime(buf, sizeof(buf), format, &tstruct);
	return (buf);
}

std::map<int, std::string> initErrorMap()
{
    std::map<int, std::string> m;

	m[400] = "Bad Request";
	m[401] = "Unauthorized";
	m[403] = "Forbidden";
	m[404] = "Not Found";
	m[405] = "Method Not Allowed";
	m[406] = "Not Acceptable";
	m[408] = "Request Timeout";
	m[409] = "Conflict";
	m[411] = "Length Required";
	m[413] = "Payload Too Large";
	m[414] = "URI Too Long";
	m[415] = "Unsupported Media Type";
	m[500] = "Internal Server Error";
	m[501] = "Not Implemented";
	m[504] = "Gateway Timeout";
	m[505] = "HTTP Version Not Supported";
	m[508] = "Loop Detected";
	return m;
}

std::map<int, std::string> _errorHTML = initErrorMap();

std::string utils::_defaultErrorPages(int status, std::string subText)
{
	std::string statusDescrip = _errorHTML[status];

	std::string BodyPage = 
	"<!DOCTYPE html>\n"
	"<html lang=\"en\">\n"
	"<head>\n"
	"    <meta charset=\"UTF-8\">\n"
	"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, maximum-scale=1\">\n"
	"    <title>Error Page</title>\n"
	//"    <link rel=\"canonical\" href=\"https://www.site1/index.html\" />\n"
	"    <link rel=\"shortcut icon\" href=\"/favicon.ico\">\n"
	"    <link rel=\"stylesheet\" href=\"/styles.css\">\n"
	"    <link href=\"https://fonts.googleapis.com/css2?family=DM+Mono:ital,wght@0,300;0,400;0,500;1,300;1,400;1,500&family=Major+Mono+Display&display=swap\" rel=\"stylesheet\">\n"
	"</head>\n"
	"<body>\n"
	"    <div class=\"error-container\">\n"
	"        <h3 class=\"errortitle\">Error " + utils::itoa(status) + "</h3>\n"
	"        <h3 class=\"errortitle\">" + statusDescrip + ": </h3>\n"
	"        <p class=\"error-paragraph\">" + subText + " </p>\n"
	"    </div>\n"
	"</body>\n"
	"</html>\n";

	return (BodyPage);
}

std::string utils::_readFileContent(const std::string &path, const std::string &root = "") {
    std::string full_path = path;
    if (!root.empty() && path[0] != '/')
        full_path = root + "/" + path;

    std::ifstream file(full_path.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + full_path);
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}