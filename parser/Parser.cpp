#include "Parser.hpp"

Parser::Parser(int ac, char **av)
{
    //1-检查ac, av格式是否正确，如果没有av，ac，就用默认路径
    _isValidInput(ac, av);

    //2-检查是否是.conf文件，以及.conf文件能否正确打开
    _isValidConfigFile();
    
    //3-准备parser，初始化需要的keywords和参数
    _initValidKeywords();
    this->_bracketOpen = 0;
    this->_foundServer = false;

    //4-parser
    _startParsing();
}

Parser::~Parser()
{
    this->_configFileStream.close();
}

/**
 * prework before parsing
 */
//检查ac, av格式是否正确，如果没有av，ac，就用默认路径
void Parser::_isValidInput(int ac, char **av)
{
    if (ac > 2)
        throw std::runtime_error("Parser: Too many arguments");
    else if (ac == 2)
        this->_configPath = av[1];
    else
        this->_configPath = "configFiles/default.conf"; //默认路径
}

//检查是否是.conf文件，以及.conf文件能否正确打开
void Parser::_isValidConfigFile()
{
    //检查文件名是否正确
    std::string filename = this->_configPath;

    size_t slash = filename.find_last_of("/");
    if (slash != std::string::npos)
        filename = this->_configPath.substr(slash + 1);
    
    size_t dot = filename.find_last_of(".");
    if (dot == std::string::npos || filename.substr(dot) != ".conf" || filename == ".conf")
        throw std::runtime_error("Parser Error: Extension should be .conf");

    //检查confFile是否能正确打开
    this->_configFileStream.open(this->_configPath.c_str());
    if (!this->_configFileStream.is_open())
        throw std::runtime_error("Parser Error: Couldn't open .conf file.");
    // else
    //     std::cout << "Config file opened successfully: " << this->_configPath << std::endl;
}

//初始化合法的keywords
void Parser::_initValidKeywords() 
{
    _validKeywords.insert("server");

    _validKeywords.insert("server_name");
    _validKeywords.insert("listen");
    _validKeywords.insert("host");
    _validKeywords.insert("root");
    _validKeywords.insert("index");
    _validKeywords.insert("client_body_size");
    _validKeywords.insert("error_page");

    _validKeywords.insert("location");
    _validKeywords.insert("allow_methods");
    _validKeywords.insert("cgi_path");
    _validKeywords.insert("cgi_ext");
    _validKeywords.insert("upload_to");
    _validKeywords.insert("autoindex");
    _validKeywords.insert("return");
    _validKeywords.insert("try_file");
}

/**
 * main parseing part
 */
//开始解析文件
void Parser::_startParsing()
{
    char currentChar = this->_configFileStream.get();
    // std::cout << "1-Initial char read: '" << currentChar << "'" << std::endl;

    std::string token = _checkNextToken(currentChar);
    // std::cout << "1-First token: " << token << std::endl;

    while (token != "END")
    {
        // std::cout << "1-Parsing server block..." << std::endl;
        //获取第一个server的config setting
        _parseServerSetting(token, currentChar);
        
        //获取下一个token，可能是下一个server，可能是END
        token = _checkNextToken(currentChar);
    }
}

//解析server block and put into Config.servers
void Parser::_parseServerSetting(std::string &token, char &currentChar)
{
    //std::cout << "2-Entered _parseServerSetting function. token is:" << token << std::endl;
    if (token != "server")
        throw std::runtime_error("Parser: Conf block must start with \"server\".");
    
    this->_serverSetting.clear();
    this->_locations.clear();
    while (token != "}" && token != "END")
    {
        //不是server就获取config
        if (token != "server")
            _getSetting(token, currentChar, this->_serverSetting);
        // if it is location, need more exe
        if (token == "location")
        {
            //TODO
            
            this->_locations.insert(_getLocationSetting(currentChar));

        }
        //是server就获取下一个token
        token = _checkNextToken(currentChar);
    }
    //这个server block结束了,检查括号是否匹配
    if (this->_bracketOpen != 0)
        throw std::runtime_error("Parser: Bracket error detected.");
    // if bracket closed, add this server into servers Config
    else
        Config::getinstance().addConfigToServers(this->_serverSetting, this->_locations);
}

