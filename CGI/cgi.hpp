#pragma once

#include <stdio.h> 
#include <sys/types.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h> 
#include <csignal> // Para C++
#include <iostream>
#include <cstring>
#include <exception>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <ctime>
#include <vector>

class CGI
{
    private:
        int _outFile;
        int _status;

        std::string _realFile;
        std::string _uploadTo;
        std::vector<char>& _payLoad;
        size_t _requestLength;

        std::vector<char*> _env;
        std::vector<char*> _args;

        int    _writeRequestToCGI(std::string& fname, FILE*& filePtr);
        bool _hasTimeOut(time_t start_time);

    public:
        CGI(std::string realFile, std::string uploadTo, std::vector<char> &payload, size_t requestLength);
        ~CGI();

        void execute();
        void setNewEnv(std::string key, std::string value);
};