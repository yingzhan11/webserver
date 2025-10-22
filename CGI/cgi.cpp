#include "cgi.hpp"

CGI::CGI(std::string realFile, std::string uploadTo, std::vector<char> &payload, size_t requestLength) :
_outFile(0), _status(0), _realFile(realFile), _uploadTo(uploadTo), _payLoad(payload),
_requestLength(requestLength), _env() {}

CGI::~CGI()
{
    for (size_t i = 0; i < this->_env.size(); ++i)
        delete[] _env[i];
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

void CGI::execute()
{
    std::string tmp = ".inputFile";
	FILE* inputFilePtr = NULL;
    int	 inputFileFD;

    _exportEnvArgs();
    // std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    // for (size_t i = 0; i < _env.size(); ++i) {
    //     if (_env[i])
    //         std::cout << _env[i] << std::endl;
    // }
    // std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    //throw std::runtime_error("500");

    //设置outfile, inputFile
    if ((this->_outFile = open("cgi.html", O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1)
        throw std::runtime_error("500");
        //throw std::runtime_error(utils::_defaultErrorPages(500, "The server was unable to create output for the CGI to write to"));
    if ((inputFileFD = _writeRequestToCGI(const_cast<std::string&>(tmp), inputFilePtr)) == -1)
        throw std::runtime_error("500");
        //throw std::runtime_error(utils::_defaultErrorPages(500, "The server was unable to write the request to the CGI"));


    //fork
    pid_t pid = fork();
    if (pid == -1)
        throw std::runtime_error("500");
		//throw std::runtime_error(utils::_defaultErrorPages(500, "The server was unable to open a child process for the CGI"));

    //std::cout << "22222222222222222222222222222222222\n";
    if (pid == 0)
    {
        dup2(inputFileFD, STDIN_FILENO);
		close(inputFileFD);

		dup2(this->_outFile, STDOUT_FILENO);
		close(this->_outFile);

        std::string python3 = "/usr/bin/python3";

        // std::cout << "33333333333333333333333333333\n";
        if (execve(python3.c_str(), &(this->_args[0]), &(this->_env[0])) == -1)
            throw std::runtime_error("500");
			//throw std::runtime_error(utils::_defaultErrorPages(500, "The server was unable to execute the CGI"));
		//std::cout << "444444444444444444444444\n";
        exit (1);
    }
    else
    {
        close(inputFileFD);
		close(this->_outFile);

        time_t start_time = utils::nowTime();
        const int timeout = 1;
        //std::cout << "5555555555555555555555\n";
        while (true)
        {
            pid_t result = waitpid(pid, &(this->_status), WNOHANG);
            if (result < 0)
                throw std::runtime_error("500");
				//throw std::runtime_error(utils::_defaultErrorPages(500, "The server detected an error while executing the CGI"));
			else if (result == 0)
			{
				if (utils::nowTime() - start_time > timeout)
				{
                    kill(pid, SIGKILL);
                    // waitpid(pid, &(this->_status), 0);
                    throw std::runtime_error("500");
					//throw std::runtime_error(utils::_defaultErrorPages(500, "timeout in cgi"));
					break;
				}
			}
            else
                break;
        }
        //std::cout << "6666666666666666666666666\n";

    }
}

void CGI::_exportEnvArgs()
{
	this->_env.push_back(NULL);
	this->_args.push_back(const_cast<char*>("python3"));
	this->_args.push_back(const_cast<char*>(this->_realFile.c_str()));
	this->_args.push_back(const_cast<char*>(this->_uploadTo.c_str()));
	this->_args.push_back(NULL);
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