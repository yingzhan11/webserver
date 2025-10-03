#include "http_request.hpp"
http_request::http_request() {}
http_request::~http_request() {}
int http_request::w_epollfd = -1;
int http_request::w_user_count = 0;
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

int setnonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}
void addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
	epoll_event event;
	event.data.fd = fd;
std::cout<<"addfd\n"<<epollfd<<std::endl;
	if (1 == TRIGMode)
		event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	else
		event.events = EPOLLIN | EPOLLRDHUP;
	if (one_shot)
		event.events |= EPOLLONESHOT;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
}

void modfd(int epollfd, int fd, int ev, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void http_request::init(int sockfd, const sockaddr_in &addr, int tri)
{

	w_sockfd = sockfd;
	w_address = addr;
	w_TRIGMode = tri;
	addfd(w_epollfd, sockfd, true, w_TRIGMode);
	w_user_count++;
	initt();
}
void http_request::initt()
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

bool http_request::read_once()
{
	if(w_read_idx >= READ_BUFFER_SIZE)
		return false;
	int bytes_read = 0;
	if(0 == w_TRIGMode) //LT
	{
		bytes_read = recv(w_sockfd,w_read_buf+w_read_idx,READ_BUFFER_SIZE - w_read_idx,0);
		w_read_idx += bytes_read;
		if(bytes_read<= 0)
			return false;

		return true;
	}
	else  //ET
	{
		while(true)
		{
			bytes_read = recv(w_sockfd,w_read_buf+w_read_idx,READ_BUFFER_SIZE - w_read_idx,0);
			if (bytes_read == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            }
            else if (bytes_read == 0)
            {
                return false;
            }
			w_read_idx += bytes_read;
		}
		for (int i = 0; i < w_read_idx; i++) {
    unsigned char c = w_read_buf[i];
    if (c == '\r') std::cout << "\\r";
    else if (c == '\n') std::cout << "\\n";
    else std::cout << c;
}
std::cout << std::endl;


		return true;
	}
}
void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}
void http_request::close_conn(bool real_close)
{
    if (real_close && (w_sockfd != -1))
    {
        printf("close %d\n", w_sockfd);
        removefd(w_epollfd, w_sockfd);
        w_sockfd = -1;
        w_user_count--;
    }
}
http_request::LINE_STATUS http_request::parse_line()
{
    char temp;
    for (; w_checked_idx < w_read_idx; ++w_checked_idx)
    {
        temp = w_read_buf[w_checked_idx];

        if (temp == '\r')
        {
           
            if ((w_checked_idx + 1) == w_read_idx)
                return LINE_OPEN;

       
            if (w_read_buf[w_checked_idx + 1] == '\n')
            {
                w_read_buf[w_checked_idx] = '\0';    
                w_checked_idx += 2;                  
                return LINE_OK;
            }

         
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
          
            if (w_checked_idx > 0 && w_read_buf[w_checked_idx - 1] == '\r')
            {
                w_read_buf[w_checked_idx - 1] = '\0'; 
                w_checked_idx++;
                return LINE_OK;
            }
         
            return LINE_BAD;
        }
    }

    // 数据还没读完
    return LINE_OPEN;
}

http_request::HTTP_CODE http_request::parse_request_line(char *text)
{
	static int i =0;
	i++;
	w_url = strpbrk(text, " \t");

    if (!w_url)
        return BAD_REQUEST;
    *w_url++ = '\0';
    char *method = text;
    if (strcasecmp(method, "GET") == 0)
        w_method = GET;
    else if (strcasecmp(method, "POST") == 0)
    {
        w_method = POST;
        cgi = 1;
    }
    else
        return BAD_REQUEST;
    w_url += strspn(w_url, " \t");
    w_version = strpbrk(w_url, " \t");
    if (!w_version)
        return BAD_REQUEST;
    *w_version++ = '\0';
    w_version += strspn(w_version, " \t");
    if (strcasecmp(w_version, "HTTP/1.1") != 0)
        return BAD_REQUEST;
    if (strncasecmp(w_url, "http://", 7) == 0)
    {
        w_url += 7;
        w_url = strchr(w_url, '/');
    }

    if (strncasecmp(w_url, "https://", 8) == 0)
    {
        w_url += 8;
        w_url = strchr(w_url, '/');
    }

    if (!w_url || w_url[0] != '/')
        return BAD_REQUEST;
    //当url为/时，显示判断界面
    if (strlen(w_url) == 1)
        strcat(w_url, "index.html");
    w_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}
