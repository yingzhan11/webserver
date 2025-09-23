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

using RawServer = std::map<std::string, std::string>;

class Parser
{
    private:
        std::string     _configPath;
        std::ifstream   _configFileStream;
        std::set<std::string>   _validKeywords;
        int _bracketOpen;
        bool  _foundServer;

        RawServer _server_setting;

        void _initValidKeywords();
        void _startParsing();

        std::string _checkNextToken(char &currentChar);
        std::string _getKeyword(char &currentChar, std::string &keyword);

        void _parseServerSetting(std::string &token, char &currentChar);
        void _getSetting(std::string const &key, char &currentChar);
        


        bool _skipSpacesAndComments(char &currentChar);
        
    public:
        std::vector<RawServer> servers_settings;

        Parser(int ac, char **av);
        ~Parser();
        

};