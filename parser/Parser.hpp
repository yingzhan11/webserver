#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>

// #include "../config/Config.hpp"

// class ServerConfig;

struct Location
{
    std::string root;
    std::string redirect;
    std::string tryFile;
    std::string cgiPath;
    std::string cgiExtension;
    std::string uploadTo;
    bool autoindex;
    std::vector<std::string> allowedMethods;
};

using RawServer = std::map<std::string, std::string>;

class Parser
{
    private:
        std::string     _configPath;
        std::ifstream   _configFileStream;
        std::set<std::string>   _validKeywords;
        int _bracketOpen;
        bool  _foundServer;

        RawServer _serverSetting;
        std::pair<std::string, Location> _locationSetting;

        void _isValidInput(int ac, char **av);
        void _isValidConfigFile();
        void _initValidKeywords();
        void _startParsing();

        std::string _checkNextToken(char &currentChar);
        std::string _getKeyword(char &currentChar, std::string &keyword);

        void _parseServerSetting(std::string &token, char &currentChar);
        void _getSetting(std::string const &key, char &currentChar);
        void _getLocationSetting(char &currentChar);


        bool _skipSpacesAndComments(char &currentChar);
        
    public:
        //parser里存了config的原始数据，在config.cpp里对原始数据进行检查和记录
        std::vector<RawServer> serverVec;
        std::map<std::string, Location> locationMap;

        Parser(int ac, char **av);
        ~Parser();
        

};