http_request::HTTP_CODE http_request::parse_headers(char *text)
{
    if (text[0] == '\0')
    {
        if (w_content_length != 0)
        {
            w_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0)
    {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0)
        {
            w_linger = true;
        }
    }
    else if (strncasecmp(text, "Content-length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        w_content_length = atol(text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, " \t");
        w_host = text;
    }

    return NO_REQUEST;
}
http_request::HTTP_CODE http_request::parse_content(char *text)
{
    if (w_read_idx >= (w_content_length + w_checked_idx))
    {
        text[w_content_length] = '\0';
        w_string = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}
http_request::HTTP_CODE http_request::do_request()
{
    // 默认根目录
    std::string root = "/home/yan/webserver/site1";
    if (serverconfig && !serverconfig->empty()) {
        const ServerConfig &cfg = *(serverconfig->begin());
        if (!cfg.root_directory.empty()) {
            root = cfg.root_directory;
        }
    }

    // 拼接真实文件路径
    if (!w_url || w_url[0] == '\0' || strcmp(w_url, "/") == 0) {
        snprintf(w_real_file, FILENAME_LEN, "%s/index.html", root.c_str());
    } else {
        snprintf(w_real_file, FILENAME_LEN, "%s%s", root.c_str(), w_url);
    }

    printf("[do_request] real_file = %s\n", w_real_file);

    if (stat(w_real_file, &w_file_stat) < 0)
        return NO_RESOURCE;

    if (!(w_file_stat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;

    if (S_ISDIR(w_file_stat.st_mode))  // 如果还是目录，直接返回 BAD_REQUEST
        return BAD_REQUEST;

    int fd = open(w_real_file, O_RDONLY);
    if (fd < 0)
        return NO_RESOURCE;

    w_file_address = (char *)mmap(0, w_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    return FILE_REQUEST;
}



http_request::HTTP_CODE http_request::process_read()
{
      std::cout << "11111" <<std::endl;
 LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;

    while ((w_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line()) == LINE_OK))
    {
        text = get_line();
        std::cout << text <<std::endl;
		w_start_line = w_checked_idx;

        switch (w_check_state)
        {
        case CHECK_STATE_REQUESTLINE:
        {	std::cout << "====header3===============\n";
            ret = parse_request_line(text);
            if (ret == BAD_REQUEST)
                return BAD_REQUEST;
            break;
        }
        case CHECK_STATE_HEADER:
        {	std::cout << "====head1===============\n";
            ret = parse_headers(text);
            if (ret == BAD_REQUEST)
                return BAD_REQUEST;
            else if (ret == GET_REQUEST)
            {
					std::cout << "====header2===============\n";
                return do_request();
            }
            break;
        }
        case CHECK_STATE_CONTENT:
        {	std::cout << "====header4===============\n";
            ret = parse_content(text);
            if (ret == GET_REQUEST)
                return do_request();
            line_status = LINE_OPEN;
            break;
        }
        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}
void http_request::unmap()
{
    if (w_file_address)
    {
        munmap(w_file_address, w_file_stat.st_size);
        w_file_address = 0;
    }
}
bool http_request::write()
{
    int temp = 0;

    if (bytes_to_send == 0)
    {
        modfd(w_epollfd, w_sockfd, EPOLLIN, w_TRIGMode);
        initt();
        return true;
    }

    while (1)
    {
        temp = writev(w_sockfd, w_iv, w_iv_count);

        if (temp < 0)
        {
            if (errno == EAGAIN)
            {
                modfd(w_epollfd, w_sockfd, EPOLLOUT, w_TRIGMode);
                return true;
            }
            unmap();
            return false;
        }

        bytes_have_send += temp;
        bytes_to_send -= temp;
        if (bytes_have_send >= w_iv[0].iov_len)
        {
            w_iv[0].iov_len = 0;
            w_iv[1].iov_base = w_file_address + (bytes_have_send - w_write_idx);
            w_iv[1].iov_len = bytes_to_send;
        }
        else
        {
            w_iv[0].iov_base = w_write_buf + bytes_have_send;
            w_iv[0].iov_len = w_iv[0].iov_len - bytes_have_send;
        }

        if (bytes_to_send <= 0)
        {
            unmap();
            modfd(w_epollfd, w_sockfd, EPOLLIN, w_TRIGMode);
            if (w_linger)
            {
                initt();
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}
bool http_request::add_response(const char *format, ...)
{
    if (w_write_idx >= WRITE_BUFFER_SIZE)
        return false;
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(w_write_buf + w_write_idx, WRITE_BUFFER_SIZE - 1 - w_write_idx, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - w_write_idx))
    {
        va_end(arg_list);
        return false;
    }
    w_write_idx += len;
    va_end(arg_list);



    return true;
}
bool http_request::add_status_line(int status, const char *title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}
bool http_request::add_headers(int content_len)
{
    return add_content_length(content_len) && add_linger() &&
           add_blank_line();
}
bool http_request::add_content_length(int content_len)
{
    return add_response("Content-Length:%d\r\n", content_len);
}
bool http_request::add_content_type()
{
    return add_response("Content-Type:%s\r\n", "text/html");
}
bool http_request::add_linger()
{
    return add_response("Connection:%s\r\n", (w_linger == true) ? "keep-alive" : "close");
}
bool http_request::add_blank_line()
{
    return add_response("%s", "\r\n");
}
bool http_request::add_content(const char *content)
{
    return add_response("%s", content);
}
bool http_request::process_write(http_request::HTTP_CODE ret)
{
    switch (ret)
    {
    case INTERNAL_ERROR:
    {
        add_status_line(500, error_500_title);
        add_headers(strlen(error_500_form));
        if (!add_content(error_500_form))
            return false;
        break;
    }
    case BAD_REQUEST:
    {
        add_status_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form))
            return false;
        break;
    }
    case FORBIDDEN_REQUEST:
    {
        add_status_line(403, error_403_title);
        add_headers(strlen(error_403_form));
        if (!add_content(error_403_form))
            return false;
        break;
    }
    case FILE_REQUEST:
    {
        add_status_line(200, ok_200_title);
        if (w_file_stat.st_size != 0)
        {
            add_headers(w_file_stat.st_size);
            w_iv[0].iov_base = w_write_buf;
            w_iv[0].iov_len = w_write_idx;
            w_iv[1].iov_base = w_file_address;
            w_iv[1].iov_len = w_file_stat.st_size;
            w_iv_count = 2;
            bytes_to_send = w_write_idx + w_file_stat.st_size;
            return true;
        }
        else
        {
            const char *ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            if (!add_content(ok_string))
                return false;
        }
    }
    default:
        return false;
    }
    w_iv[0].iov_base = w_write_buf;
    w_iv[0].iov_len = w_write_idx;
    w_iv_count = 1;
    bytes_to_send = w_write_idx;
    return true;
}
void http_request::process()
{

	  std::cout << "=== serverconfig ===\n";
    for (const auto &server : *serverconfig) {
        std::cout << "Server: " << server.server_name
                  << ", IP: " << server.ip
                  << ", Ports: ";
        for (int port : server.ports) {
            std::cout << port << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "====================\n";
	HTTP_CODE read_ret = process_read();
	    if (read_ret == NO_REQUEST)
	    {
			std::cout << "====NO===============\n";
	        modfd(w_epollfd, w_sockfd, EPOLLIN, w_TRIGMode);
	        return;
	    }
    bool write_ret = process_write(read_ret);
    if (!write_ret)
    {
        close_conn();
    }
    modfd(w_epollfd, w_sockfd, EPOLLOUT, w_TRIGMode);
}
