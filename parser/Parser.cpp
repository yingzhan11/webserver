#include "Parser.hpp"

Parser::Parser(int ac, char **av)
{
    //1-检查ac, av格式是否正确TODO，如果没有av，ac，就用默认路径
    if (ac > 2)
        throw std::runtime_error("Parser Error: Too many arguments");
    else if (ac == 2)
        this->_configPath = av[1];
    else
        this->_configPath = "configFiles/multi_ports.conf"; //默认路径
        // std::cout << "Using default config file: " << this->_configPath << std::endl;
    
    //2-检查confFile是否能正确打开
    this->_configFileStream.open(this->_configPath.c_str());
    if (!this->_configFileStream.is_open())
        throw std::runtime_error("Parser Error: Couldn't open .conf file.");
    // else
    //     std::cout << "Config file opened successfully: " << this->_configPath << std::endl;
    
    //3-准备parser
    _initValidKeywords();

    //4-parser
    _startParsing();
}

Parser::~Parser()
{
    this->_configFileStream.close();
}

/*Private*/
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

    // _validKeywords.insert("try_file");
}

//开始parser解析文件，
void Parser::_startParsing()
{
    char currentChar = this->_configFileStream.get();
    // std::cout << "1-Initial char read: '" << currentChar << "'" << std::endl;

    std::string token = this->_checkNextToken(currentChar);
    // std::cout << "1-First token: " << token << std::endl;

    while (token != "END")
    {
        // std::cout << "1-Parsing server block..." << std::endl;
        //获取第一个server的config setting
        this->_parseServerSetting(token, currentChar);
        //放到servers settings里
        this->servers_settings.push_back(this->_server_setting);
        //获取下一个token，可能是下一个server，可能是END
        token = this->_checkNextToken(currentChar);
    }
}

//检查下一个token
std::string Parser::_checkNextToken(char &currentChar)
{
    std::string token = "";
    // std::cout << "2-Checking next token, current char: '" << currentChar << "'" << std::endl;

    while (!this->_configFileStream.eof()) 
    {
        if (this->_skipSpacesAndComments(currentChar)) {
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
            return (this->_getKeyword(currentChar, token));
        }
        token += currentChar;
        currentChar = this->_configFileStream.get();
    }
    if (this->_bracketOpen != 0)
        throw std::runtime_error("Error: Bracket error detected.");
    if (!this->_foundServer)
        throw std::runtime_error("Error: No server configuration found.");
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
        throw std::runtime_error("Error: Invalid keyword: " + keyword);
    if (keyword == "server")
    {
        this->_foundServer = true;
        //只往后检查，不移动token
        if(this->_checkNextToken(currentChar) != "{")
            throw std::runtime_error("Error: Server should be followed by \"{\"");
    }
    // std::cout << "3-Getting keyword: " << keyword << std::endl;
    // std::cout << "3-Next char after keyword: '" << currentChar << "'" << std::endl;
    return (keyword);
}

//解析server block
void Parser::_parseServerSetting(std::string &token, char &currentChar)
{
    // std::cout << "2-Entered _parseServerSetting function. token is:" << token << std::endl;
    
    if (token != "server")
        throw std::runtime_error("2-Error: Conf block must start with \"server\".");
    
    this->_server_setting.clear();
    while (token != "}" && token != "END")
    {
        //不是server和location就获取config
        if (token != "server")
            this->_getSetting(token, currentChar);
        if (token == "location")
        {
            std::cout << "need get location\n";
        }
        //是server就获取下一个token
        token = this->_checkNextToken(currentChar);
    }
    //这个server block结束了,检查括号是否匹配
    if (this->_bracketOpen != 0)
        throw std::runtime_error("2-Error: Bracket error detected.");    
}

void Parser::_getSetting(std::string const &key, char &currentChar)
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
        // if (currentChar == '{')
        //     break;
        value += currentChar;
        currentChar = this->_configFileStream.get();
    }
    if (value.empty())
        throw std::runtime_error("4-Error: No value found for this key: " + key);
    
    this->_server_setting[key] = value;
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
    // if (currentChar == '#') 
    // {
    //     std::string discardedLine;
    //     std::getline(this->fileStream, discardedLine); 
    //     currentChar = this->fileStream.get(); 
    //     return true;
    // }
    return false;
}