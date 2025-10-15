#pragma once
#include <unordered_map>
#include <unordered_set>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../config/Config.hpp"
#include "../CGI/cgi.hpp"
#include "../Utils/utils.hpp"

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
	const std::unordered_set<ServerConfig>* serverconfig;
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
	char w_real_file[FILENAME_LEN]; //fullPath
	char *w_url;
	char *w_version;
	char *w_host;
	long w_content_length; //requestLength, & payload length
	bool w_linger;
	char *w_file_address;
	struct stat w_file_stat;
	struct iovec w_iv[2];
	int w_iv_count;
	int is_cgi;
	char *w_string; //payload
	int bytes_to_send;
	int bytes_have_send;

public:
	http_request();
	~http_request();
	void init(int clientfd, const sockaddr_in &addr, int tri);
	void initt();
	bool read_once();
	void process();
	bool write();
	void close_conn(bool real_close = true);
private:
	HTTP_CODE process_read();
	char *get_line() { return w_read_buf + w_start_line; };
	HTTP_CODE parse_request_line(char *text);
	LINE_STATUS parse_line();
	HTTP_CODE parse_headers(char *text);
	HTTP_CODE parse_content(char *text);
	HTTP_CODE do_request();
	bool process_write(HTTP_CODE ret);
	bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();
	void unmap();

	void _exportEnv(CGI &cgi);
};
