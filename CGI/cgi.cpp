#include "cgi.hpp"

CGI::CGI(std::string realFile, std::string uploadTo, std::vector<char> &payload, size_t requestLength) :
_outFile(0), _status(0), _realFile(realFile), _uploadTo(uploadTo), _payLoad(payload),
_requestLength(requestLength), _env() {}

CGI::~CGI()
{
    for (size_t i = 0; i < this->_env.size(); ++i)
        delete[] _env[i];
}

//----------------------------------------------------------------------------------
void _handle_timeout(pid_t pid)
{
    kill(pid, SIGKILL);
}

time_t _nowTime()
{
    return (std::time(NULL));
}

bool _hasTimeOut(time_t start_time)
{
	const int timeout = 1;
	return (_nowTime() - start_time > timeout);
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
std::string _itoa(int n)
{
	std::stringstream ss;
	ss << n;
	return ss.str();
}
std::string _getTimeStamp(const char *format)
{
	time_t		now = _nowTime();
	struct tm	tstruct;
	char		buf[80];

	tstruct = *localtime(&now); 
	strftime(buf, sizeof(buf), format, &tstruct);
	return (buf);
}
std::string _defaultErrorPages(int status, std::string subText)
{
	std::string statusDescrip = _errorHTML[status];

	std::string BodyPage = 
	"<!DOCTYPE html>\n"
	"<html lang=\"pt-BR\">\n"
	"<head>\n"
	"    <meta charset=\"UTF-8\">\n"
	"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, maximum-scale=1\">\n"
	"    <link rel=\"canonical\" href=\"https://www.site1/index.html\" />\n"
	"    <link rel=\"shortcut icon\" href=\"/favicon.ico\">\n"
	"    <title>Error</title>\n"
	"    <link rel=\"stylesheet\" href=\"/styles.css\">\n"
	"    <link href=\"https://fonts.googleapis.com/css2?family=DM+Mono:ital,wght@0,300;0,400;0,500;1,300;1,400;1,500&family=Major+Mono+Display&display=swap\" rel=\"stylesheet\">\n"
	"</head>\n"
	"<body>\n"
	"    <div class=\"container-top\">\n"
	"        <h3 class=\"title\">Error " + _itoa(status) + ": " + statusDescrip + "</h3>\n"
	"        <p class=\"general-paragraph\">" + subText + " </p>\n"
	"        <footer>\n"
	"            <p>Copyright © 2024 Clara Franco & Ívany Pinheiro.</p>\n"
	"        </footer>\n"
	"    </div>\n"
	"</body>\n"
	"</html>\n";

	std::string HTMLHeaders = 
	"HTTP/1.1 " + _itoa(status) + " " + statusDescrip + "\n"
	"Date: " + _getTimeStamp("%a, %d %b %Y %H:%M:%S GMT") + "\n" +
	"Server: Webserv/1.0.0 (Linux)\n" +
	"Connection: keep-alive\n" +
	"Content-Type: text/html; charset=utf-8\n" +
	"Content-Length: " + _itoa(BodyPage.size()) + "\n\n";

	std::string HtmlErrorContent = HTMLHeaders + BodyPage;
	return (HtmlErrorContent);
}
//----------------------------------------------------------------------------------------

void CGI::execute()
{
    std::string tmp = ".inputFile";
    FILE* inputFilePtr = NULL;
    int inputFileFd;

    // _exportEnvPath();
    this->_env.push_back(NULL);
    this->_args.push_back(const_cast<char*>("python3"));
    this->_args.push_back(const_cast<char*>(this->_realFile.c_str()));
    this->_args.push_back(const_cast<char*>(this->_uploadTo.c_str()));
    this->_args.push_back(NULL);

    // _setOutFile();
    if ((this->_outFile = open("cgi.html", O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1)
        throw std::runtime_error(_defaultErrorPages(500, "The server was unable to create output for the CGI to write to"));

    if ((inputFileFd = _writeRequestToCGI(tmp, inputFilePtr)) == -1)
        throw std::runtime_error(_defaultErrorPages(500, "The server was unable to write the request to the CGI"));

    pid_t pid = fork();
    if (pid == -1)
		throw std::runtime_error(_defaultErrorPages(500, "The server was unable to open a child process for the CGI"));

    if (pid == 0)
    {
        dup2(inputFileFd, STDIN_FILENO);
		close(inputFileFd);

        dup2(this->_outFile, STDOUT_FILENO);
		close(this->_outFile);

        std::string python3 = "/usr/bin/python3";

        if (execve(python3.c_str(), &(this->_args[0]), &(this->_env[0])) == -1)
			throw std::runtime_error(_defaultErrorPages(500, "The server was unable to execute the CGI"));
		exit (1);
    }
    else
    {
        close(inputFileFd);
		close(this->_outFile);

        time_t start_time = _nowTime();
        while (true)
        {
            pid_t result = waitpid(pid, &(this->_status), WNOHANG);
            if (result < 0)
				throw std::runtime_error(_defaultErrorPages(500, "The server detected an error while executing the CGI"));
			else if (result == 0)
			{
				if (_hasTimeOut(start_time))
				{
					_handle_timeout(pid);
                    // kill(pid, SIGKILL);
					throw std::runtime_error(_defaultErrorPages(500, "timeout in cgi"));
					break;
				}
			}
            else
                break;
        }
        
    }
}

void CGI::setNewEnv(std::string key, std::string value)
{
    if (!value.empty() && !key.empty())
    {
        std::string env = key + "=" + value;
        char* env_heap = new char[env.size() + 1];
        std::copy(env.begin(), env.end(), env_heap);
        env_heap[env.size()] = '\0'; 
        this->_env.push_back(env_heap);
    }
}



int CGI::_writeRequestToCGI(std::string& fname, FILE*& filePtr)
{
	std::ofstream inputFile(fname.c_str());
	if (!inputFile.is_open())
		return (-1);
	
	for (size_t i = 0; i < _payLoad.size(); i++)
		inputFile << _payLoad[i];
	inputFile.close();

	filePtr = std::fopen(fname.c_str(), "r");
	if (filePtr == NULL)
		return (-1);
	return(fileno(filePtr));
} 