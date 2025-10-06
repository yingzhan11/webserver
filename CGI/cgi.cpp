#include "cgi.hpp"

CGI::CGI(std::string realFile, std::string uploadTo, std::vector<char> &payload, size_t requestLength) :
_outFile(0), _status(0), _realFile(realFile), _uploadTo(uploadTo), _payLoad(payload),
_requestLength(requestLength), _env() {}

CGI::~CGI()
{
    for (size_t i = 0; i < this->_env.size(); ++i)
        delete[] _env[i];
}

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
        throw std::runtime_error("The server was unable to create output for the CGI to write to");
		// throw std::runtime_error(Utils::_defaultErrorPages(500, "O servidor nao foi capaz de criar saida para o CGI escrever"));

    if ((inputFileFd = _writeRequestToCGI(tmp, inputFilePtr)) == -1)
        throw std::runtime_error("The server was unable to write the request to the CGI");
		// throw std::runtime_error(Utils::_defaultErrorPages(500, "O servidor nao foi capaz de escrever o request para o CGI"));

    pid_t pid = fork();
    if (pid == -1)
		throw std::runtime_error("The server was unable to open a child process for the CGI");

    if (pid == 0)
    {
        dup2(inputFileFd, STDIN_FILENO);
		close(inputFileFd);

        dup2(this->_outFile, STDOUT_FILENO);
		close(this->_outFile);

        std::string python3 = "/usr/bin/python3";

        if (execve(python3.c_str(), &(this->_args[0]), &(this->_env[0])) == -1)
			throw std::runtime_error("The server was unable to execute the CGI");
		exit (1);
    }
    else
    {
        close(inputFileFd);
		close(this->_outFile);

        time_t start_time = std::time(NULL);
        while (true)
        {
            pid_t result = waitpid(pid, &(this->_status), WNOHANG);
            if (result < 0)
				throw std::runtime_error("The server detected an error while executing the CGI");
			else if (result == 0)
			{
				if (_hasTimeOut(start_time))
				{
					// _handle_timeout(pid);
                    kill(pid, SIGKILL);
					throw std::runtime_error("timeout in cgi");
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

bool CGI::_hasTimeOut(time_t start_time)
{
	const int timeout = 1;
	return (std::time(NULL) - start_time > timeout);
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