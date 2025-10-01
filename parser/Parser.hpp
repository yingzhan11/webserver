#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>

#include "../config/Config.hpp"

class Parser
{
    private:
        std::string     _configPath;
        std::ifstream   _configFileStream;
        std::set<std::string>   _validKeywords;
        int _bracketOpen;
        bool  _foundServer;

        RawSetting _serverSetting;
        std::map<std::string, RawSetting> _locations;

        void _isValidInput(int ac, char **av);
        void _isValidConfigFile();
        void _initValidKeywords();
        void _startParsing();

        std::string _checkNextToken(char &currentChar);
        std::string _getKeyword(char &currentChar, std::string &keyword);

        void _parseServerSetting(std::string &token, char &currentChar);
        void _getSetting(std::string const &key, char &currentChar, RawSetting &setting);
        std::pair<std::string, RawSetting> _getLocationSetting(char &currentChar);


        bool _skipSpacesAndComments(char &currentChar);
        
    public:
        Parser(int ac, char **av);
        ~Parser();
        

};