//parse locations
std::pair<std::string, RawSetting> Parser::_getLocationSetting(char &currentChar)
{
    //std::cout << "4-need get location\n";
    std::string locationTitle;
    RawSetting locationSetting;
    std::string token;

    //get the location title
    locationTitle = this->_serverSetting["location"];
    //clean the location title's format, extra spaces
    locationTitle.erase(0, locationTitle.find_first_not_of(" \t\r\v\f"));
    locationTitle.erase(locationTitle.find_last_not_of(" \t\r\v\f") + 1);
    //delete location temp
    this->_serverSetting.erase("location");

    // check the {} format after location
    if (_checkNextToken(currentChar) != "{")
        throw std::runtime_error("Parser: Location xx has no { after it");
    // if there is {, get the value until } or eof
    token = _checkNextToken(currentChar);
    while (!this->_configFileStream.eof() && token != "}")
    {
        _getSetting(token, currentChar, locationSetting);
        token = _checkNextToken(currentChar);
    }
    //fill location structure here or in config? TODO
    return (std::make_pair(locationTitle, locationSetting));
}

//parser value from configFile to RawSetting type
void Parser::_getSetting(std::string const &key, char &currentChar, RawSetting &setting)
{
    if (key == "{")
        return;

    //获取key后面的value
    std::string value = "";

    // std::cout << "4-Getting setting for key: " << key << std::endl;
    // std::cout << "4.1-Current char before reading value: '" << currentChar << "'" << std::endl;
    
    while (!this->_configFileStream.eof() && std::isspace(currentChar)) 
        currentChar = this->_configFileStream.get();
    
    // std::cout << "4.1-Current char before reading value: '" << currentChar << "'" << std::endl;
    while (!this->_configFileStream.eof())
    {
        if (currentChar == ';')
        {
            //跳过；去下一个char
            currentChar = this->_configFileStream.get();
            break;
        }
        if (currentChar == '{')
            break;
        //todo 检查空格和location
        if (std::isspace(currentChar) && key != "error_page" && key != "allow_methods" && key != "location")
            throw std::runtime_error("Parser: Too many value in this key: " + key);
        value += currentChar;
        currentChar = this->_configFileStream.get();
    }
    if (value.empty())
        throw std::runtime_error("Parser: No value found for this key: " + key);
    //if there are multiple ports, add all of them in one key
    if (key == "listen" && setting.find(key) != setting.end())
        setting[key] += " " + value;
    else
        setting[key] = value;
}

/**
 * helper functions
 */
//检查下一个token
std::string Parser::_checkNextToken(char &currentChar)
{
    std::string token = "";
    // std::cout << "2-Checking next token, current char: '" << currentChar << "'" << std::endl;

    while (!this->_configFileStream.eof()) 
    {
        if (_skipSpacesAndComments(currentChar)) {
            continue;
        }
        if (currentChar == '{')
        {
            this->_bracketOpen++;
            currentChar = this->_configFileStream.get();
            return("{");
        }
        if (currentChar == '}')
        {
            this->_bracketOpen--;
            currentChar = this->_configFileStream.get();
            return("}");
        }
        if (std::isalpha(currentChar))
        {
            return (_getKeyword(currentChar, token));
        }
        token += currentChar;
        currentChar = this->_configFileStream.get();
    }
    if (this->_bracketOpen != 0)
        throw std::runtime_error("Parser: Bracket error detected.");
    if (!this->_foundServer)
        throw std::runtime_error("Parser: No server configuration found.");
    // std::cout << "2-End of file reached." << std::endl;
    token = "END";
    return token;
}

std::string Parser::_getKeyword(char &currentChar, std::string &keyword) 
{
    while (!this->_configFileStream.eof() && (std::isalnum(currentChar) || currentChar == '_')) 
    {
        // std::cout << "3-Getting keyword, current char: '" << currentChar << "'" << std::endl;
        keyword += currentChar;
        currentChar = this->_configFileStream.get();
    }
    if (this->_validKeywords.find(keyword) == this->_validKeywords.end()) 
        throw std::runtime_error("Parser: Invalid keyword: " + keyword);
    if (keyword == "server")
    {
        this->_foundServer = true;
        //只往后检查，不移动token
        if(_checkNextToken(currentChar) != "{")
            throw std::runtime_error("Parser: Server should be followed by \"{\"");
    }
    // std::cout << "3-Getting keyword: " << keyword << std::endl;
    // std::cout << "3-Next char after keyword: '" << currentChar << "'" << std::endl;
    return (keyword);
}

bool Parser::_skipSpacesAndComments(char &currentChar) 
{
    if (std::isspace(currentChar)) 
    {
        while (!this->_configFileStream.eof() && std::isspace(currentChar)) 
        {
            currentChar = this->_configFileStream.get();
        }
        return true;
    }
    if (currentChar == '#')
    {
        std::string discardedLine;
        std::getline(this->_configFileStream, discardedLine); 
        currentChar = this->_configFileStream.get(); 
        return true;
    }
    return false;
}