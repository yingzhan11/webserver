#pragma once

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <cstring>
class http_request
{
public:
	static const int FILENAME_LEN = 200;
	static const int READ_BUFFER_SIZE = 2048;
	static const int WRITE_BUFFER_SIZE = 1024;
	static int w_user_count;
	static int w_epollfd;
	int w_state;
	int improv;
	enum METHOD
	{
		GET = 0,
		POST,
		HEAD,
		PUT,
		DELETE,
		TRACE,
		OPTIONS,
		CONNECT,
		PATH
	};
	enum CHECK_STATE
	{
		CHECK_STATE_REQUESTLINE = 0,
		CHECK_STATE_HEADER,
		CHECK_STATE_CONTENT
	};
	enum HTTP_CODE
	{
		NO_REQUEST,
		GET_REQUEST,
		BAD_REQUEST,
		NO_RESOURCE,
		FORBIDDEN_REQUEST,
		FILE_REQUEST,
		INTERNAL_ERROR,
		CLOSED_CONNECTION
	};
	enum LINE_STATUS
	{
		LINE_OK = 0,
		LINE_BAD,
		LINE_OPEN
	};

private:
	int w_sockfd;
	int w_clientfd;
	sockaddr_in w_address;
	int w_TRIGMode;
	char w_read_buf[READ_BUFFER_SIZE];
	long w_read_idx;
	long w_checked_idx;
	int w_start_line;
	char w_write_buf[WRITE_BUFFER_SIZE];
	int w_write_idx;
	CHECK_STATE w_check_state;
	METHOD w_method;
	char w_real_file[FILENAME_LEN];
	char *w_url;
	char *w_version;
	char *w_host;
	long w_content_length;
	bool w_linger;
	char *w_file_address;
	struct stat w_file_stat;
	struct iovec w_iv[2];
	int w_iv_count;
	int cgi;
	char *w_string;
	int bytes_to_send;
	int bytes_have_send;

public:

	http_request();
	~http_request();
	void init(int clientfd, const sockaddr_in &addr, int tri, int sockfd);
	void init();

};
