/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yzheng <yzheng@student.hive.fi>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/14 13:20:24 by yzheng            #+#    #+#             */
/*   Updated: 2025/04/14 14:15:09 by yzheng           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <sys/epoll.h>
#include <fcntl.h>
class utils
{
private:
public:
	utils(){}
	~utils(){}
	void	addfd(int epollfd,int fd,bool one_shot, int trigmode);
	void setnonblocking(int fd);
};

