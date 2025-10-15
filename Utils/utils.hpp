/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzheng <yzheng@student.hive.fi>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/14 13:20:24 by yzheng            #+#    #+#             */
/*   Updated: 2025/04/15 18:28:23 by yzheng           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>
#include <cstring>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <ctime>
#include <sstream>
#include <map>

class utils
{
private:
public:
	utils(){}
	~utils(){}
	void	addfd(int epollfd,int fd,bool one_shot, int trigmode);
	void setnonblocking(int fd);
	void addsig(int sig,void(handler)(int),bool restart = true);
	void show_error(int connfd, const char *info);
	static void sig_handler(int sig);
	static std::string itoa(int n);
	static time_t nowTime();
	static std::string getTimeStamp(const char *format);

public:
	static int *u_pipefd;
	static int u_epollfd;
	static std::string _defaultErrorPages(int status, std::string subText);
};

