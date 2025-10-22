#include "http_request.hpp"
#include <dirent.h>     
#include <sys/types.h>  
#include <sys/stat.h>   
#include <fcntl.h>      
#include <unistd.h>   

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
// 归一化绝对路径
static bool realpath_strict(const std::string& p, std::string& out) {
    char buf[PATH_MAX];
    if (!realpath(p.c_str(), buf)) return false;
    out.assign(buf);
    return true;
}

// 判断 target 是否在 baseDir（绝对路径）之内
static bool is_inside_dir(const std::string& baseDirAbs, const std::string& targetAbs) {
    if (targetAbs.size() <= baseDirAbs.size()) return false;
    if (targetAbs.compare(0, baseDirAbs.size(), baseDirAbs) != 0) return false;
    // 防止 /var/www_rootX 假前缀
    if (targetAbs[baseDirAbs.size()] != '/') return false;
    return true;
}

// 只允许删除常规文件（拒绝目录/符号链接/设备等）
static http_request::HTTP_CODE unlink_regular_file_only(const std::string& absPath) {
    struct stat st;
    if (lstat(absPath.c_str(), &st) < 0) {
        return (errno == ENOENT) ? http_request::NO_RESOURCE : http_request::FORBIDDEN_REQUEST;
    }
    if (!S_ISREG(st.st_mode)) {
        return http_request::FORBIDDEN_REQUEST; // 不是常规文件就拒绝
    }
    if (unlink(absPath.c_str()) == 0) {
        return http_request::DELETE_OK;
    }
    if (errno == ENOENT) return http_request::NO_RESOURCE;
    if (errno == EACCES || errno == EPERM || errno == EROFS) return http_request::FORBIDDEN_REQUEST;
    return http_request::INTERNAL_ERROR;
}

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
    // std::cout<<"addfd\n"<<epollfd<<std::endl;
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
    is_cgi = 0;
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
            // if (c == '\r') std::cout << "\\r";
            // else if (c == '\n') std::cout << "\\n";
            // else std::cout << c;
        }
        // std::cout << std::endl;
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
    {
        w_method = GET;
        //TODO just for test
        if (strstr(w_url, ".py") != nullptr)
        {
            is_cgi = 1;
            std::cout << "++---+++++++++++url" << w_url << "+++++++" << std::endl;
        }
    }
    else if (strcasecmp(method, "POST") == 0)
    {
        w_method = POST;
        is_cgi = 1;
    }
    else if (strcasecmp(method, "DELETE") == 0)
    {
        w_method = DELETE;
        is_cgi = 0;                           // DELETE 不走 CGI
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
    else if (strncasecmp(text, "Content-Type:", 13) == 0)
    {
        text += 13; text += strspn(text, " \t");
        w_content_type = text;  
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

void http_request::_exportEnv(CGI &cgi)
{
	cgi.setNewEnv("GATEWAY_INTERFACE", "CGI/1.1");
	cgi.setNewEnv("SERVER_PROTOCOL", "HTTP/1.1");
	cgi.setNewEnv("SERVER_SOFTWARE", "server");

    cgi.setNewEnv("QUERY_STRING", "");

	// cgi.setNewEnv("ROOT_FOLDER" , this->serverconfig.root_directory);
    // 当前服务器根目录（如果有 server 配置）
    if (serverconfig && !serverconfig->empty())
    {
        const ServerConfig &conf = *(serverconfig->begin());
        cgi.setNewEnv("ROOT_FOLDER", conf.root_directory);
    }
    else
        cgi.setNewEnv("ROOT_FOLDER", "");
    
	cgi.setNewEnv("SCRIPT_FILENAME" , this->w_real_file);
    cgi.setNewEnv("SCRIPT_NAME", this->w_url);

	// cgi.setNewEnv("REQUEST_METHOD", this->_myRequest.getMethod());
    std::string method;
    switch (w_method)
    {
        case http_request::GET: method = "GET"; break;
        case http_request::POST: method = "POST"; break;
        case http_request::PUT: method = "PUT"; break;
        case http_request::DELETE: method = "DELETE"; break;
        case http_request::HEAD: method = "HEAD"; break;
        default: method = "UNKNOWN"; break;
    }
    cgi.setNewEnv("REQUEST_METHOD", method);
    // 内容长度 & 类型
    cgi.setNewEnv("CONTENT_LENGTH", std::to_string(w_content_length));
    cgi.setNewEnv("CONTENT_TYPE", w_content_type.empty() ? "" : w_content_type);

    // Host 字段
    cgi.setNewEnv("SERVER_NAME", w_host);
    cgi.setNewEnv("HTTP_HOST", w_host ? w_host : "");

    // payload（POST body）
    if (w_string)
        cgi.setNewEnv("PAYLOAD", w_string);
    else
        cgi.setNewEnv("PAYLOAD", "");
}

// 最长前缀匹配：从 server.routes 里选出 path 最长且是 url 前缀的那条
static const RouteConfig* pick_route_for(const ServerConfig& s, const std::string& urlPath)
{
    const RouteConfig* best = NULL;
    size_t bestLen = 0;
    for (size_t i = 0; i < s.routes.size(); ++i) {
        const RouteConfig& r = s.routes[i];
        if (r.path.empty()) continue;

        bool matched = false;

        // 1️⃣ 普通路径前缀匹配（/upload、/cgi 等）
        if (urlPath.compare(0, r.path.size(), r.path) == 0)
            matched = true;

        // 2️⃣ 额外：文件后缀匹配（.py、.php、.cgi）
        else if (r.path[0] == '.' && urlPath.size() >= r.path.size()) {
            if (urlPath.compare(urlPath.size() - r.path.size(), r.path.size(), r.path) == 0)
                matched = true;
        }

        if (matched && r.path.size() > bestLen) {
            best = &r;
            bestLen = r.path.size();
        }
    }

    // fallback：没找到时尝试 path="/"
    if (!best) {
        for (size_t i = 0; i < s.routes.size(); ++i) {
            if (s.routes[i].path == "/") {
                best = &s.routes[i];
                break;
            }
        }
    }
    return best;
}

bool http_request::compute_paths_once(const std::string& /*urlPath*/) {
    std::cerr << "\n========== [PATHS DEBUG] (.py mode) ==========\n";

    // 1) root -> w_root_abs
    std::string root = "/home/yan/webserver/site1";
    if (serverconfig && !serverconfig->empty()) {
        const ServerConfig &cfg = *(serverconfig->begin());
        if (!cfg.root_directory.empty()) root = cfg.root_directory;
    }
    std::cerr << "[PATHS] cfg.root_directory = " << root << "\n";
    if (!realpath_strict(root, w_root_abs)) {
        std::cerr << "[PATHS] realpath(root) failed\n";
        return false;
    }
    std::cerr << "[PATHS] w_root_abs = " << w_root_abs << "\n";

    // 2) 只看 ".py" 这条 route，找 upload_to
    w_upload_abs.clear();
    w_upload_url_prefix.clear();

    if (!serverconfig || serverconfig->empty()) {
        std::cerr << "[PATHS] empty serverconfig\n";
        return true;
    }
    const ServerConfig &srv = *(serverconfig->begin());

    const RouteConfig* py = NULL;
    for (size_t i = 0; i < srv.routes.size(); ++i) {
        if (srv.routes[i].path == ".py") { py = &srv.routes[i]; break; }
    }
    if (!py) {
        std::cerr << "[PATHS] no '.py' route found\n";
        std::cerr << "===============================================\n";
        return true;
    }

    std::cerr << "[PATHS] '.py' route found, upload_to = " << py->upload_to << "\n";
    if (py->upload_to.empty()) {
        std::cerr << "[PATHS] '.py' route has NO upload_to\n";
        std::cerr << "===============================================\n";
        return true;
    }

    // 保存 URL 前缀（比如 "/uploads"）
    w_upload_url_prefix = py->upload_to;
    if (w_upload_url_prefix.empty() || w_upload_url_prefix[0] != '/')
        w_upload_url_prefix = "/" + w_upload_url_prefix; // 兜底成以 / 开头

    // 3) 物理目录 = w_root_abs + upload_to
    std::string physical = w_root_abs + w_upload_url_prefix; // upload_to 被当成站点相对路径
    std::cerr << "[PATHS] upload physical = " << physical << "\n";

    std::string up_abs;
    if (realpath_strict(physical, up_abs)) {
        struct stat st;
        if (stat(up_abs.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            w_upload_abs = up_abs;
            std::cerr << "[PATHS] w_upload_abs = " << w_upload_abs << "\n";
        } else {
            std::cerr << "[PATHS] upload path exists but is NOT a directory\n";
        }
    } else {
        std::cerr << "[PATHS] realpath(upload) failed (dir likely missing)\n";
    }
    std::cerr << "===============================================\n";
    return true;
}



http_request::HTTP_CODE http_request::do_request()
{  
    const std::string urlPath = (w_url && *w_url) ? std::string(w_url) : std::string("/");

    // ✅ 计算并缓存：w_root_abs / w_upload_abs
    if (!compute_paths_once(urlPath)) {
        return INTERNAL_ERROR;
    } if (w_method == DELETE) {
        std::cerr << "\n========== [DELETE DEBUG] (.py upload_to) ==========\n";

        // 必须先算出缓存
        const std::string urlPath = (w_url && *w_url) ? std::string(w_url) : std::string("/");
        if (!compute_paths_once(urlPath)) {
            std::cerr << "[DELETE] compute_paths_once failed\n";
            return INTERNAL_ERROR;
        }

        std::cerr << "[DELETE] urlPath = " << urlPath << "\n";
        std::cerr << "[DELETE] w_root_abs = " << w_root_abs << "\n";
        std::cerr << "[DELETE] w_upload_url_prefix = " << w_upload_url_prefix << "\n";
        std::cerr << "[DELETE] w_upload_abs = " << w_upload_abs << "\n";

        if (w_upload_abs.empty() || w_upload_url_prefix.empty()) {
            std::cerr << "[DELETE] no upload_to configured on .py route or dir invalid\n";
            return FORBIDDEN_REQUEST;
        }

        // URL 必须以 upload_to (URL 前缀) 开头，且边界正确
        const std::string& P = w_upload_url_prefix; // 例如 "/uploads"
        if (!(urlPath.compare(0, P.size(), P) == 0 &&
            (urlPath.size() == P.size() || urlPath[P.size()] == '/'))) {
            std::cerr << "[DELETE] URL does not start with upload URL prefix\n";
            return FORBIDDEN_REQUEST;
        }

        // 相对路径部分
        std::string rel = urlPath.substr(P.size());
        if (!rel.empty() && rel[0] == '/') rel.erase(0, 1);
        std::cerr << "[DELETE] rel = " << rel << "\n";

        // 目标物理路径
        std::string target_raw = w_upload_abs;
        if (!rel.empty()) target_raw += "/" + rel;
        std::cerr << "[DELETE] target_raw = " << target_raw << "\n";

        std::string target_abs;
        if (!realpath_strict(target_raw, target_abs)) {
            std::cerr << "[DELETE] realpath(target) failed => 404\n";
            return NO_RESOURCE;
        }
        std::cerr << "[DELETE] target_abs = " << target_abs << "\n";

        if (!is_inside_dir(w_upload_abs, target_abs)) {
            std::cerr << "[DELETE] target not inside upload dir => 403\n";
            return FORBIDDEN_REQUEST;
        }

        http_request::HTTP_CODE res = unlink_regular_file_only(target_abs);
        std::cerr << "[DELETE] unlink result code = " << res << "\n";
        if (res == DELETE_OK) std::cerr << "[DELETE] ✅ deleted\n";
        std::cerr << "===============================================\n";
        return res;
    }

    // 默认根目录
    std::string root = "";
    if (serverconfig && !serverconfig->empty()) {
        const ServerConfig &cfg = *(serverconfig->begin());
        if (!cfg.root_directory.empty()) {
            root = cfg.root_directory;
        }
    }
    fprintf(stderr, "[DEBUG] method=%d is_cgi=%d url=%s\n",
            (int)w_method, is_cgi, w_url ? w_url : "(null)");
    fflush(stderr);

    if (is_cgi == 1)
    {
        // === 找到当前请求对应的 route ===
        std::string upload_path = "";  // 默认
        if (serverconfig && !serverconfig->empty()) {
            const ServerConfig &srv = *(serverconfig->begin());
            const std::string urlPath = (w_url && *w_url) ? std::string(w_url) : std::string("/");

            std::cout << "[UPLOAD DEBUG] urlPath = " << urlPath << std::endl;
            std::cout << "[UPLOAD DEBUG] server.root_directory = " << srv.root_directory << std::endl;

            const RouteConfig* r = pick_route_for(srv, urlPath);
            if (r) {
                std::cout << "[UPLOAD DEBUG] matched route.path = " << r->path << std::endl;
                std::cout << "[UPLOAD DEBUG] route.upload_to = " << r->upload_to << std::endl;
            } else {
                std::cout << "[UPLOAD DEBUG] no matching route found for this URL" << std::endl;
            }

            if (r && !r->upload_to.empty()) {
                upload_path = r->upload_to;   // ← 用配置里的 upload_to 覆盖默认
                // 如果配置里是相对路径，比如 "/upload"，可以拼上 server 根目录：
                if (!srv.root_directory.empty() && upload_path[0] == '/')
                    upload_path = srv.root_directory + upload_path;

                std::cout << "[UPLOAD DEBUG] final upload_path after merge = " << upload_path << std::endl;
            } else {
                std::cout << "[UPLOAD DEBUG] route.upload_to empty, using default upload_path = " << upload_path << std::endl;
            }
        }

        char abs_root[PATH_MAX];
        if (realpath(root.c_str(), abs_root)) {
            root = abs_root;
        }
                
        std::cout << "++++++++++++++++++++++++++get cgi+++++++++++++++" << std::endl;
        std::cout << "[CGI] root = " << root << std::endl;
        std::cout << "[CGI] url = " << w_url << std::endl;
        // 确保 root 是绝对路径

        //获取cgi路径（读取location，或默认）TODO
        snprintf(w_real_file, FILENAME_LEN, "%s/cgi%s", root.c_str(), w_url);
                
        std::cout << "[CGI] wrf = " << w_real_file << std::endl;
        //需要检查路径是否存在，不在要报错 TODO
        // 传入 POST 的 body（multipart/raw 都行），否则 CGI 读不到
        std::vector<char> tmp;
        if (w_method == POST && w_string && w_content_length > 0) {
            tmp.assign(w_string, w_string + w_content_length);
        }

        // 用基于 route.upload_to 计算出的 upload_path 构造 CGI
        CGI cgi(w_real_file, upload_path, tmp, w_content_length);

        // 导出环境（会设置 CONTENT_LENGTH/CONTENT_TYPE 等）
        _exportEnv(cgi);
        cgi.setNewEnv("UPLOAD_PATH", upload_path);
        std::cout << "[CGI] UPLOAD_PATH = " << upload_path << std::endl;
        // 检查 upload_path 是否存在且是目录
        struct stat upload_stat;
        if (stat(upload_path.c_str(), &upload_stat) < 0) {
            std::cerr << "[ERROR] upload path not found: " << upload_path << std::endl;
            // 可选：返回自定义错误页
            return FORBIDDEN_REQUEST;
        }
        if (!S_ISDIR(upload_stat.st_mode)) {
            std::cerr << "[ERROR] upload path is not a directory: " << upload_path << std::endl;
            return FORBIDDEN_REQUEST;
        }

        std::cout << "++++++++++++++++++++++++++into cgi+++++++++++++++" << std::endl;
        try {
            cgi.execute();
        }
        catch (const std::runtime_error& e) {
            std::string err = e.what();
            if (err == "500")
                return INTERNAL_ERROR;
        }
        std::cout << "++++++++++++++++++++++++++cgi done+++++++++++++++" << std::endl;

        // 读取 CGI 输出内容
        std::ifstream outputFile("cgi.html");
        if (!outputFile.is_open()) {
            std::cerr << "[CGI] Cannot open CGI output file!" << std::endl;
            return NO_RESOURCE;
        }

        std::string cgiOutput((std::istreambuf_iterator<char>(outputFile)),
                            std::istreambuf_iterator<char>());
        outputFile.close();

        // 把 cgi.html 映射为响应内容
        if (stat("cgi.html", &w_file_stat) < 0)
            return NO_RESOURCE;

        int fd = open("cgi.html", O_RDONLY);
        if (fd < 0)
            return NO_RESOURCE;

        w_file_address = (char *)mmap(0, w_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);

        return FILE_REQUEST;
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
    bool route_autoindex = false;
    if (serverconfig && !serverconfig->empty()) {
        const ServerConfig &srv = *(serverconfig->begin());           // 默认 server（不引入按 Host 精选）
        const std::string urlPath = (w_url && *w_url) ? std::string(w_url) : std::string("/");
        const RouteConfig* r = pick_route_for(srv, urlPath);
        if (r) route_autoindex = r->directory_listing;                // ← 关键：读出该 route 的 autoindex
    }
    
    if (S_ISDIR(w_file_stat.st_mode))
    {
        // 1) 先尝试 index 文件：优先用 server 的 default_file；没有就回退到 "index.html"
        std::string index_name = "index.html";
        if (serverconfig && !serverconfig->empty()) {
            const ServerConfig &cfg = *(serverconfig->begin());
            if (!cfg.default_file.empty())
                index_name = cfg.default_file;
        }

        std::string index_path = std::string(w_real_file);
        if (!index_path.empty() && index_path[index_path.size() - 1] != '/')
            index_path += "/";
        index_path += index_name;

        struct stat idx_stat;
        if (stat(index_path.c_str(), &idx_stat) == 0 && !S_ISDIR(idx_stat.st_mode)) {
            int fd2 = open(index_path.c_str(), O_RDONLY);
            if (fd2 >= 0) {
                w_file_address = (char *)mmap(0, idx_stat.st_size, PROT_READ, MAP_PRIVATE, fd2, 0);
                close(fd2);
                w_file_stat = idx_stat;
                return FILE_REQUEST;
            }
        }

    
        if (route_autoindex) {
            DIR *dir = opendir(w_real_file);
            if (!dir)
                return FORBIDDEN_REQUEST;

            std::string html;
            html.reserve(4096);
            html += "<!DOCTYPE html><html><head><meta charset=\"utf-8\">";
            html += "<title>Index of ";
            html += (w_url ? w_url : "/");
            html += "</title><style>body{font-family:Arial,Helvetica,sans-serif;padding:12px}ul{line-height:1.6}</style>";
            html += "</head><body><h2>Index of ";
            html += (w_url ? w_url : "/");
            html += "</h2><hr><ul>";

            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                const char *name_c = ent->d_name;
                if (!name_c) continue;
                std::string name = name_c;
                if (name == ".") continue; // 不展示当前目录

                std::string href = (w_url ? std::string(w_url) : std::string("/"));
                if (!href.empty() && href[href.size() - 1] != '/') href += "/";
                href += name;

                html += "<li><a href=\"";
                html += href;
                html += "\">";
                html += name;
                html += "</a></li>";
            }
            closedir(dir);

            html += "</ul><hr></body></html>";

            // 写到临时文件再 mmap（复用你现有 FILE_REQUEST 的发送路径）
            const char *ai_file = "autoindex.html";
            int afd = open(ai_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (afd < 0) return FORBIDDEN_REQUEST;
            ssize_t wr = ::write(afd, html.data(), html.size()); // 注意使用 ::write
            (void)wr;
            close(afd);

            if (stat(ai_file, &w_file_stat) < 0) return NO_RESOURCE;
            int fd3 = open(ai_file, O_RDONLY);
            if (fd3 < 0) return NO_RESOURCE;
            w_file_address = (char *)mmap(0, w_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd3, 0);
            close(fd3);

            return FILE_REQUEST;
        }
        // 3) 未开启 autoindex，则禁止访问目录
        return FORBIDDEN_REQUEST;
    }

    int fd = open(w_real_file, O_RDONLY);
    if (fd < 0)
        return NO_RESOURCE;

    w_file_address = (char *)mmap(0, w_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    return FILE_REQUEST;
}

http_request::HTTP_CODE http_request::process_read()
{
    // std::cout << "11111" <<std::endl;
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;

    while ((w_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line()) == LINE_OK))
    {
        text = get_line();
        // std::cout << text <<std::endl;
		w_start_line = w_checked_idx;

        switch (w_check_state)
        {
            case CHECK_STATE_REQUESTLINE:
            {	
                // std::cout << "====header3===============\n";
                ret = parse_request_line(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }
            case CHECK_STATE_HEADER:
            {	
                // std::cout << "====head1===============\n";
                ret = parse_headers(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                else if (ret == GET_REQUEST)
                {
                        // std::cout << "====header2===============\n";
                    return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT:
            {	
                // std::cout << "====header4===============\n";
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

std::string http_request::_getErrorPage(int code, std::string error_form)
{
    std::string defaultErrorPage;
    if (serverconfig && !serverconfig->empty()) {
        const ServerConfig &srv = *(serverconfig->begin());
        std::unordered_map<int, std::string>::const_iterator it = srv.error_pages.find(code);
        if (it != srv.error_pages.end()) {
            // Found custom page in config
            defaultErrorPage = utils::_readFileContent(it->second, srv.root_directory);  // or whatever loads the file
        } else {
            // Use default error page
            defaultErrorPage = utils::_defaultErrorPages(code, error_form);
        }
    }
    return defaultErrorPage;
}

bool http_request::process_write(http_request::HTTP_CODE ret)
{
    std::string defaultErrorPage;

    switch (ret)
    {
        case INTERNAL_ERROR:
        {
            defaultErrorPage = _getErrorPage(500, error_500_form);
            //defaultErrorPage = utils::_defaultErrorPages(500, error_500_form);
            add_status_line(500, error_500_title);
            break;
        }
        case BAD_REQUEST:
        {
            // 这里应当是 400，而不是 404
            defaultErrorPage = _getErrorPage(400, error_400_form);
            //defaultErrorPage = utils::_defaultErrorPages(400, error_400_form);
            add_status_line(400, error_400_title);
            break;
        }
        case FORBIDDEN_REQUEST:
        {
            defaultErrorPage = _getErrorPage(403, error_403_form);
            //defaultErrorPage = utils::_defaultErrorPages(403, error_403_form);
            add_status_line(403, error_403_title);
            break;
        }
        case NO_RESOURCE:   // ← 新增：处理 404
        {
            defaultErrorPage = _getErrorPage(404, error_404_form);
            //defaultErrorPage = utils::_defaultErrorPages(404, error_404_form);
            add_status_line(404, error_404_title);
            break;
        }
        case DELETE_OK:
        {
            const char *ok_delete =
                "<html><body><h3>✅ File deleted successfully.</h3></body></html>";
            add_status_line(200, ok_200_title);
            add_headers(strlen(ok_delete));
            if (!add_content(ok_delete)) return false;

            w_iv[0].iov_base = w_write_buf;
            w_iv[0].iov_len  = w_write_idx;
            w_iv_count       = 1;
            bytes_to_send    = w_write_idx;
            return true;
        }
        case FILE_REQUEST:
        {
            if (is_cgi == 1) {
                // CGI: 直接把 cgi.html 的内容作为响应体（内部已 mmap 到 w_file_address）
                w_iv[0].iov_base = w_file_address;
                w_iv[0].iov_len  = w_file_stat.st_size;
                w_iv_count       = 1;
                bytes_to_send    = w_file_stat.st_size;
                return true;
            } else {
                add_status_line(200, ok_200_title);
                if (w_file_stat.st_size != 0) {
                    add_headers(w_file_stat.st_size);
                    w_iv[0].iov_base = w_write_buf;
                    w_iv[0].iov_len  = w_write_idx;
                    w_iv[1].iov_base = w_file_address;
                    w_iv[1].iov_len  = w_file_stat.st_size;
                    w_iv_count       = 2;
                    bytes_to_send    = w_write_idx + w_file_stat.st_size;
                    return true;
                } else {
                    const char *ok_string = "<html><body></body></html>";
                    add_headers(strlen(ok_string));
                    if (!add_content(ok_string)) return false;
                }
            }
            break;
        }
        default:
            return false;
    }

    // 只要不是 FILE_REQUEST/DELETE_OK，走这里统一发送错误页
    if (ret != FILE_REQUEST && ret != DELETE_OK) {
        add_headers(defaultErrorPage.size());
        if (!add_content(defaultErrorPage.c_str())) return false;

        w_iv[0].iov_base = w_write_buf;
        w_iv[0].iov_len  = w_write_idx;
        w_iv_count       = 1;
        bytes_to_send    = w_write_idx;
        return true;
    }

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
			// std::cout << "====NO===============\n";
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
